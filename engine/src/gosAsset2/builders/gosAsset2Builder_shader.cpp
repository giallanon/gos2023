#include "gos.h"
#include "../gosAsset2Builder.h"
#include "gosAsset2Builder_shader.h"
#include "gosGPU.h"


using namespace gos;
using namespace gos::asset2;

//************************************
bool Builder_shader::priv_extractParams (const char *absFilename, const IniFileSection *sec, Params *out_params)
{
    assert (NULL != sec);
    assert (NULL != out_params);
    
    char s[1024];
    memset (out_params, 0, sizeof(Params));

    //param:src         e' mandatorio ed indica il filename dello shader in formato testo da compilare
    if (!sec->get("src", s, sizeof(s)))
    {
        logger->log(eTextColor::red, "line %d => can't find param <src>\n", sec->getLineStarted());
        return false;
    }
    prot_makeABSPath (absFilename, s, out_params->src, sizeof(out_params->src));




    /*param:def     e' opzionale e, se esiste, e' una sequenza di parole separate da spazio
                    che vengono passate come 'define' durante la compilazione dello shader.
                    Nel file ini le parole possono essere separate da piu' spazi o magari da tab. Quello che voglio io
                    qui e' un elenco di parole, trimmate e ordinate alfabeticamente in modo che 2 dichiarazioni apparentemente differenti tipo:
                        1- def: a     b c
                        2- def: a b c
                        3- def: b   a   c

                    risultino tutte uguali (ovvero lo stesso elenco di 3 parole: "a", "b", "c")
    */
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
            logger->log (eTextColor::red, "line %d => <%s> is not a valid param\n", sec->getLineStarted(), paramName);
            return false;
        }
    }


    return true;
}

//************************************
bool Builder_shader::build (DBContext &ctx, u64 buildTime_UTC, const char *absFilename, UID uid_of_iniFile, const gos::IniFileSection *sec, bool doCreateAnAssetFile, sBuildResult *out_result)
{
    assert (ctx.isValid());
    assert (NULL != sec);
    assert (NULL != out_result);

    out_result->reset();

    //parse della sezione
    Params params;
    if (!priv_extractParams(absFilename, sec, &params))
        return false;

    //il parametro src indica una risorsa eResType::shader_txt da cui io dipendo
    //La risorsa deve esistere nel DB. Se non c'e' gia', al devo inserire
    if (!prot_needResource (ctx, eResType::shader_txt, params.src, &params.uid__resource_shader_txt))
    {
        logger->log (eTextColor::red, "resource [%s] '%s' not found in DB\n", asset2::enumToString(eResType::shader_txt), params.src);
        return false;
    }

    //questo file gosasset_d dipende dalla risorsa params.uid__resource_shader_txt) che e' il src dello shader
    if (!asset2::dependency_exists(ctx, uid_of_iniFile, params.uid__resource_shader_txt))
    {
        if (!asset2::dependency_add (ctx, uid_of_iniFile, params.uid__resource_shader_txt)) 
            return false;  
    }

    //calcolo assetUID
    if (!asset2::asset_createUID (getAssetType(), &params, sizeof(Params), &out_result->uid_concrete_asset))
    {
        logger->log (eTextColor::red, "error generating concrete assetUID\n");
        return false;
    }


    //inserisco il virtual asset nel DB
    char rtname[128];
    memset (rtname, 0, sizeof(rtname));
    sec->get("__value", rtname, sizeof(rtname));
    if (!virtasset_insert (ctx, getAssetType(), rtname, uid_of_iniFile, sec->getLineStarted(), out_result->uid_concrete_asset, &out_result->uid_virtual_asset))
    {
        logger->log (eTextColor::red, "error inserting virtual assetUID\n");
        return false;
    }


    //vediamo se il concrete-asset esiste gia' nel DB
    if (asset2::asset_exists (ctx, out_result->uid_concrete_asset))
    {
        out_result->result = eBuildResult::was_already_built;
    }
    else
    {
        //non esisteva nel DB, ottimo, lo aggiungo e poi lo buildo
        out_result->result = eBuildResult::just_built;
        if (!asset_insert (ctx, out_result->uid_concrete_asset))
        {
            logger->log (eTextColor::red, "error inserting asset in DB\n");
            return false;
        }        
    }

    //aggiungo le sue dipendenze
    if (!dependency_add (ctx, out_result->uid_virtual_asset, uid_of_iniFile)) return false;        
    if (!dependency_add (ctx, out_result->uid_virtual_asset, out_result->uid_concrete_asset)) return false;
    if (!dependency_add (ctx, out_result->uid_virtual_asset, params.uid__resource_shader_txt)) return false;
    



    //a questo punto devo compilare lo shader per davvero
    if (doCreateAnAssetFile && eBuildResult::just_built == out_result->result)
    {
        char shaderStage[8];
        if (eAssetType::vtx_shader == getAssetType())
            sprintf_s (shaderStage, sizeof(shaderStage), "vert");
        else
            sprintf_s (shaderStage, sizeof(shaderStage), "frag");

        char filenameDST[1024];
        asset2::asset_manufacture_fullFilename (ctx, out_result->uid_concrete_asset, filenameDST, sizeof(filenameDST));

        //creo la versione ottimizzata e la versione con le debug-info. Quest'ultima
        //serve per esempio alle pipeline_def per recuprare i nomi e il formato dei descrittori
        if (!GPU::shader_compile (params.src, shaderStage, params.def, filenameDST, false))
            return false;

        strcat_s (filenameDST, sizeof(filenameDST), "d");
        if (!GPU::shader_compile (params.src, shaderStage, params.def, filenameDST, true))
            return false;

    }


    return true;
}
