#include "gos.h"
#include "../gosAsset2Builder.h"
#include "gosAsset2Builder_shader.h"
#include "gosGPU.h"


using namespace gos;
using namespace gos::asset2;

//************************************
bool Builder_shader::priv_extractParams (DBContext &ctx, const UniqueUIDList &listof_UID_of_known_ini_file, const char *absFilename)
{
    char s[1024];
    memset (&params, 0, sizeof(Params));

    //param:src         e' mandatorio ed indica il filename dello shader in formato testo da compilare
    if (!sec->get("src", s, sizeof(s)))
    {
        logger->log(eTextColor::red, "line %d => can't find param <src>\n", sec->getLineStarted());
        return false;
    }
    if (!asset2::Builder::makeABSPathFromFilename (ctx, logger, listof_UID_of_known_ini_file, absFilename, s, params.src, sizeof(params.src)))
        return false;




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

        u32 n = list.getNElem();
        assert (n>0);
        sprintf_s (params.def, sizeof(params.def), "%s", list(0).getBuffer());
        for (u32 i=1; i<n; i++)
        {
            strcat_s (params.def, sizeof(params.def), " ");
            strcat_s (params.def, sizeof(params.def), list(i).getBuffer());
        }

#ifdef _DEBUG
        //per lo meno nella versione WINDOWS, la sprintf_s in versione DEBUG riempe params.def di 0xFE, probabilmente per detectare
        //i buffer overflow. Il fatto di avere degli 0xFE al posto dei normali 0x00 che ci dovrebbero essere, altera il calcolo dell'asset UID visto
        //che il buffer che fornisco a prot_setupVirtualAsset() e' diverso nella versione debug rispetto alla versione release.
        //Per fixare la cosa, riempo di 0x00 la parte non usata di params.def
        n = (u32)strlen(params.def);
        memset (&params.def[n], 0x00, sizeof(params.def)-n);
#endif
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
bool Builder_shader::build_begin (DBContext &ctx, const UniqueUIDList &listof_UID_of_known_ini_file, const char *absFilename, UID uid_of_iniFileIN, const gos::IniFileSection *secIN)
{
    assert (ctx.isValid());
    assert (NULL != secIN);
    
	//mi salvo alcune info per dopo (fn build_exec)
	uid_of_iniFile = uid_of_iniFileIN;
	sec = secIN;

    //parse della sezione
    if (!priv_extractParams(ctx, listof_UID_of_known_ini_file, absFilename))
        return false;

    //il parametro src indica una risorsa eResType::shader_txt da cui io dipendo
    //La risorsa deve esistere nel DB. Se non c'e' gia', al devo inserire
    if (!prot_needResource (ctx, listof_UID_of_known_ini_file, eResType::shader_txt, params.src, &params.uid__resource_shader_txt))
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

	return true;
}

//************************************
bool Builder_shader::build_exe (DBContext &ctx, bool doCreateAnAssetFile, bool *out_bCallMeAgain, sBuildResult *out_result)
{
	assert (NULL != out_result);
	assert (NULL != out_bCallMeAgain);
	*out_bCallMeAgain = false;
	out_result->reset();


    //setup di virtual-asset
    //All'uscita da questa fn:
    //  out_result->uid_virtual_asset       contiene l'UID di questo virtual asset, gia' inserito nel DB
    //  out_result->uid_concrete_asset      contiene l'UID dell'asset concreto a cui questo virtual-asset punta
    //  out_result->result                  vale <eBuildResult::just_built> se e' necessario creare fisicamente il concrete-asset, altrimenti vale <eBuildResult::was_already_built>
    if (!prot_setupVirtualAsset (ctx, &params, sizeof(Params), uid_of_iniFile, sec, out_result))
        return false;

    //aggiungo le dipendenze di virtual-asset dalla risorsa shader_txt
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