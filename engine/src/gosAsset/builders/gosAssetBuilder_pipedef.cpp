#include "gos.h"
#include "../gosAssetBuilder.h"
#include "gosAssetBuilder_pipedef.h"
#include "gosAssetBuilder_shader.h"


using namespace gos;
using namespace gos::asset;


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
bool Builder_pipeDef::build (Context &ctx, u64 buildTimeUTC, const IniFileSection *sec, sBuildResult *out)
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


    //calcolo assetUID
    if (!asset::asset_createUID (getAssType(), &params, sizeof(Params), &out->uid))
    {
        gos::logger::err ("error generating assetUID\n");
        return false;
    }

    //idealmente asset::UID non dovrebbe esistere in tabella visto che lo sto buildando
    //C'e' pero' la possibilita' che durante il buildAll, lo stesso asset venga buildata + di una
    //volta.
    //Voglio evitare questo fatto
    const u64 lastTimeBuilt = asset::asset_query_lastTimeBuilt (ctx, out->uid);
    if (lastTimeBuilt >= buildTimeUTC)
    {
        out->result = eBuildResult::was_already_built;
        return true;
    }


    //lo registro in tabella
    if (!asset::asset_insert (ctx, out->uid, getAssType(), buildTimeUTC))
    {
        gos::logger::err ("error inserting asset\n");
        return false;
    }

    out->result = eBuildResult::just_built;
    return true;
}
