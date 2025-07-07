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
const char* SPVReflect::enumToString (eDescriptrorType s)
{
    switch (s)
    {
    default: return "eDescriptrorType::invalid value";
    case eDescriptrorType::SAMPLER: return "SAMPLER";
    case eDescriptrorType::COMBINED_IMAGE_SAMPLER: return "COMBINED_IMAGE_SAMPLER";
    case eDescriptrorType::TEXTURE2D: return "TEXTURE2D";
    case eDescriptrorType::STORAGE_IMAGE: return "STORAGE_IMAGE";
    case eDescriptrorType::UNIFORM_TEXEL_BUFFER: return "UNIFORM_TEXEL_BUFFER";
    case eDescriptrorType::STORAGE_TEXEL_BUFFER: return "STORAGE_TEXEL_BUFFER";
    case eDescriptrorType::UNIFORM_BUFFER: return "UNIFORM_BUFFER";
    case eDescriptrorType::STORAGE_BUFFER: return "STORAGE_BUFFER";
    case eDescriptrorType::DYNAMIC_UNIFORM_BUFFER: return "DYNAMIC_UNIFORM_BUFFER";
    case eDescriptrorType::DYNAMIC_STORAGE_BUFFER: return "DYNAMIC_STORAGE_BUFFER";
    case eDescriptrorType::INPUT_ATTACHMENT: return "INPUT_ATTACHMENT";
    case eDescriptrorType::UNKNOWN: return "UNKNOWN";
    }
}

//***************************************************
const char* SPVReflect::enumToString (eResourceType s)
{
    switch (s)
    {
    default: return "eResourceType::invalid value";
    case eResourceType::_struct: return "struct";
    case eResourceType::_array: return "array";
    case eResourceType::_dynamicArray: return "dynamicArray";
    }
}

//***************************************************
eDataFormat SPVReflect::priv_fromSPVReflectTypeDescrToDataFormat (const SpvReflectTypeDescription *strTypeDescr) const
{
    bool bSigned = false;
    if (strTypeDescr->traits.numeric.scalar.signedness)
        bSigned = true;

    eDataFormat_type basicType = eDataFormat_type::_8bit;

    SpvReflectTypeFlags srcFlags = strTypeDescr->type_flags;
    if ((srcFlags & SPV_REFLECT_TYPE_FLAG_BOOL) != 0)
        basicType = eDataFormat_type::_8bit;
    else if ((srcFlags & SPV_REFLECT_TYPE_FLAG_INT) != 0)
        basicType = eDataFormat_type::_32bit;
    else if ((srcFlags & SPV_REFLECT_TYPE_FLAG_FLOAT) != 0)
    {
        basicType = eDataFormat_type::_f32;
        bSigned = true;
    }


    u8 numRow = 0;
    u8 numCol = 0;
    if ((srcFlags & SPV_REFLECT_TYPE_FLAG_MATRIX) != 0)
    {
        numRow = strTypeDescr->traits.numeric.matrix.row_count;
        numCol = strTypeDescr->traits.numeric.matrix.column_count;
    }
    else if ((srcFlags & SPV_REFLECT_TYPE_FLAG_VECTOR) != 0)
    {
        numCol = strTypeDescr->traits.numeric.vector.component_count;
    }
    else if ((srcFlags & SPV_REFLECT_TYPE_FLAG_ARRAY) != 0)
    {
        numCol = strTypeDescr->traits.array.dims[0];
    }
    else
        numCol = 1;

    return gos::dataformat::build (basicType, bSigned, numRow, numCol);
}

//***************************************************
void SPVReflect::reset()
{
    vtxDeclList.reset();
    pushConstantList.reset();
    descrSetList.reset();
}


//***************************************************
bool SPVReflect::parseFromFile (const char *vtxShaderFilename, const char *fragShaderFilename, gos::Logger *loggerIN)
{
    if (NULL == loggerIN)
        loggerIN = gos::logger::getSystemLogger();

    reset();

    bool ret = true;
    u32 fsize;
    
    if (NULL != vtxShaderFilename)
    {
        u8 *buffer = gos::fs::fileLoadInMemory (gos::getScrapAllocator(), vtxShaderFilename, &fsize);
        if (NULL == buffer)
        {
            loggerIN->err ("SPVReflect::loadAndParse() => can't load %s\n", vtxShaderFilename);
            return false;
        }
        ret = VS_parseFromMemory (buffer, fsize, loggerIN);
        GOSFREE_SCRAP(buffer);
    }

    if (!ret)
        return false;

    if (NULL != fragShaderFilename)
    {
        u8 *buffer = gos::fs::fileLoadInMemory (gos::getScrapAllocator(), fragShaderFilename, &fsize);
        if (NULL == buffer)
        {
            loggerIN->err ("SPVReflect::loadAndParse() => can't load %s\n", fragShaderFilename);
            return false;
        }
        ret = PS_parseFromMemory (buffer, fsize, loggerIN);
        GOSFREE_SCRAP(buffer);
    }
    return ret;
}

//***************************************************
bool SPVReflect::VS_parseFromMemory (const u8 *buffer, u32 bufferSize, gos::Logger *loggerIN)
{
    if (NULL == loggerIN)
        loggerIN = gos::logger::getSystemLogger();

    SpvReflectShaderModule module;
    SpvReflectResult result = spvReflectCreateShaderModule (bufferSize, buffer, &module);
    if (result != SPV_REFLECT_RESULT_SUCCESS)
    {
        loggerIN->err ("SPVReflect::VS_parseFromMemory() => error parsing shader, errcode=%d\n", result);
        return false;
    }

    bool ret = false;
    if (SPV_REFLECT_SHADER_STAGE_VERTEX_BIT == module.shader_stage)
    {
        ret = priv_parse_vtxShader (&module, loggerIN);
    }
    else
    {
        loggerIN->err ("SPVReflect::VS_parseFromMemory() => not a vertex shader!\n");
    }

    spvReflectDestroyShaderModule (&module);
    return ret;
}

//***************************************************
bool SPVReflect::priv_parse_vtxShader (SpvReflectShaderModule *module, gos::Logger *loggerIN)
{
    assert (NULL != loggerIN);

    if (!priv_parse_vtxShader_vtxDecl(module, loggerIN))
        return false;
    if (!priv_parse_pushConstant(module, loggerIN))
        return false;
    if (!priv_parse_descriptors(module, loggerIN))
        return false;
    return true;
}

//***************************************************
bool SPVReflect::priv_parse_vtxShader_vtxDecl (SpvReflectShaderModule *module, gos::Logger *loggerIN)
{
    assert (NULL != loggerIN);

    u32 n = 0;
    SpvReflectResult result = spvReflectEnumerateInputVariables (module, &n, NULL);
    if (SPV_REFLECT_RESULT_SUCCESS != result)
    {
        loggerIN->err ("SPVReflect::priv_parse_vtxShader_vtxDecl() => error <spvReflectEnumerateInputVariables>, %d\n", result);
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
bool SPVReflect::priv_parse_pushConstant (SpvReflectShaderModule *module, gos::Logger *loggerIN)
{
    assert (NULL != loggerIN);

    u32 n = 0;
    SpvReflectResult result = spvReflectEnumeratePushConstantBlocks (module, &n, NULL);
    if (SPV_REFLECT_RESULT_SUCCESS != result)
    {
        loggerIN->err ("SPVReflect::priv_parse_vtxShader_pushConstant() => error <spvReflectEnumeratePushConstantBlocks>, %d\n", result);
        return false;
    }

    //n dovrebbe essere sempre == 0 oppure == 1 in quanto n al massimo e' una struct
    //con un certo numero di membri, non puo' mai essere una singola variabile fuori da struct
    if (n > 0)
    {
        if (n > 1)
        {
            loggerIN->err ("SPVReflect::priv_parse_vtxShader_pushConstant() => error n is >1\n");
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
                    e.fmt = priv_fromSPVReflectTypeDescrToDataFormat (info->type_description);
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
bool SPVReflect::PS_parseFromMemory (const u8 *buffer, u32 bufferSize, gos::Logger *loggerIN)
{
    if (NULL == loggerIN)
        loggerIN = gos::logger::getSystemLogger();

    SpvReflectShaderModule module;
    SpvReflectResult result = spvReflectCreateShaderModule (bufferSize, buffer, &module);
    if (result != SPV_REFLECT_RESULT_SUCCESS)
    {
        loggerIN->err ("SPVReflect::PS_parseFromMemory() => error parsing shader, errcode=%d\n", result);
        return false;
    }

    bool ret = false;
    if (SPV_REFLECT_SHADER_STAGE_FRAGMENT_BIT == module.shader_stage)
    {
        ret = priv_parse_fragShader (&module, loggerIN);
    }
    else
    {
        loggerIN->err ("SPVReflect::PS_parseFromMemory() => not a fragment shader!\n");
    }

    spvReflectDestroyShaderModule (&module);
    return ret;
}

//***************************************************
bool SPVReflect::priv_parse_fragShader (SpvReflectShaderModule *module, gos::Logger *loggerIN)
{
    assert (NULL != loggerIN);

    if (!priv_parse_pushConstant (module, loggerIN))
        return false;
    if (!priv_parse_descriptors(module, loggerIN))
        return false;
    return true;
}

//***************************************************
bool SPVReflect::priv_parse_descriptors (SpvReflectShaderModule *module, gos::Logger *loggerIN)
{
    assert (NULL != loggerIN);

    u32 n = 0;
    SpvReflectResult result = spvReflectEnumerateDescriptorBindings (module, &n, NULL);
    if (SPV_REFLECT_RESULT_SUCCESS != result)
    {
        loggerIN->err ("SPVReflect::priv_parse_descriptors() => error <spvReflectEnumerateDescriptorBindings>, %d\n", result);
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

            //tipo (vulkan) di descriptor
            switch (vars[i]->descriptor_type)
            {
            default:
                loggerIN->err ("SPVReflect::priv_parse_descriptors() => error <descriptor_type> invalid, %d\n", (int)vars[i]->descriptor_type);
                return false;

            case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER: e.vulkanDescrType = eDescriptrorType::SAMPLER; break;
            case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER: e.vulkanDescrType = eDescriptrorType::COMBINED_IMAGE_SAMPLER; break;
            case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE: e.vulkanDescrType = eDescriptrorType::TEXTURE2D; break;
            case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE: e.vulkanDescrType = eDescriptrorType::STORAGE_IMAGE; break;
            case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER: e.vulkanDescrType = eDescriptrorType::UNIFORM_TEXEL_BUFFER; break;
            case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER: e.vulkanDescrType = eDescriptrorType::STORAGE_TEXEL_BUFFER; break;
            case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER: e.vulkanDescrType = eDescriptrorType::UNIFORM_BUFFER; break;
            case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER: e.vulkanDescrType = eDescriptrorType::STORAGE_BUFFER; break;
            case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC: e.vulkanDescrType = eDescriptrorType::DYNAMIC_UNIFORM_BUFFER; break;
            case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC: e.vulkanDescrType = eDescriptrorType::DYNAMIC_STORAGE_BUFFER; break;
            case SPV_REFLECT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT: e.vulkanDescrType = eDescriptrorType::INPUT_ATTACHMENT; break;
            }

            switch (vars[i]->type_description->op)
            {
            default:
                loggerIN->err ("SPVReflect::priv_parse_descriptors() => error <type_description> invalid, %d\n", (int)vars[i]->descriptor_type);
                return false;

            case SpvOpTypeArray:
                {
                    e.resType.type = eResourceType::_array;
                    e.resType.info.asArray.ordine = vars[i]->array.dims_count;
                    for (u8 t = 0; t < e.resType.info.asArray.ordine; t++)
                    {
                        e.resType.info.asArray.numElem[t] = vars[i]->array.dims[t];
                    }
                }
                break;

            case SpvOpTypeRuntimeArray:
                e.resType.type = eResourceType::_dynamicArray;
                e.resType.info.asDynArray.ordine = vars[i]->array.dims_count;
                break;

            case SpvOpTypeStruct:
                {
                    e.resType.type = eResourceType::_struct;
                    e.resType.info.asStruct.numElem = vars[i]->type_description->member_count;
                    for (u8 t = 0; t < e.resType.info.asStruct.numElem; t++)
                    {
                        sprintf_s (e.resType.info.asStruct.name[t], sizeof(e.resType.info.asStruct.name[t]), "%s", vars[i]->type_description->members[t].struct_member_name);
                        e.resType.info.asStruct.fmt[t] = priv_fromSPVReflectTypeDescrToDataFormat(&vars[i]->type_description->members[t]);
                    }
                }
                break;

            }

            descrSetList.add (e);
        }
    }
    free (vars);

    descrSetList.sort();
    return true;
}

//***************************************************
void SPVReflect::printInfo(gos::Logger *loggerIN) const
{
    if (NULL == loggerIN)
        loggerIN = gos::logger::getSystemLogger();

    loggerIN->log ("SPVReflect::printInfo()\n");
    loggerIN->incIndent();

    loggerIN->log ("Vertex declaration:\n");
    {
        loggerIN->incIndent();
        if (0 == vtxDeclList.getNElem())
            loggerIN->log ("no info!\n");
        else
        {
            for (u32 i=0; i<vtxDeclList.getNElem(); i++)
            {
                loggerIN->log ("bindingLoc:% 2d, name:% -32s, fmt=% -10s, size=%d\n", 
                        vtxDeclList(i).bindingLocation,
                        vtxDeclList(i).name,
                        utils::enumToString (vtxDeclList(i).fmt),
                        dataformat::getSize (vtxDeclList(i).fmt)
                );
            }
        }
        loggerIN->decIndent();
    }

    loggerIN->log ("\nPush constant:\n");
    {
        loggerIN->incIndent();
        if (0 == pushConstantList.getNElem())
            loggerIN->log ("no info!\n");
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

                loggerIN->log ("[% -12s] name:% -32s, fmt=% -10s, size=% -3d (paddedSize=% -3d), offset=% -3d (absOffset=%d)\n", 
                        stage, 
                        pushConstantList(i).name,
                        utils::enumToString (pushConstantList(i).fmt),
                        pushConstantList(i).size, pushConstantList(i).paddedSize,
                        pushConstantList(i).offset, pushConstantList(i).absoluteOffset);
            }
        }
        loggerIN->decIndent();
    }

    loggerIN->log ("\nDescriptor sets:\n");
    {
        loggerIN->incIndent();
        if (0 == descrSetList.getNElem())
            loggerIN->log ("no info!\n");
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

                const sResInfo *resInfo = &descrSetList(i).resType;
                loggerIN->log ("[% -12s] name:% -32s (set=%d, binding=%d), VKtype=%s, data-type:%s", 
                        stage, 
                        descrSetList(i).name,
                        descrSetList(i).set, descrSetList(i).binding,
                        enumToString (descrSetList(i).vulkanDescrType),
                        enumToString (resInfo->type)
                        );

                
                switch (resInfo->type)
                {
                default: 
                    loggerIN->log (", ERR");
                    break;
                
                case eResourceType::_array:
                    {
                        loggerIN->log (", %s", descrSetList(i).name);
                        for (u8 t = 0; t < resInfo->info.asArray.ordine; t++)
                        {
                            loggerIN->log ("[%d]", resInfo->info.asArray.numElem[t]);
                        }
                    }
                    break;

                case eResourceType::_dynamicArray:
                    {
                        loggerIN->log (", %s", descrSetList(i).name);
                        for (u8 t = 0; t < resInfo->info.asArray.ordine; t++)
                        {
                            loggerIN->log ("[]");
                        }
                    }
                    break;
                
                case eResourceType::_struct:
                    for (u8 t = 0; t < resInfo->info.asStruct.numElem; t++)
                    {
                        const eDataFormat fmt = resInfo->info.asStruct.fmt[t];
                        loggerIN->log ("\n     % -16s, fmt=% -10s, size=%d",
                            resInfo->info.asStruct.name[t],
                            utils::enumToString (fmt), gos::dataformat::getSize(fmt));
                    }
                    break;

                }
                loggerIN->log ("\n");
            }
        }
        loggerIN->decIndent();
    }    

    loggerIN->decIndent();
}



