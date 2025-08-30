#include "gos.h"
#include "../gosAssetBuilder.h"
#include "gosAssetBuilder_pipedef.h"
#include "gosAssetBuilder_shader.h"


using namespace gos;
using namespace gos::asset;

//************************************
u32 Builder_pipeDef::calc_depth()
{
    static constexpr u32 BASE_DEPTH = 1; //1 perche' io dipendo da almeno un altro asset
    u32 depth = 0;
    u32 d;

    //dipendo da vtx shader
    d = BASE_DEPTH + asset::Builder_vtxShader::calc_depth();    if (d > depth) depth = d;

    //dipendo da pxl shader
    d = BASE_DEPTH + asset::Builder_pxlShader::calc_depth();    if (d > depth) depth = d;

    return depth;
}

//************************************
bool Builder_pipeDef::extractParams (const IniFileSection *sec, Params *out_params)
{
    assert (NULL != sec);
    assert (NULL != out_params);
    
    memset (out_params, 0, sizeof(Params));

    //param:param1          e' mandatorio 
    if (!sec->get("param1", out_params->param1, sizeof(out_params->param1)))
    {
        logger::err ("asset::Builder_pipeDef::extractParams => can't find param <param1>\n");
        return false;
    }

    //param:param2          e' mandatorio 
    if (!sec->get("param2", out_params->param2, sizeof(out_params->param2)))
    {
        logger::err ("asset::Builder_pipeDef::extractParams => can't find param <param2>\n");
        return false;
    }

    return true;
}

//************************************
bool Builder_pipeDef::build (Context &ctx, u64 buildTimeUTC, const char *sourceFileInfo, const asset::UID &uid_of_iniFile, const IniFileSection *sec, bool doCreateAnAssetFile, sBuildResult *out)
{
    assert (ctx.isValid());
    assert (NULL != sec);
    assert (NULL != out);

    out->reset();

    //parse della sezione
    Params params;
    if (!extractParams(sec, &params))
    {
        gos::logger::err ("error parsing IniFileSection\n");
        return false;
    }

    //devo avere una subsection di tipo @vtx_shader, gia' risolta
    if (!prot_needResolvedSubsection (ctx, sec, eAssetType::vtx_shader, &params.uid_vtxshader))
    {
        gos::logger::err ("section [vtx_shader] is error or missing\n");
        return false;
    }

    //devo avere una subsection di tipo @pxl_shader, gia' risolta
    if (!prot_needResolvedSubsection (ctx, sec, eAssetType::pxl_shader, &params.uid_pxlshader))
    {
        gos::logger::err ("section [pxl_shader] is error or missing\n");
        return false;
    }


    //calcolo assetUID
    if (!asset::asset_createUID (getAssType(), &params, sizeof(Params), &out->uid))
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
    if (!asset::depend_add (ctx, out->uid, params.uid_vtxshader)) return false;
    if (!asset::depend_add (ctx, out->uid, params.uid_pxlshader)) return false;

    //segno che e' stato buildato di fresco
    out->result = eBuildResult::just_built;

    
    //a questo punto devo davvero creare il file dell'asset
    if (doCreateAnAssetFile)
        return priv_do_create_assetFile (ctx, params);

    return true;
}


//************************************
bool Builder_pipeDef::priv_do_create_assetFile (Context &ctx, const Params &params) const
{
    //il vtx/pxl shader esistono gia' e sono gia' stati compilati.
    //A me serva la versione con le debug info
    char s[1024];
    asset::asset_manufacture_fullFilename (ctx, params.uid_vtxshader, s, sizeof(s));
    strcat_s (s, sizeof(s), "_d");


    return true;
}