#include "gos.h"
#include "../gosAssetBuilder.h"
#include "string/gosStringList.h"
#include "gosAssetBuilder_shader.h"
#include "gosGPU.h"


using namespace gos;
using namespace gos::asset;

//************************************
bool Builder_shader::priv_extractParams (const IniFileSection *sec, Params *out_params)
{
    assert (NULL != sec);
    assert (NULL != out_params);
    
    memset (out_params, 0, sizeof(Params));

    //param:src         e' mandatorio ed indica il nome dello shader in formato testo da compilare
    //                  Lo shader in questione deve esistere in res/01-shader_txt
    if (!sec->get("src", out_params->src, sizeof(out_params->src)))
    {
        gos::logger::err ("asset::Builder_shader::extractParams => can't find param <src>\n");
        return false;
    }

    /*param:def     e' opzionale e, se esiste, e' una sequenza di parole separate da spazio
                    che vengono passate come 'define' durante la compilazione dello shader.
                    Nel file ini le parole possono essere separate da piu' spazi o magari da tab. Quello che voglio io
                    qui e' un elenco di parole, trimmate e ordinate alfabeticamente in modo che 2 dichiarazioni apparentemente differenti tipo:
                        1- def: a     b c
                        2- def: a b c
                        3- def: b   a   c

                    risultino tutte uguali (ovvero lo stesso elenco di 3 parole: "a", "b", "c")
    */
    char s[512];
    if (sec->get ("def", s, sizeof(s)))
    {
        gos::Array<gos::UTF8String> list;
        list.setup (gos::getScrapAllocator(), 1024);

        string::utf8::StringListParser sp;
        sp.toStart (s, ' ');
        
        char parola[128];
        while (sp.next (parola, sizeof(parola)))
            list.append (parola);

        list.bubbleSort ( [](const gos::UTF8String &s1, const gos::UTF8String &s2)
        {
            if (s1.compare(s2) > 0)
                return true;
            return false;
        });

        const u32 n = list.getNElem();
        assert (n>0);
        sprintf_s (out_params->def, sizeof(out_params->def), "%s", list(0).getBuffer());
        for (u32 i=1; i<n; i++)
        {
            strcat_s (out_params->def, sizeof(out_params->def), " ");
            strcat_s (out_params->def, sizeof(out_params->def), list(i).getBuffer());
        }
    }


    //per evitare problemi, controllo che ci siano solo ed esattamente i parametri che mi aspetto
    for (u32 i=0; i<sec->getNIdentifier(); i++)
    {
        const char *paramName = sec->getIdentifierByIndex(i);
        if (!prot_isOneOfThis(paramName, "src", "def", NULL))
        {
            gos::logger::err ("asset::Builder_shader::extractParams => <%s> is not a valid one\n", paramName);
            return false;
        }
    }


    return true;
}

//************************************
bool Builder_shader::build (Context &ctx, u64 buildTimeUTC, const char *sourceFileInfo, const asset::UID &uid_of_iniFile, const IniFileSection *sec, bool doCreateAnAssetFile, gos::GPU *gpu, sBuildResult *out)
{
    assert (ctx.isValid());
    assert (NULL != sec);
    assert (NULL != out);

    out->reset();

    //parse della sezione
    Params params;
    if (!priv_extractParams(sec, &params))
    {
        gos::logger::err ("error parsing IniFileSection\n");
        return false;
    }

    //il parametro src indica una risorsa eResType::shader_txt da cui io dipendo
    //La risorsa deve esistere nel DB
    if (!prot_needResource (ctx, eResType::shader_txt, params.src, &params.uid__resource_shader_txt))
    {
        gos::logger::err ("resource [%s] '%s' not found in DB\n", asset::enumToString(eResType::shader_txt), params.src);
        return false;
    }    

    //calcolo assetUID
    if (!asset::asset_createUID (getAssType(), calc_depth(), &params, sizeof(Params), &out->uid))
    {
        gos::logger::err ("error generating assetUID\n");
        return false;
    }

    /*  Idealmente asset::UID non dovrebbe esistere in tabella visto che lo sto buildando.
        Potenzialmente pero', lo stesso UID puo' essere generato da diversi asset perche' lo specificano
        inline o perche' vi fanno riferimento direttamente usando un runtimeName.
        In linea di massimo quindi, se l'asset esiste gia', non sto a ricompilarlo dato che il 
        risultato sarebbe il medesimo
    */
    const u64 lastTimeBuilt = asset::asset_query_lastTimeBuilt (ctx, out->uid);
    if (0 != lastTimeBuilt)
    {
        //asset::UID esiste gia' nel DB ma e' stato buildato a questo giro di build, quindi va bene,
        //semplicemente non sto a buildarlo una seconda volta
        out->result = eBuildResult::was_already_built;
        return true;
    }


    //asset::UID non esisteva nel DB, ottimo, lo aggiungo e termino con successo
    if (!asset::asset_insert (ctx, out->uid, getAssType(), buildTimeUTC, sourceFileInfo))
    {
        gos::logger::err ("error inserting asset\n");
        return false;
    }

    //aggiungo le sue dipendenze
    if (!asset::depend_add (ctx, out->uid, uid_of_iniFile)) return false;        
    if (!asset::depend_add (ctx, out->uid, params.uid__resource_shader_txt)) return false;

    //segno che e' stato buildato di fresco
    out->result = eBuildResult::just_built;


    //a questo punto devo compilare lo shader per davvero
    if (doCreateAnAssetFile)
    {
        char shaderStage[8];
        if (eAssetType::vtx_shader == getAssType())
            sprintf_s (shaderStage, sizeof(shaderStage), "vert");
        else
            sprintf_s (shaderStage, sizeof(shaderStage), "frag");

        char filenameSRC[1024];
        asset::res_get_folder_nameByType (ctx, eResType::shader_txt, filenameSRC, sizeof(filenameSRC));
        strcat_s (filenameSRC, sizeof(filenameSRC), "/");
        strcat_s (filenameSRC, sizeof(filenameSRC), params.src);

        char filenameDST[1024];
        asset::asset_manufacture_fullFilename (ctx, out->uid, filenameDST, sizeof(filenameDST));

        //creo la versione ottimizzata e la versione con le debug-info. Quest'ultima
        //serve per esempio alle pipeline_def per recuprare i nomi e il formato dei descrittori
        if (!GPU::shader_compile (filenameSRC, shaderStage, params.def, filenameDST, false))
            return false;

        strcat_s (filenameDST, sizeof(filenameDST), "d");
        if (!GPU::shader_compile (filenameSRC, shaderStage, params.def, filenameDST, true))
            return false;

    }


    return true;
}
