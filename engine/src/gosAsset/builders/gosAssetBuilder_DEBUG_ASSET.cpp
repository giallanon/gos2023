#include "gos.h"
#include "../gosAssetBuilder.h"
#include "gosAssetBuilder_DEBUG_ASSET.h"
#include "gosAssetBuilder_pipe.h"
#include "gosAssetBuilder_shader.h"



using namespace gos;
using namespace gos::asset;

//************************************
u32 Builder_DEBUG_ASSET::calc_depth()
{
    static constexpr u32 BASE_DEPTH = 1; //1 perche' io dipendo da almeno un altro asset
    u32 depth = 0;
    u32 d;

    //dipendo da pipline_def
    d = BASE_DEPTH + asset::Builder_pipe::calc_depth();    if (d > depth) depth = d;

    //dipendo da vtx shader
    d = BASE_DEPTH + asset::Builder_vtxShader::calc_depth();    if (d > depth) depth = d;

    return depth;
}

//************************************
bool Builder_DEBUG_ASSET::priv_extractParams (const IniFileSection *sec, Params *out_params)
{
    assert (NULL != sec);
    assert (NULL != out_params);
    
    //setto i default
    memset (out_params, 0, sizeof(Params));
    return true;
}

//************************************
bool Builder_DEBUG_ASSET::build (Context &ctx, u64 buildTimeUTC, const char *sourceFileInfo, const asset::UID &uid_of_iniFile, const IniFileSection *sec, bool doCreateAnAssetFile, gos::GPU *gpu, sBuildResult *out)
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

    //devo avere una subsection di tipo @vtx_shader, gia' risolta
    if (!prot_needResolvedSubsection (ctx, sec, eAssetType::vtx_shader, &params.uid_vtxshader))
    {
        gos::logger::err ("section [vtx_shader] is error or missing\n");
        return false;
    }

    //devo avere una subsection di tipo @pipeline_def, gia' risolta
    if (!prot_needResolvedSubsection (ctx, sec, eAssetType::pipe, &params.uid_pipedef))
    {
        gos::logger::err ("section [pipe] is error or missing\n");
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

    if (!asset::depend_add (ctx, out->uid, params.uid_vtxshader)) return false;
    if (!asset::depend_add (ctx, out->uid, params.uid_pipedef)) return false;

    //segno che e' stato buildato di fresco
    out->result = eBuildResult::just_built;

    
    //a questo punto devo davvero creare il file dell'asset
    if (doCreateAnAssetFile)
    {
        char filenameDST[1024];
        asset::asset_manufacture_fullFilename (ctx, out->uid, filenameDST, sizeof(filenameDST));
        return priv_do_create_assetFile (ctx, params, filenameDST);
    }

    return true;
}


//************************************
bool Builder_DEBUG_ASSET::priv_do_create_assetFile (Context &ctx, const Params &params, const char *filenameDST) const
{
    return true;
}