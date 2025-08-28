#include "gos.h"
#include "../gosAssetBuilder.h"
#include "gosAssetBuilder_shader.h"


using namespace gos;
using namespace gos::asset;


//********************************************
bool Builder_shader::shader_compile (const char *shaderSRCFile, const char *shaderStage, const char *spaceSeparateDefineList, const char *shaderDSTFile, bool bIncludeDebugInfo)
{
    //se esistono delle define da passare al compilatore...
    char defineList[2048];
    memset (defineList, 0, sizeof(defineList));
    if (NULL != spaceSeparateDefineList)
    {
        string::utf8::StringListParser parser;
        parser.toStart (spaceSeparateDefineList, ' ');
        
        char def[256];
        while (parser.next (def, sizeof(def)))
        {
            strcat_s (defineList, sizeof(defineList), "-D");
            strcat_s (defineList, sizeof(defineList), def);
            strcat_s (defineList, sizeof(defineList), " ");
        }
    }

    char opt_includeDebugInfo[4];
    if (bIncludeDebugInfo)
        sprintf_s (opt_includeDebugInfo, sizeof(opt_includeDebugInfo), "-g");
    else
        opt_includeDebugInfo[0] = 0x00;

    //glslc -fshader-stage=vert --target-env=vulkan1.3 lineRenderer.vert.shader -g -O -o lineRenderer.vert.spv
    char cmd[1024];
    sprintf_s (cmd, sizeof(cmd), "glslc -fshader-stage=%s --target-env=vulkan1.3 %s %s %s -O -o %s 2>&1",  shaderStage, defineList, shaderSRCFile, opt_includeDebugInfo, shaderDSTFile);
    gos::logger::log ("%s\n", cmd);

    char *result;
    u32 len;
    if (!gos::runShellScriptAndStoreResult (cmd, gos::getScrapAllocator(), &result, &len))
        return false;

    if (NULL == result)
        return true;

    //c'e' stato qualche errore di compilazione
    gos::logger::err ("ERR => %s\n", result);
    GOSFREE_SCRAP(result);
    return false;
}

//************************************
bool Builder_shader::extractParams (const IniFileSection *sec, Params *out_params)
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

    //param:define      e' opzionale e, se esiste, e' una sequenza di parole separate da spazio
    //                  che vengono passate come 'define' durante la compilazione dello shader
    sec->get ("define", out_params->define, sizeof(out_params->define));

    return true;
}

//************************************
bool Builder_shader::build (Context &ctx, u64 buildTimeUTC, const IniFileSection *sec, sBuildResult *out)
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
