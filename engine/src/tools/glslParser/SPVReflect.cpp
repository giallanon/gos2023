#include "SPVReflect.h"
#include "gosUtils.h"

using namespace gos;

//***************************************************
SPVReflect::SPVReflect()
{
    reset();
}

//***************************************************
SPVReflect::~SPVReflect()
{
}

//***************************************************
bool SPVReflect::priv_SpvReflectFormat_to_eDataFormat (SpvReflectFormat fmtIN, eDataFormat *out_fmt) const
{
    bool ret = true;
    switch (fmtIN)
    {
    default:
        ret = false;
        *out_fmt = eDataFormat::_1f32;
        break;
    
    case SPV_REFLECT_FORMAT_R32_SFLOAT:             *out_fmt = eDataFormat::_1f32; break;
    case SPV_REFLECT_FORMAT_R32G32_SFLOAT:          *out_fmt = eDataFormat::_2f32; break;
    case SPV_REFLECT_FORMAT_R32G32B32_SFLOAT:       *out_fmt = eDataFormat::_3f32; break;
    case SPV_REFLECT_FORMAT_R32G32B32A32_SFLOAT:    *out_fmt = eDataFormat::_4f32; break;

    case SPV_REFLECT_FORMAT_R32_UINT:               *out_fmt = eDataFormat::_1u32; break;
    case SPV_REFLECT_FORMAT_R32G32_UINT:            *out_fmt = eDataFormat::_2u32; break;
    case SPV_REFLECT_FORMAT_R32G32B32_UINT:         *out_fmt = eDataFormat::_3u32; break;
    case SPV_REFLECT_FORMAT_R32G32B32A32_UINT:      *out_fmt = eDataFormat::_4u32; break;

    case SPV_REFLECT_FORMAT_R32_SINT:               *out_fmt = eDataFormat::_1i32; break;
    case SPV_REFLECT_FORMAT_R32G32_SINT:            *out_fmt = eDataFormat::_2i32; break;
    case SPV_REFLECT_FORMAT_R32G32B32_SINT:         *out_fmt = eDataFormat::_3i32; break;
    case SPV_REFLECT_FORMAT_R32G32B32A32_SINT:      *out_fmt = eDataFormat::_4i32; break;
    }

    return ret;
}

//***************************************************
void SPVReflect::reset()
{
    vtxDeclList.reset();
    pushConstantList.reset();
    descrSetList.reset();
}


//***************************************************
bool SPVReflect::parseFromFile (const char *vtxShaderFilename, const char *fragShaderFilename)
{
    reset();

    bool ret = true;
    u32 fsize;
    
    if (NULL != vtxShaderFilename)
    {
        u8 *buffer = gos::fs::fileLoadInMemory (gos::getScrapAllocator(), vtxShaderFilename, &fsize);
        if (NULL == buffer)
        {
            logger::err ("SPVReflect::loadAndParse() => can't load %s\n", vtxShaderFilename);
            return false;
        }
        ret = VS_parseFromMemory (buffer, fsize);
        GOSFREE_SCRAP(buffer);
    }

    if (!ret)
        return false;

    if (NULL != fragShaderFilename)
    {
        u8 *buffer = gos::fs::fileLoadInMemory (gos::getScrapAllocator(), fragShaderFilename, &fsize);
        if (NULL == buffer)
        {
            logger::err ("SPVReflect::loadAndParse() => can't load %s\n", fragShaderFilename);
            return false;
        }
        ret = PS_parseFromMemory (buffer, fsize);
        GOSFREE_SCRAP(buffer);
    }
    return ret;
}

//***************************************************
bool SPVReflect::VS_parseFromMemory (const u8 *buffer, u32 bufferSize)
{
    SpvReflectShaderModule module;
    SpvReflectResult result = spvReflectCreateShaderModule (bufferSize, buffer, &module);
    if (result != SPV_REFLECT_RESULT_SUCCESS)
    {
        gos::logger::err ("SPVReflect::VS_parseFromMemory() => error parsing shader, errcode=%d\n", result);
        return false;
    }

    bool ret = false;
    if (SPV_REFLECT_SHADER_STAGE_VERTEX_BIT == module.shader_stage)
    {
        ret = priv_parse_vtxShader (&module);
    }
    else
    {
        gos::logger::err ("SPVReflect::VS_parseFromMemory() => not a vertex shader!\n");
    }

    spvReflectDestroyShaderModule (&module);
    return ret;
}

//***************************************************
bool SPVReflect::priv_parse_vtxShader (SpvReflectShaderModule *module)
{
    if (!priv_parse_vtxShader_vtxDecl(module))
        return false;
    if (!priv_parse_pushConstant(module))
        return false;
    if (!priv_parse_descriptors(module))
        return false;
    return true;
}

//***************************************************
bool SPVReflect::priv_parse_vtxShader_vtxDecl (SpvReflectShaderModule *module)
{
    u32 n = 0;
    SpvReflectResult result = spvReflectEnumerateInputVariables (module, &n, NULL);
    if (SPV_REFLECT_RESULT_SUCCESS != result)
    {
        gos::logger::err ("SPVReflect::priv_parse_vtxShader_vtxDecl() => error <spvReflectEnumerateInputVariables>, %d\n", result);
        return false;
    }
    
    SpvReflectInterfaceVariable **vars = (SpvReflectInterfaceVariable**) malloc (n * sizeof(SpvReflectInterfaceVariable*));
    result = spvReflectEnumerateInputVariables (module, &n, vars);
    if (SPV_REFLECT_RESULT_SUCCESS == result)
    {
        for (u32 i=0; i<n; i++)
        {
            if (vars[i]->built_in == -1)
            {
                VtxDeclElem e;

                sprintf_s (e.name, sizeof(e.name), "%s", vars[i]->name);
                e.bindingLocation = vars[i]->location;
                priv_SpvReflectFormat_to_eDataFormat (vars[i]->format, &e.fmt);
                vtxDeclList.add(e);
            }
        }
    }
    free (vars);
    vtxDeclList.sort();
    return true;
}

//***************************************************
bool SPVReflect::priv_parse_pushConstant (SpvReflectShaderModule *module)
{
    u32 n = 0;
    SpvReflectResult result = spvReflectEnumeratePushConstantBlocks (module, &n, NULL);
    if (SPV_REFLECT_RESULT_SUCCESS != result)
    {
        gos::logger::err ("SPVReflect::priv_parse_vtxShader_pushConstant() => error <spvReflectEnumeratePushConstantBlocks>, %d\n", result);
        return false;
    }

    //n dovrebbe essere sempre == 0 oppure == 1 in quanto n al massimo e' una struct
    //con un certo numero di membri, non puo' mai essere una singola variabile fuori da struct
    if (n > 0)
    {
        if (n > 1)
        {
            gos::logger::err ("SPVReflect::priv_parse_vtxShader_pushConstant() => error n is >1\n");
            return false;
        }

        SpvReflectBlockVariable **vars = (SpvReflectBlockVariable**) malloc (n * sizeof(SpvReflectBlockVariable*));
        result = spvReflectEnumeratePushConstantBlocks (module, &n, vars);
        if (SPV_REFLECT_RESULT_SUCCESS == result)
        {
            for (u32 i=0; i<n; i++)
            {
                for (u32 i2=0; i2<vars[i]->member_count; i2++)
                {
                    const SpvReflectBlockVariable *info = &vars[i]->members[i2];

                    PushConstantElem e;
                    if ( (info->flags & SPV_REFLECT_VARIABLE_FLAGS_UNUSED) == 0)
                    {
                        if (SPV_REFLECT_SHADER_STAGE_VERTEX_BIT == module->shader_stage)
                            e.flag |= PushConstantElem::FLAG__USED_IN_VTX_SHADER;
                        if (SPV_REFLECT_SHADER_STAGE_FRAGMENT_BIT == module->shader_stage)
                            e.flag |= PushConstantElem::FLAG__USED_IN_FRAG_SHADER;
                    }
                                        sprintf_s (e.name, sizeof(e.name), "%s.%s", vars[i]->name, info->name);
                    e.offset = info->offset;
                    e.absoluteOffset = info->absolute_offset;
                    e.size = vars[i]->members[i2].size;
                    e.paddedSize = info->padded_size;

                    //informazioni sul tipo della variabile
                    const SpvReflectTypeDescription *typeInfo = info->type_description;

                    bool bSigned = false;
                    if (typeInfo->traits.numeric.scalar.signedness)
                        bSigned = true;

                    eDataFormat_type basicType = eDataFormat_type::_8bit;
                    if ((typeInfo->type_flags & SPV_REFLECT_TYPE_FLAG_BOOL) != 0)
                        basicType = eDataFormat_type::_8bit;
                    else if ((typeInfo->type_flags & SPV_REFLECT_TYPE_FLAG_INT) != 0)
                        basicType = eDataFormat_type::_32bit;
                    else if ((typeInfo->type_flags & SPV_REFLECT_TYPE_FLAG_FLOAT) != 0)
                    {
                        basicType = eDataFormat_type::_f32;
                        bSigned = true;
                    }


                    u8 numRow = 0;
                    u8 numCol = 0;
                    if ((typeInfo->type_flags & SPV_REFLECT_TYPE_FLAG_MATRIX) != 0)
                    {
                        numRow = typeInfo->traits.numeric.matrix.row_count;
                        numCol = typeInfo->traits.numeric.matrix.column_count;
                    }
                    else if ((typeInfo->type_flags & SPV_REFLECT_TYPE_FLAG_VECTOR) != 0)
                    {
                        numCol = typeInfo->traits.numeric.vector.component_count;
                    }
                    else if ((typeInfo->type_flags & SPV_REFLECT_TYPE_FLAG_ARRAY) != 0)
                    {
                        numCol = typeInfo->traits.array.dims[0];
                    }
                    
                    else
                        numCol = 1;

                    e.fmt = gos::dataformat::build (basicType, bSigned, numRow, numCol);
                    pushConstantList.add (e);
                }
            }
        }
        free (vars);
    }

    pushConstantList.sort();
    return true;
}

//***************************************************
bool SPVReflect::PS_parseFromMemory (const u8 *buffer, u32 bufferSize)
{
    SpvReflectShaderModule module;
    SpvReflectResult result = spvReflectCreateShaderModule (bufferSize, buffer, &module);
    if (result != SPV_REFLECT_RESULT_SUCCESS)
    {
        gos::logger::err ("SPVReflect::PS_parseFromMemory() => error parsing shader, errcode=%d\n", result);
        return false;
    }

    bool ret = false;
    if (SPV_REFLECT_SHADER_STAGE_FRAGMENT_BIT == module.shader_stage)
    {
        ret = priv_parse_fragShader (&module);
    }
    else
    {
        gos::logger::err ("SPVReflect::PS_parseFromMemory() => not a fragment shader!\n");
    }

    spvReflectDestroyShaderModule (&module);
    return ret;
}

//***************************************************
bool SPVReflect::priv_parse_fragShader (SpvReflectShaderModule *module)
{
    if (!priv_parse_pushConstant (module))
        return false;
    if (!priv_parse_descriptors(module))
        return false;
    return true;
}

//***************************************************
bool SPVReflect::priv_parse_descriptors (SpvReflectShaderModule *module)
{
    u32 n = 0;
    SpvReflectResult result = spvReflectEnumerateDescriptorBindings (module, &n, NULL);
    if (SPV_REFLECT_RESULT_SUCCESS != result)
    {
        gos::logger::err ("SPVReflect::priv_parse_descriptors() => error <spvReflectEnumerateDescriptorBindings>, %d\n", result);
        return false;
    }
    
    SpvReflectDescriptorBinding **vars = (SpvReflectDescriptorBinding**) malloc (n * sizeof(SpvReflectDescriptorBinding*));
    result = spvReflectEnumerateDescriptorBindings (module, &n, vars);
    if (SPV_REFLECT_RESULT_SUCCESS == result)
    {
        for (u32 i=0; i<n; i++)
        {
            DescrSetElem e;
            sprintf_s (e.name, sizeof(e.name), "%s", vars[i]->name);
            e.set = vars[i]->set;
            e.binding = vars[i]->binding;
            if (0 != vars[i]->accessed)
            {
                if (SPV_REFLECT_SHADER_STAGE_VERTEX_BIT == module->shader_stage)
                    e.flag |= DescrSetElem::FLAG__USED_IN_VTX_SHADER;
                if (SPV_REFLECT_SHADER_STAGE_FRAGMENT_BIT == module->shader_stage)
                    e.flag |= DescrSetElem::FLAG__USED_IN_FRAG_SHADER;
            }

            descrSetList.add (e);
        }
    }
    free (vars);

    descrSetList.sort();
    return true;
}

//***************************************************
void SPVReflect::printInfo() const
{
    logger::log ("SPVReflect::printInfo()\n");
    logger::incIndent();

    logger::log ("Vertex declaration:\n");
    {
        logger::incIndent();
        if (0 == vtxDeclList.getNElem())
            logger::log ("no info!\n");
        else
        {
            for (u32 i=0; i<vtxDeclList.getNElem(); i++)
            {
                logger::log ("bindingLoc:% 2d, name:% -32s, fmt=% -10s, size=%d\n", 
                        vtxDeclList(i).bindingLocation,
                        vtxDeclList(i).name,
                        utils::enumToString (vtxDeclList(i).fmt),
                        dataformat::getSize (vtxDeclList(i).fmt));
            }
        }
        logger::decIndent();
    }

    logger::log ("\nPush constant:\n");
    {
        logger::incIndent();
        if (0 == pushConstantList.getNElem())
            logger::log ("no info!\n");
        else
        {
            for (u32 i=0; i<pushConstantList.getNElem(); i++)
            {
                char stage[32];

                memset (stage, 0, sizeof(stage));

                if (pushConstantList(i).flag & PushConstantElem::FLAG__USED_IN_VTX_SHADER)
                    strcat_s (stage, sizeof(stage), "VTX ");
                if (pushConstantList(i).flag & PushConstantElem::FLAG__USED_IN_FRAG_SHADER)
                    strcat_s (stage, sizeof(stage), "FRG ");

                logger::log ("[% -12s] name:% -32s, fmt=% -10s, size=% -3d (paddedSize=% -3d), offset=% -3d (absOffset=%d)\n", 
                        stage, 
                        pushConstantList(i).name,
                        utils::enumToString (pushConstantList(i).fmt),
                        pushConstantList(i).size, pushConstantList(i).paddedSize,
                        pushConstantList(i).offset, pushConstantList(i).absoluteOffset);
            }
        }
        logger::decIndent();
    }

    logger::log ("\nDescriptor sets:\n");
    {
        logger::incIndent();
        if (0 == descrSetList.getNElem())
            logger::log ("no info!\n");
        else
        {
            for (u32 i=0; i<descrSetList.getNElem(); i++)
            {
                char stage[32];

                memset (stage, 0, sizeof(stage));

                if (descrSetList(i).flag & DescrSetElem::FLAG__USED_IN_VTX_SHADER)
                    strcat_s (stage, sizeof(stage), "VTX ");
                if (descrSetList(i).flag & DescrSetElem::FLAG__USED_IN_FRAG_SHADER)
                    strcat_s (stage, sizeof(stage), "FRG ");

                logger::log ("[% -12s] name:% -32s (set=%d, binding=%d)\n", 
                        stage, 
                        descrSetList(i).name,
                        descrSetList(i).set, descrSetList(i).binding);
            }
        }
        logger::decIndent();
    }    

    logger::decIndent();
}



