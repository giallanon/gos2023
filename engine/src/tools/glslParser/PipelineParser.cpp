#include "PipelineParser.h"
#include "../../gos/gosIniFile.h"
#include "../../gos/gosUtils.h"
#include "../../gos/string/gosUTF8String.h"
#include "SPVReflect.h"

using namespace gos;


//************************************** 
PipelineParser::PipelineParser()
{
}

//************************************** 
PipelineParser::~PipelineParser()
{
}

//************************************** 
bool PipelineParser::createFromIniFile (const char *fname, PipelineDef *out)
{
    assert (NULL != out);

    gos::IniFile ini;
    if (!ini.loadAndParse (fname))
    {
        gos::logger::err ("PipelineParser::createFromIniFile() => error parsing file %s\n", fname);
        return false;
    }
    

    IniFileSection *sec = ini.getSubsection ("pipeline_def");
    if (NULL == sec)
    {
        gos::logger::err ("PipelineParser::createFromIniFile() => can't find section <def-pipeline>\n");
        return false;
    }

    char srcFolder[512];
    gos::fs::extractFilePathWithOutSlash (fname, srcFolder, sizeof(srcFolder));
    return priv_parseIniFileSection (srcFolder, *sec, out);
    return false;
}

//******************************** 
bool PipelineParser::priv_shader_compile (const char *shaderSRCFile, const char *shaderStage, const char *firstDefine, ...) const
{
    //se esistono delle define da passare al compilatore...
    char defineList[1024];
    memset (defineList, 0, sizeof(defineList));
    if (NULL != firstDefine)
    {
        va_list argptr;
        va_start (argptr, firstDefine);

        const char *def = firstDefine;
        while (1)
        {
            strcat_s (defineList, sizeof(defineList), "-D");
            strcat_s (defineList, sizeof(defineList), def);
            strcat_s (defineList, sizeof(defineList), " ");

            def = va_arg(argptr, const char *);
            if (NULL == def)
                break;
        }
        va_end(argptr);
    }

    //glslc -fshader-stage=vert --target-env=vulkan1.3 lineRenderer.vert.shader -g -O -o lineRenderer.vert.spv
    char cmd[1024];
    sprintf_s (cmd, sizeof(cmd), "glslc -fshader-stage=%s --target-env=vulkan1.3 %s %s -g -O -o %s.spv 2>&1",  shaderStage, defineList, shaderSRCFile, shaderSRCFile);

    char *result;
    u32 len;
    if (!gos::runShellScriptAndStoreResult (cmd, gos::getScrapAllocator(), &result, &len))
        return false;

    if (NULL == result)
        return true;

    //c'e' stato qualche errore di compilazione
    gos::logger::err("PipelineParser::priv_shader_compile, error compiling shader %s\n%s", shaderSRCFile, result);
    GOSFREE_SCRAP(result);
    return false;
}


//************************************** 
template<typename LAMBDA>
bool PipelineParser_priv_splitParams (gos::IniFileSection &sec, const char *paramName, u32 minNumExpectedParams, LAMBDA&& evalParamFn)
{
    char s[512];
    if (!sec.get (paramName, s, sizeof(s)))
    {
        gos::logger::err ("PipelineParser => param <%s> not found\n", paramName);
        return false;
    }

    u32 paramIndex = 0;
    char value[256];
    gos::string::utf8::StringListParser slp;
    slp.toStart (s, ',');
    while (slp.next (value, sizeof(value)))
    {
        if (!evalParamFn(paramName, paramIndex, value))
            return false;
        paramIndex++;
    }


    if (paramIndex < minNumExpectedParams)
    {
        gos::logger::err ("PipelineParser => not enough params for <%s>, %d found, at least %d expected\n", paramName, paramIndex, minNumExpectedParams);
        return false;
    }
  
    return true;
}

//**************************************
bool PipelineParser::priv_parseIniFileSection (const char *srcFolder, gos::IniFileSection &sec, PipelineDef *out)
{
    assert (NULL != out);
    out->setDefault();

    if (!sec.get("name", out->name, sizeof(out->name)))
    {
        gos::logger::err ("PipelineParser::priv_parseIniFileSection() => param <name> not found\n");
        return false;
    }

    //output_rt: sameAsSwapchain | <eImgFormat>, <finalLayout>, <loadOp>, <storeOp>, <optional_clearColor>
    PipelineParser_priv_splitParams (sec, "output_rt", 4, [out](const char *paramName, u32 paramIndex, const char *paramValue){
        switch (paramIndex)
        {
        default:
            gos::logger::err ("PipelineParser => error parsing <%s, too many parameters\n", paramName);
            return false;

        case 0:
            if (!utils::stringToEnum (paramValue, &out->outputRT_fmt))
            {
                gos::logger::err ("PipelineParser => error parsing <%s>, invalid param-value %d (eImgFormat): %s\n", paramName, paramIndex+1, paramValue);
                return false;
            }
            break;

        case 1:
            if (!utils::stringToEnum (paramValue, &out->outputRT_finalLayout))
            {
                gos::logger::err ("PipelineParser => error parsing <%s>, invalid param-value %d (eImageLayout): %s\n", paramName, paramIndex+1, paramValue);
                return false;
            }
            break;

        case 2:
            if (!utils::stringToEnum (paramValue, &out->outputRT_loadOp))
            {
                gos::logger::err ("PipelineParser => error parsing <%s>, invalid param-value %d (eAttachmentLoadOp): %s\n", paramName, paramIndex+1, paramValue);
                return false;
            }
            break;
            
        case 3:
            if (!utils::stringToEnum (paramValue, &out->outputRT_storeOp))
            {
                gos::logger::err ("PipelineParser => error parsing <%s>, invalid param-value %d (eAttachmentStoreOp): %s\n", paramName, paramIndex+1, paramValue);
                return false;
            }
            break;
            
        case 4: //option
            {
                ColorHDR col;
                if (col.setFromString (paramValue))
                    out->outputRT_clearCol_ARGB = col.toU32ARGB();
                else
                {
                    gos::logger::err ("PipelineParser => error parsing <%s>, invalid param-value %d (color): %s\n", paramName, paramIndex+1, paramValue);
                    return false;
                }
            }
            break;

        }
        return true;
    });

    
    //output_depthStencil: sameAsSwapchain | <eImgFormat>, <finalLayout>, <loadOp>, <storeOp>, <optional_zClearValue>, <optional_stencilClearValue>
    PipelineParser_priv_splitParams (sec, "output_depthStencil", 4, [out](const char *paramName, u32 paramIndex, const char *paramValue){
        switch (paramIndex)
        {
        default:
            gos::logger::err ("PipelineParser => error parsing <%s, too many parameters\n", paramName);
            return false;

        case 0:
            if (!utils::stringToEnum (paramValue, &out->outputDepth_fmt))
            {
                gos::logger::err ("PipelineParser => error parsing <%s>, invalid param-value %d (eImgFormat): %s\n", paramName, paramIndex+1, paramValue);
                return false;
            }
            break;

        case 1:
            if (!utils::stringToEnum (paramValue, &out->outputDepth_finalLayout))
            {
                gos::logger::err ("PipelineParser => error parsing <%s>, invalid param-value %d (eImageLayout): %s\n", paramName, paramIndex+1, paramValue);
                return false;
            }
            break;

        case 2:
            if (!utils::stringToEnum (paramValue, &out->outputDepth_loadOp))
            {
                gos::logger::err ("PipelineParser => error parsing <%s>, invalid param-value %d (eAttachmentLoadOp): %s\n", paramName, paramIndex+1, paramValue);
                return false;
            }
            break;
            
        case 3:
            if (!utils::stringToEnum (paramValue, &out->outputDepth_storeOp))
            {
                gos::logger::err ("PipelineParser => error parsing <%s>, invalid param-value %d (eAttachmentStoreOp): %s\n", paramName, paramIndex+1, paramValue);
                return false;
            }
            break;
            
        case 4: //optional
            out->outputDepth_zClearValue = gos::string::ansi::toF32(paramValue);
            if (out->outputDepth_zClearValue < 0 || out->outputDepth_zClearValue > 1.0f)
            {
                gos::logger::err ("PipelineParser => error parsing <%s>, invalid param-value %d (zcolor): %s\n", paramName, paramIndex+1, paramValue);
                return false;
            }
            break;

        case 5: //optional
            out->outputDepth_stencilClearValue = gos::string::ansi::toF32(paramValue);
            if (out->outputDepth_stencilClearValue < 0 || out->outputDepth_stencilClearValue > 1.0f)
            {
                gos::logger::err ("PipelineParser => error parsing <%s>, invalid param-value %d (stencil color): %s\n", paramName, paramIndex+1, paramValue);
                return false;
            }
            break;            

        }
        return true;
    });


    //Opzionale:
    //zOption: <enabled>, <write>, <cmpFN>
    if (sec.exists("zOption"))
    {
        PipelineParser_priv_splitParams (sec, "zOption", 3, [out](const char *paramName, u32 paramIndex, const char *paramValue){
            switch (paramIndex)
            {
            default:
                gos::logger::err ("PipelineParser => error parsing <%s, too many parameters\n", paramName);
                return false;

            case 0:
                if (!utils::stringIsTrueOrFalse (paramValue, &out->zbuffer_enabled))
                {
                    gos::logger::err ("PipelineParser => error parsing <%s>, invalid param-value %d (zbuffer_enabled): %s\n", paramName, paramIndex+1, paramValue);
                    return false;
                }
                break;

            case 1:
                if (!utils::stringIsTrueOrFalse (paramValue, &out->zbuffer_write))
                {
                    gos::logger::err ("PipelineParser => error parsing <%s>, invalid param-value %d (zbuffer_write): %s\n", paramName, paramIndex+1, paramValue);
                    return false;
                }
                break;

            case 2:
                if (!utils::stringToEnum (paramValue, &out->zbuffer_cmpFn))
                {
                    gos::logger::err ("PipelineParser => error parsing <%s>, invalid param-value %d (zbuffer cmpFN): %s\n", paramName, paramIndex+1, paramValue);
                    return false;
                }
                break;
            }
            return true;
        });
    }

    //Opzionale:
    //stencilOption: <enabled>, <cmpFN>
    if (sec.exists("stencilOption"))
    {    
        PipelineParser_priv_splitParams (sec, "stencilOption", 2, [out](const char *paramName, u32 paramIndex, const char *paramValue){
            switch (paramIndex)
            {
            default:
                gos::logger::err ("PipelineParser => error parsing <%s, too many parameters\n", paramName);
                return false;

            case 0:
                if (!utils::stringIsTrueOrFalse (paramValue, &out->stencil_enabled))
                {
                    gos::logger::err ("PipelineParser => error parsing <%s>, invalid param-value %d (stencil_enabled): %s\n", paramName, paramIndex+1, paramValue);
                    return false;
                }
                break;

            case 1:
                if (!utils::stringToEnum (paramValue, &out->stencil_cmpFn))
                {
                    gos::logger::err ("PipelineParser => error parsing <%s>, invalid param-value %d (stencil cmpFN): %s\n", paramName, paramIndex+1, paramValue);
                    return false;
                }
                break;
            }
            return true;
        });
    }

    //Opzionale:
    //cullMode: <fn>
    if (sec.exists("cullMode"))
    {    
        PipelineParser_priv_splitParams (sec, "cullMode", 1, [out](const char *paramName, u32 paramIndex, const char *paramValue){
            switch (paramIndex)
            {
            default:
                gos::logger::err ("PipelineParser => error parsing <%s, too many parameters\n", paramName);
                return false;

            case 0:
                if (!utils::stringToEnum (paramValue, &out->cullMode))
                {
                    gos::logger::err ("PipelineParser => error parsing <%s>, invalid param-value %d (fn): %s\n", paramName, paramIndex+1, paramValue);
                    return false;
                }
                break;
            }
            return true;
        });
    }
    
    //Opzionale:
    //drawPrimitive: <fn>
    if (sec.exists("drawPrimitive"))
    {    
        PipelineParser_priv_splitParams (sec, "drawPrimitive", 1, [out](const char *paramName, u32 paramIndex, const char *paramValue){
            switch (paramIndex)
            {
            default:
                gos::logger::err ("PipelineParser => error parsing <%s, too many parameters\n", paramName);
                return false;

            case 0:
                if (!utils::stringToEnum (paramValue, &out->drawPrimitive))
                {
                    gos::logger::err ("PipelineParser => error parsing <%s>, invalid param-value %d (fn): %s\n", paramName, paramIndex+1, paramValue);
                    return false;
                }
                break;
            }
            return true;
        });
    }


    //vtx shader (e' opzionale)
    char s[1024];
    char filename_vtxShader[1024];

    filename_vtxShader[0] = 0x00;
    if (sec.get("vtxShader", filename_vtxShader, sizeof(filename_vtxShader)))
    {
        if (strcmp(filename_vtxShader, "none") == 0)
            filename_vtxShader[0] = 0x00;
        else
        {
            sprintf_s (s, sizeof(s), "%s/%s", srcFolder, filename_vtxShader);
            fs::resolvePath (s, filename_vtxShader, sizeof(filename_vtxShader));
            if (!priv_shader_compile (filename_vtxShader, "vert"))
            {
                gos::logger::err ("PipelineParser => error compiling vtxshader %s\n", s);
                return false;
            }

            strcat_s (filename_vtxShader, sizeof(filename_vtxShader), ".spv");
        }
    }

    //frag shader (e' opzionale)
    char filename_pxlShader[1024];

    filename_pxlShader[0] = 0x00;
    if (sec.get("pxlShader", filename_pxlShader, sizeof(filename_pxlShader)))
    {
        if (strcmp(filename_pxlShader, "none") == 0)
            filename_pxlShader[0] = 0x00;
        else
        {
            sprintf_s (s, sizeof(s), "%s/%s", srcFolder, filename_pxlShader);
            fs::resolvePath (s, filename_pxlShader, sizeof(filename_pxlShader));
            if (!priv_shader_compile (filename_pxlShader, "frag"))
            {
                gos::logger::err ("PipelineParser => error compiling pxlshader %s\n", s);
                return false;
            }

            strcat_s (filename_pxlShader, sizeof(filename_pxlShader), ".spv");
        }
    }    
 
    SPVReflect reflect;
    if (!reflect.parseFromFile (filename_vtxShader, filename_pxlShader))
    {
        gos::logger::err ("PipelineParser => error 'reflecting' shaders %s\n", s);
        return false;
    }

    return true;
}