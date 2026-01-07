#include "gos.h"
#include "../gos/gosString.h"
#include "../gosAsset2Builder.h"
#include "../gos/gosMagicUID.h"
#include "gosAsset2Builder_pipe.h"
#include "gosAsset2Builder_shader.h"
#include "../shader_reflect/SPVReflect.h"



using namespace gos;
using namespace gos::asset2;

//************************************
bool Builder_pipe::priv_extractParams (const IniFileSection *sec, Params *out_params)
{
    assert (NULL != sec);
    assert (NULL != out_params);
    
    //setto i default
    memset (out_params, 0, sizeof(Params));
    out_params->magic = GOS_MAGIC__ASSET_PIPELINE_DEF;
    out_params->cullMode = eCullMode::CCW;
    out_params->drawPrimitive = eDrawPrimitive::trisList;

    out_params->zbuffer_enabled = true;
    out_params->zbuffer_format = eImageFormat::_DEPTH_BEST;
    out_params->zbuffer_write = true;
    out_params->zbuffer_cmpFn = eZFunc::LESS;

    //parse dell'ini
    string::utf8::StringListParser stringParser;

    //param: cullMode
    char s[1024];
    char s2[256];
    if (sec->get("cullMode", s, sizeof(s)))
    {
        if (!gos::utils::stringToEnum (s, &out_params->cullMode))
        {
            logger::err ("Builder_pipe::extractParams => invalid option '%s' for <cullMode>\n", s);
            return false;
        }
    }

    //param: drawPrimitive
    if (sec->get("drawPrimitive", s, sizeof(s)))
    {
        if (!gos::utils::stringToEnum (s, &out_params->drawPrimitive))
        {
            logger::err ("Builder_pipe::extractParams => invalid option '%s' for <drawPrimitive>\n", s);
            return false;
        }
    }

    //param: wireframe
    if (sec->get("wireframe", s, sizeof(s)))
    {
        if (string::utf8::toI32(s) != 0)
            out_params->bWireframe = 1;
    }

    //param: zb
    if (sec->get("zb", s, sizeof(s)))
    {
        stringParser.toStart (s, ",");
        if (!stringParser.next(s2, sizeof(s2)))
        {
            logger::err ("Builder_pipe::extractParams => invalid option '%s' for <zb>\n", s);
            return false;
        }
        if (0 == strcasecmp(s2, "none"))
        {
            out_params->zbuffer_enabled = false;
        }
        else
        {
            //3 parametri: <imgFormat = BEST | ...>, <zwrite = 0|1>, <zcmpFn = LESS|...>
            if (!utils::stringToEnum(s2, &out_params->zbuffer_format))
            {
                logger::err ("Builder_pipe::extractParams => invalid option(1) '%s' for <zb>\n", s);
                return false;
            }

            //zwrite
            if (!stringParser.next(s2, sizeof(s2)))
            {
                logger::err ("Builder_pipe::extractParams => invalid option(2) '%s' for <zb>\n", s);
                return false;
            }
            if (string::utf8::toI32(s2) == 0)
                out_params->zbuffer_write = false;

            //cmpFn
            if (!stringParser.next(s2, sizeof(s2)))
            {
                logger::err ("Builder_pipe::extractParams => invalid option(3) '%s' for <zb>\n", s);
                return false;
            }
            if (!utils::stringToEnum(s2, &out_params->zbuffer_cmpFn))
            {
                logger::err ("Builder_pipe::extractParams => invalid option(3) '%s' for <zb>\n", s);
                return false;
            }
        }
    }

    //render target
    for (u32 i = 0; i < GOSGPU__NUM_MAX_ATTACHMENT; i++)
    {
        sprintf_s (s, sizeof(s), "rt%d", i);
        if (!sec->get (s, s2, sizeof(s2)))
            break;

        if (!utils::stringToEnum (s2, &out_params->renderTargetFormat[out_params->numRT]))
        {
            logger::err ("Builder_pipe::extractParams => invalid option '%s' for <rt[%d]>\n", s2, i);
            return false;
        }
        out_params->numRT++;
    }



    //per evitare problemi, controllo che ci siano solo ed esattamente i parametri che mi aspetto
    for (u32 i=0; i<sec->getNIdentifier(); i++)
    {
        const char *paramName = sec->getIdentifierByIndex(i);
        if (!prot_isOneOfThis(paramName, "rt0", "rt1", "rt2", "rt3", "rt4", "rt5", "rt6", "rt7", "rt8", "rt9", "rt10", "rt11", "rt12",
                "rt13", "rt14", "rt15", "zb", "cullMode", "drawPrimitive", "wireframe", NULL))
        {
            gos::logger::err ("Builder_shader::extractParams => <%s> is not a valid one\n", paramName);
            return false;
        }
    }

    return true;
}

//************************************
bool Builder_pipe::build (DBContext &ctx, u64 buildTime_UTC, const char *absFilename, UID uid_of_iniFile, const gos::IniFileSection *sec, bool doCreateAnAssetFile, sBuildResult *out_result)
{
    assert (ctx.isValid());
    assert (NULL != sec);
    assert (NULL != out_result);

    out_result->reset();

    //parse della sezione
    Params params;
    if (!priv_extractParams(sec, &params))
    {
        gos::logger::err ("error parsing IniFileSection\n");
        return false;
    }

    //devo avere una subsection di tipo @vtx_shader, gia' risolta
    if (!prot_needResolvedSubsection (ctx, sec, eAssetType::vtx_shader, &params.uid__virtual_vtxshader))
    {
        gos::logger::err ("section [vtx_shader] is error or missing\n");
        return false;
    }

    //devo avere una subsection di tipo @pxl_shader, gia' risolta
    if (!prot_needResolvedSubsection (ctx, sec, eAssetType::pxl_shader, &params.uid__virtual_pxlshader))
    {
        gos::logger::err ("section [pxl_shader] is error or missing\n");
        return false;
    }


    //setup di virtual-asset
    //All'uscita da questa fn:
    //  out_result->uid_virtual_asset       contiene l'UID di questo virtual asset, gia' inserito nel DB
    //  out_result->uid_concrete_asset      contiene l'UID dell'asset concreto a cui questo virtual-asset punta
    //  out_result->result                  vale <eBuildResult::just_built> se e' necessario creare fisicamente il concrete-asset, altrimenti vale <eBuildResult::was_already_built>
    if (!prot_setupVirtualAsset (ctx, &params, sizeof(Params), uid_of_iniFile, sec, out_result))
        return false;



    //aggiungo le dipendenze di virtual-asset dai virtual-asset degli shader
    if (!dependency_add (ctx, out_result->uid_virtual_asset, params.uid__virtual_vtxshader)) return false;
    if (!dependency_add (ctx, out_result->uid_virtual_asset, params.uid__virtual_pxlshader)) return false;


    //l'asset concreto dipende dagli asseti concreti di vtx/pxl shader
    if (params.uid__virtual_vtxshader.isValid())
    {
        UID uid__concrete_vtxShader;
        if (!virtasset_get_info (ctx, params.uid__virtual_vtxshader, NULL, &uid__concrete_vtxShader))   return false;
        if (!dependencyRT_add (ctx, out_result->uid_concrete_asset, uid__concrete_vtxShader)) return false;
    }
    if (params.uid__virtual_pxlshader.isValid())
    {
        UID uid__concrete_pxlShader;
        if (!virtasset_get_info (ctx, params.uid__virtual_pxlshader, NULL, &uid__concrete_pxlShader))   return false;
        if (!dependencyRT_add (ctx, out_result->uid_concrete_asset, uid__concrete_pxlShader)) return false;
    }       

    
    //a questo punto devo davvero creare il file dell'asset
    if (doCreateAnAssetFile && eBuildResult::just_built == out_result->result)
    {
        char filenameDST[1024];
        asset_manufacture_fullFilename (ctx, out_result->uid_concrete_asset, filenameDST, sizeof(filenameDST));
        return priv_do_create_assetFile (ctx, out_result->uid_concrete_asset, params, filenameDST);
    }

    return true;
}


//************************************
bool Builder_pipe::priv_do_create_assetFile (DBContext &ctx, UID uid_concrete_asset, const Params &params, const char *filenameDST) const
{
    SPVReflect reflect;
    reflect.beginParseFromMemory();

    //il vtx/pxl shader esistono gia' e sono gia' stati compilati.
    //A me pero' serve la versione con le debug info
    char s[1024];
    UID uid__concrete_vtxShader;
    uid__concrete_vtxShader.setInvalid();
    if (params.uid__virtual_vtxshader.isValid())
    {
        if (!virtasset_get_info (ctx, params.uid__virtual_vtxshader, NULL, &uid__concrete_vtxShader))
            return false;

        asset_manufacture_fullFilename (ctx, uid__concrete_vtxShader, s, sizeof(s));
        strcat_s (s, sizeof(s), "d");

        u32 fsize = 0;
        u8 *buffer = fs::fileLoadInMemory (gos::getScrapAllocator(), s, &fsize);
        if (NULL == buffer)
        {
            logger::err ("Builder_pipe::priv_do_create_assetFile => can't read file %s\n", s);
            return false;
        }
        if (!reflect.VS_parseFromMemory (buffer, fsize))
        {
            logger::err ("Builder_pipe::priv_do_create_assetFile => error parsing (reflect) VS file %s\n", s);
            return false;
        }
        GOSFREE_SCRAP(buffer);
    }

    UID uid__concrete_pxlShader;
    uid__concrete_pxlShader.setInvalid();
    if (params.uid__virtual_pxlshader.isValid())
    {
        if (!virtasset_get_info (ctx, params.uid__virtual_pxlshader, NULL, &uid__concrete_pxlShader))
            return false;

        asset_manufacture_fullFilename (ctx, uid__concrete_pxlShader, s, sizeof(s));
        strcat_s (s, sizeof(s), "d");

        u32 fsize = 0;
        u8 *buffer = fs::fileLoadInMemory (gos::getScrapAllocator(), s, &fsize);
        if (NULL == buffer)
        {
            logger::err ("Builder_pipe::priv_do_create_assetFile => can't read file %s\n", s);
            return false;
        }
        if (!reflect.PS_parseFromMemory (buffer, fsize))
        {
            logger::err ("Builder_pipe::priv_do_create_assetFile => error parsing (reflect) PS file %s\n", s);
            return false;
        }
        GOSFREE_SCRAP(buffer);
    }

    if (!reflect.endParseFromMemory())
    {
        logger::err ("Builder_pipe::priv_do_create_assetFile => error parsing (reflect), 'reflect.endParseFromMemory()'\n");
        return false;
    }

    //ora che ho le info recuperati dagli shader, posso creare tutto quel che mi serve
    u8 stackBuffer[2048];
    {
        gos::BufferW_linear buffer;
        buffer.setupWithBase (stackBuffer, sizeof(stackBuffer), gos::getScrapAllocator(), eEndianess::big);

        //magic
        buffer.writeU32 (GOS_MAGIC__ASSET_PIPELINE_DEF);

        //uid vtx shader
        buffer.writeU64 (uid__concrete_vtxShader._uid);

        //uid pxl shader
        buffer.writeU64 (uid__concrete_pxlShader._uid);

        //cull/draw
        buffer.writeU8 (static_cast<u8>(params.cullMode));
        buffer.writeU8 (static_cast<u8>(params.drawPrimitive));
        buffer.writeU8 (static_cast<u8>(params.bWireframe));

        //zbuffer
        buffer.writeU8 (static_cast<u8>(params.zbuffer_enabled));
        buffer.writeU8 (static_cast<u8>(params.zbuffer_format));
        buffer.writeU8 (static_cast<u8>(params.zbuffer_write));
        buffer.writeU8 (static_cast<u8>(params.zbuffer_cmpFn)); 

        //render target
        buffer.writeU32 (params.numRT);
        for (u32 i=0; i<params.numRT; i++)
            buffer.writeU8 (static_cast<u8>(params.renderTargetFormat[i]));

        //vtx declaration
        buffer.writeU32 (reflect.vtxdecl_getNumElem());
        for (u32 i = 0; i < reflect.vtxdecl_getNumElem(); i++)
        {
            u8 binding;
            u32 offset;
            eDataFormat fmt;
            reflect.vtxdecl_getElemByIndex(i, &binding, &offset, &fmt);
            buffer.writeU8 (binding);
            buffer.writeU32 (offset);
            buffer.writeU8 (static_cast<u8>(fmt)); 
        }


        //push constant
        {
            const u8 *pushconst = reflect.pushconst_getDataBlobDef();
            const u32 buffer_pos = buffer.tell();

            u32 numPushConstant = 0;
            buffer.writeU32 (numPushConstant);
            if (NULL != pushconst)
            {
                //devo "linearizzare" il dataBlob
                gos::datablob::DefReader dblobReader;
                dblobReader.setup (pushconst);

                datablob::DefElem elem;
                dblobReader.beginEnumerate(&elem);
                numPushConstant = priv_writePushConstant_rec (buffer, elem);
            }

            buffer.writeU32At (buffer_pos, numPushConstant);
        }

        //descriptor set
        const u32 numSet = reflect.descrset_getNumSet();
        buffer.writeU32 (numSet);
        for (u32 i = 0; i < numSet; i++)
        {
            const u32 setOptions = reflect.descrset_getOptionsPerSet(i).asU32();
            buffer.writeU32 (setOptions);

            const u32 numElem = reflect.descrset_getNumElemPerSet (i);
            buffer.writeU32 (numElem);
            for (u32 i2 = 0; i2 < numElem; i2++)
            {
                u8 binding;
                u32 count;
                eGPUDescriptrorType type;
                eGPUDescriptrorUsageBitmask usage;
                reflect.descrset_getElemByIndex (i, i2, &binding, &type, &count, &usage);

                buffer.writeU8 (static_cast<u8>(binding)); 
                buffer.writeU8 (static_cast<u8>(type)); 
                buffer.writeU32 (count);
                buffer.writeU32 (usage.bitmask);
            }
        }

        //salvo il report di reflect (per debug)
        {
            gos::UTF8String out;
            out.prealloc (1024);
            reflect.printInfo (out);

            char s[1024];
            sprintf_s (s, sizeof(s), "%s.reflect", filenameDST);
            fs::fileSaveBuffer (s, out.getBuffer(), out.lengthInByte());
        }

        //salvo il file asset
        return fs::fileSaveBuffer (filenameDST, stackBuffer, buffer.tell());
    }
}

//************************************
u32 Builder_pipe::priv_writePushConstant_rec (gos::BufferW_linear &buffer, datablob::DefElem &elem) const
{
    u32 ret = 0;

    do
    {
        switch (elem.getType())
        {
        default:
            //TODO
            DBGBREAK;
            break;

        case eDataBlobElemType::simpleType:
            {
                eShaderTypeBitmask shaderTypeList;
                if ((elem.getUserData() & 0x01) != 0)
                    shaderTypeList |= eShaderType::vtxShader;

                if ((elem.getUserData() & 0x02) != 0)
                    shaderTypeList |= eShaderType::pxlShader;

                buffer.writeU32 (elem.getOffset());
                buffer.writeU32 (elem.getPaddedSize());
                buffer.writeU32 (static_cast<u32>(shaderTypeList.bitmask));
                ret++;
            }
            break;

        case eDataBlobElemType::structType:
            {
                datablob::DefElem elemChild;
                if (elem.getFirstChild(&elemChild))
                    ret += priv_writePushConstant_rec (buffer, elemChild);
            }
            break;

        case eDataBlobElemType::arrayType:
            //TODO
            DBGBREAK;
            break;

        }
    } while (elem.next());
    return ret;
}