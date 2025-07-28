#include "PipelineParser.h"
#include "../../gos/gosIniFile.h"
#include "../../gos/gosUtils.h"
#include "../../gos/string/gosUTF8String.h"

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
bool PipelineParser::parseFromFile (const char *fname, PipelineDef *out)
{
    assert (NULL != out);

    u32 fsize = 0;
    u8 *buffer = fs::fileLoadInMemory (gos::getScrapAllocator(), fname, &fsize);
    if (NULL == buffer)
    {
        gos::logger::err ("PipelineParser::parseFromFile() => file not found %s\n", fname);
        return false;
    }

    const bool ret = parseFromMemory (buffer, fsize, out);
    GOSFREE(gos::getScrapAllocator(), buffer);
    return ret;
}

//************************************** 
bool PipelineParser::parseFromMemory (const u8 *buffer, u32 sizeof_buffer, PipelineDef *out)
{
    assert (NULL != out);

    if (NULL == buffer || 0 == sizeof_buffer)
    {
        gos::logger::err ("PipelineParser::parseFromMemory() => invalid file size %d\n", sizeof_buffer);
        return false;
    }

    gos::IniFile ini;
    if (!ini.parseFromMemory (buffer, sizeof_buffer))
    {
        gos::logger::err ("PipelineParser::parseFromMemory() => error parsing file\n");
        return false;
    }
    

    IniFileSection *sec = ini.getSubsection ("pipeline_def");
    if (NULL == sec)
    {
        gos::logger::err ("PipelineParser::parseFromMemory() => can't find section <def-pipeline>\n");
        return false;
    }
    return parse_PipelineDef (*sec, out);

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
bool PipelineParser::parse_PipelineDef (gos::IniFileSection &sec, PipelineDef *out)
{
    assert (NULL != out);
    out->setDefault();

    if (!sec.get("name", out->name, sizeof(out->name)))
    {
        gos::logger::err ("PipelineParser::parse_PipelineDef() => <name> not found\n");
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
    //rawPrimitive: <fn>
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

    return true;
}