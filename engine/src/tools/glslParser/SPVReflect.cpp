#include "SPVReflect.h"
#include "gosUtils.h"

using namespace gos;

gos::Allocator *SPVReflect::PushConstantNode::localAllocator = NULL;


//***************************************************
SPVReflect::SPVReflect()
{
    localAllocator = gos::getSysHeapAllocator();
    SPVReflect::PushConstantNode::localAllocator = localAllocator;

    pushConstant_VS = pushConstant_PS = pushConstant_merged = NULL;
    pushConstant_dataBlobDef = NULL;
}

//***************************************************
SPVReflect::~SPVReflect()
{
    priv_free();
}

//***************************************************
void SPVReflect::priv_free()
{
    PushConstantNode::deleteTree (pushConstant_VS);
    PushConstantNode::deleteTree (pushConstant_PS);
    pushConstant_VS = pushConstant_PS = pushConstant_merged = NULL;

    if (NULL != pushConstant_dataBlobDef)
    {
        GOSFREE(localAllocator, pushConstant_dataBlobDef);
        pushConstant_dataBlobDef = NULL;
    }
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
        //numCol = strTypeDescr->traits.array.dims[0];
        numCol = 1;
    }
    else
        numCol = 1;

    return gos::dataformat::build (basicType, bSigned, numRow, numCol);
}

//***************************************************
bool SPVReflect::parseFromFile (const char *vtxShaderFilename, const char *fragShaderFilename)
{
    beginParseFromMemory();

    bool ret = true;
    u32 fsize;
    
    if (NULL != vtxShaderFilename)
    {
        u8 *buffer = gos::fs::fileLoadInMemory (gos::getScrapAllocator(), vtxShaderFilename, &fsize);
        if (NULL == buffer)
        {
            gos::logger::err ("SPVReflect::loadAndParse() => can't load %s\n", vtxShaderFilename);
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
            gos::logger::err ("SPVReflect::loadAndParse() => can't load %s\n", fragShaderFilename);
            return false;
        }
        ret = PS_parseFromMemory (buffer, fsize);
        GOSFREE_SCRAP(buffer);
    }
    if (!ret)
        return false;
    
    return endParseFromMemory();
}

 //***************************************************
void SPVReflect::beginParseFromMemory()
{
    vtxDeclList.reset();
    descrSetList.reset();
    priv_free();
}

 //***************************************************
bool SPVReflect::endParseFromMemory()
{
    //UTF8String out;
    //out << "VS\n"; priv_pushConst_printNode (out, pushConstant_VS, 0);
    //out << "\nPS\n"; priv_pushConst_printNode (out, pushConstant_PS, 0);

    //se necessario, faccio il merge delle push constant di VS e PS
    pushConstant_merged = NULL;
    if (NULL == pushConstant_VS)
        pushConstant_merged = pushConstant_PS;
    else
    {
        pushConstant_merged = pushConstant_VS;
        if (NULL != pushConstant_PS)
            priv_pushConst_merge(pushConstant_merged, pushConstant_PS);
    }
    priv_pushConst_adjustArrayOffset (pushConstant_merged);
    priv_pushConst_adjustPaddedSize (pushConstant_merged);

    //out << "\nMERGED\n"; priv_pushConst_printNode (out, pushConstant_merged, 0);
    //printf ("%s", out.getBuffer());

    //Creo la blobDef per le push constant
    pushConstant_dataBlobDef = priv_pushConst_createGosDataBlobDef(localAllocator, pushConstant_merged);

    return true;
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
    
    pushConstant_VS = priv_pushConst_parseModule(module);
    if (gos::err::anyError())
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
    pushConstant_PS = priv_pushConst_parseModule (module);
    if (gos::err::anyError())
        return false;

    if (!priv_parse_descriptors(module))
        return false;
    return true;
}

//***************************************************
SPVReflect::PushConstantNode* SPVReflect::priv_pushConst_parseVar (const SpvReflectShaderModule *module, const SpvReflectBlockVariable *var, const char *parentName)
{
    PushConstantNode *node = PushConstantNode::createNew();
    sprintf_s (node->name, sizeof(node->name), "%s", var->name);

    if ( (var->flags & SPV_REFLECT_VARIABLE_FLAGS_UNUSED) == 0)
    {
        if (SPV_REFLECT_SHADER_STAGE_VERTEX_BIT == module->shader_stage)
            node->flag |= PushConstantNode::FLAG__USED_IN_VTX_SHADER;
        if (SPV_REFLECT_SHADER_STAGE_FRAGMENT_BIT == module->shader_stage)
            node->flag |= PushConstantNode::FLAG__USED_IN_FRAG_SHADER;
    }

    node->offset = var->offset;
    node->absoluteOffset = var->absolute_offset;
    //node->size = var->
    node->paddedSize = var->padded_size;
    
    switch (var->type_description->op)
    {
    case SpvOpTypeStruct:
        node->flag |= PushConstantNode::FLAG__IS_STRUCT;
        node->other.asStruct.numMembers = var->member_count;
        break;

    case SpvOpTypeArray:
        node->flag |= PushConstantNode::FLAG__IS_ARRAY;
        node->other.asArray.numDimension = static_cast<u8>(var->array.dims_count);
        node->other.asArray.sizeOfOneElem = static_cast<u16>(var->array.stride);
        {
            for (u8 i3=0; i3<node->other.asArray.numDimension; i3++)
            {
                node->other.asArray.numElem[i3] = var->array.dims[i3];
            }
        }

        if (0 == var->member_count)
            node->fmt = priv_fromSPVReflectTypeDescrToDataFormat (var->type_description);        
        break;

    default:
        //informazioni sul tipo della variabile
        node->fmt = priv_fromSPVReflectTypeDescrToDataFormat (var->type_description);
        break;
    }

    //compongo il mio full name
    if (NULL != parentName)
        sprintf_s (node->fullName, sizeof(node->fullName), "%s.%s", parentName, node->name);
    else
        sprintf_s (node->fullName, sizeof(node->fullName), "%s", node->name);

    
    for (u32 i=0; i<var->member_count; i++)
    {
        const SpvReflectBlockVariable *info = &var->members[i];
        PushConstantNode *figlio = priv_pushConst_parseVar(module, info, node->fullName);
        node->appendChild (figlio);

        if ((figlio->flag & PushConstantNode::FLAG__USED_IN_VTX_SHADER) != 0)
            node->flag |= PushConstantNode::FLAG__USED_IN_VTX_SHADER;
        if ((figlio->flag & PushConstantNode::FLAG__USED_IN_FRAG_SHADER) != 0)
            node->flag |= PushConstantNode::FLAG__USED_IN_FRAG_SHADER;
    }
    
    return node;
}

//***************************************************
SPVReflect::PushConstantNode* SPVReflect::priv_pushConst_parseModule (SpvReflectShaderModule *module)
{
    PushConstantNode *root = NULL;

    u32 n = 0;
    SpvReflectResult result = spvReflectEnumeratePushConstantBlocks (module, &n, NULL);
    if (SPV_REFLECT_RESULT_SUCCESS != result)
    {
        gos::logger::err ("SPVReflect::priv_parse_vtxShader_pushConstant() => error <spvReflectEnumeratePushConstantBlocks>, %d\n", result);
        return NULL;
    }

    //n dovrebbe essere sempre == 0 oppure == 1 in quanto n al massimo e' una struct
    //con un certo numero di membri, non puo' mai essere una singola variabile fuori da struct
    if (n > 0)
    {
        if (n > 1)
        {
            gos::logger::err ("SPVReflect::priv_parse_vtxShader_pushConstant() => error n is >1\n");
            return NULL;
        }

        SpvReflectBlockVariable **vars = (SpvReflectBlockVariable**) malloc (n * sizeof(SpvReflectBlockVariable*));
        result = spvReflectEnumeratePushConstantBlocks (module, &n, vars);
        if (SPV_REFLECT_RESULT_SUCCESS == result)
        {
            if (n > 0)
            {
                for (u32 i=0; i<n; i++)
                {
                    PushConstantNode *node = priv_pushConst_parseVar (module, vars[i], NULL);
                    if (NULL == root)
                        root = node;
                    else 
                        root->appendChild (node);
                }
            }
        }
        free (vars);
    }
    return root;
}

//***************************************************
void SPVReflect::priv_pushConst_merge (PushConstantNode *&dstIN, PushConstantNode *&srcIN)
{
    if (NULL == srcIN)
        return;

    PushConstantNode *src_prev = NULL;
    PushConstantNode *src = srcIN;
    while (src)
    {
        PushConstantNode *found = src;
        PushConstantNode *dst = dstIN;
        while (dst)
        {
            if (src->offset == dst->offset)
            {
                //l'elemento di src esiste gia' in dst
                dst->mergeFlagWith (src);
                priv_pushConst_merge (dst->figlio, src->figlio);
                found = NULL;
                break;
            }
            dst = dst->fratello;
        }

        if (NULL == found)
        {
            src_prev = src;
            src = src->fratello;
        }
        else
        {
            //tolgo <found> dal suo albero
            if (NULL == src_prev)
                srcIN = found->fratello;
            else
                src_prev->fratello = found->fratello;
            found->fratello = NULL;

            //inserisco <found> in <dst>
            PushConstantNode *prev = NULL;
            dst = dstIN;
            while (dst)            
            {
                if (dst->offset > found->offset)
                {
                    //<found> va inserito prima di me
                    found->fratello = dst;
                    if (NULL == prev)
                    {
                        dstIN = found;
                    }
                    else
                    {
                        prev->fratello = found;
                    }

                    found = NULL;
                    break;
                }

                prev = dst;
                dst = dst->fratello;
            }

            if (NULL == dst)
            {
                //se arrivo qui vuol dire che devo appendere <found> in fondo a <dst>
                assert (prev);
                prev->fratello = found;
                found->fratello = NULL;
            }

            //riparto da capo e termino
            priv_pushConst_merge (dstIN, srcIN);
            return;
        }


        
    }
}

//***************************************************
void SPVReflect::priv_pushConst_adjustPaddedSize  (PushConstantNode *node, u16 arrayStride)
{
    PushConstantNode *first = node;
    PushConstantNode *last = NULL;
    while (node)
    {
        if (node->isArray())
        {
            if (0 != node->numChildren)
                priv_pushConst_adjustPaddedSize (node->figlio, node->other.asArray.sizeOfOneElem);
        }
        else if (node->isStruct())
        {
            priv_pushConst_adjustPaddedSize (node->figlio);
        }
        else
        {
            if (node->fratello)
                node->paddedSize = node->fratello->absoluteOffset - node->absoluteOffset;
        }

        last = node;
        node = node->fratello;
    }

    if (0 != arrayStride && NULL != last)
    {
        //sono sull'ultimo elemento dell'array
        if ((last->absoluteOffset + last->paddedSize) - first->absoluteOffset < arrayStride)
            last->paddedSize = arrayStride  + first->absoluteOffset - last->absoluteOffset;
            
    }

}

//***************************************************
void SPVReflect::priv_pushConst_adjustArrayOffset (PushConstantNode *node ,u16 arrayStartAbsOffset)
{
    while (node)
    {
        if (u16MAX != arrayStartAbsOffset)
        {
            if (0 == node->absoluteOffset)
                node->absoluteOffset = arrayStartAbsOffset + node->offset;
        }

        if (node->isArray())
        {
            if (0 != node->numChildren)
                priv_pushConst_adjustArrayOffset (node->figlio, node->absoluteOffset);
        }
        else if (node->isStruct())
        {
            priv_pushConst_adjustArrayOffset (node->figlio);
        }

        node = node->fratello;
    }
}

//***************************************************
void SPVReflect::priv_pushConst_createGosDataBlobDef_ric (gos::datablob::DefBuilder &builder, PushConstantNode *node)
 {
    while (node)
    {
        u32 vtx_frag = 0;
        if (node->isUsedByVtxShader())  vtx_frag |= 0x01;
        if (node->isUsedByFragShader())  vtx_frag |= 0x02;

        if (node->isStruct())
        {
            builder.struct_beginAtOffset (node->absoluteOffset, node->name, vtx_frag);
            priv_pushConst_createGosDataBlobDef_ric (builder, node->figlio);
            builder.struct_end();
        }
        else if (node->isArray())
        {
            switch (node->other.asArray.numDimension)
            {
            default:
                DBGBREAK;
                break;

            case 1:
                builder.array_begin1DAtOffset (node->absoluteOffset, node->name, node->other.asArray.numElem[0], vtx_frag);
                break;
            case 2:
                builder.array_begin2DAtOffset (node->absoluteOffset, node->name, node->other.asArray.numElem[0], node->other.asArray.numElem[1], vtx_frag);
                break;
            case 3:
                builder.array_begin3DAtOffset (node->absoluteOffset, node->name, node->other.asArray.numElem[0], node->other.asArray.numElem[1], node->other.asArray.numElem[2], vtx_frag);
                break;
            }

            if (0 == node->numChildren)
                builder.add_simpleType (node->name, node->fmt, vtx_frag);
            else
                priv_pushConst_createGosDataBlobDef_ric (builder, node->figlio);
            builder.array_end();
        }
        else
        {
            builder.add_simpleTypeAtOffset (node->absoluteOffset, node->name, node->fmt, vtx_frag, node->paddedSize);
        }

        node = node->fratello;
    }
 }

//***************************************************
u8* SPVReflect::priv_pushConst_createGosDataBlobDef (gos::Allocator *allocator, PushConstantNode *node)
{
    if (NULL == node)
        return NULL;
    datablob::DefBuilder builder;

    builder.begin();
    priv_pushConst_createGosDataBlobDef_ric (builder, node);
    builder.end();
    if (builder.isValid())
        return builder.allocDataBlobDef (allocator);
    return NULL;
}

//***************************************************
void SPVReflect::priv_pushConst_printNode_appendUsageInfo(gos::UTF8String &out, const PushConstantNode *node) const
{
    out.fillRowUntilColumn (PRINT_COL1);
    out << "[";
    if ((node->flag & PushConstantNode::FLAG__USED_IN_VTX_SHADER) != 0)
        out << "VTX ";
    if ((node->flag & PushConstantNode::FLAG__USED_IN_FRAG_SHADER) != 0)
        out << "FRAG";
    out.fillRowUntilColumn (PRINT_COL1+9);
    out << "]";

    out.fillRowUntilColumn (PRINT_COL2);
    out << node->fullName;    
}

//***************************************************
void SPVReflect::priv_pushConst_printNode (gos::UTF8String &out, const PushConstantNode *node, u32 indent) const
{
    if (NULL == node)
        return;

    char sIndent[128];
    memset (sIndent, 0, sizeof(sIndent));
    if (indent)
        memset(sIndent, ' ', indent*4);

    if ((node->flag & PushConstantNode::FLAG__IS_STRUCT) != 0)
    {
        out << "\n" << sIndent << "struct";
        out.fillRowUntilColumn (PRINT_COL3);
        out << "abs-offset=" << STRFMT("% 5d", node->absoluteOffset) << "\n";
        
        out << sIndent << "{\n";
        priv_pushConst_printNode (out, node->figlio, indent+1);
        out << sIndent << "} " << node->name << ";";
        
        priv_pushConst_printNode_appendUsageInfo (out, node);

        out.fillRowUntilColumn (PRINT_COL3);
        out << "abs-offset=" << STRFMT("% 5d", node->absoluteOffset)
            << ", pad-size=" << STRFMT("% 5d", node->offset)
            << "\n";
    }
    else if ((node->flag & PushConstantNode::FLAG__IS_ARRAY) != 0)
    {
        char arrName[64];
        sprintf_s (arrName, sizeof(arrName), "%s", node->name);
        for (u8 i=0; i<node->other.asArray.numDimension; i++)
        {
            char s[32];
            sprintf_s (s, sizeof(s), "[%d]", node->other.asArray.numElem[i]);
            strcat_s (arrName, sizeof(arrName), s);
        }
        

        if (0 == node->numChildren)
        {
            //array semplice
            //char fmt[32];   sprintf_s (fmt, sizeof(fmt), "%-8s", gos::utils::enumToString(node->fmt));
            out << sIndent << STRFMT("%-8s", gos::utils::enumToString(node->fmt))  << arrName << ";";

            priv_pushConst_printNode_appendUsageInfo (out, node);

            out.fillRowUntilColumn (PRINT_COL3);
            out << "abs-offset=" << STRFMT("% 5d", node->absoluteOffset)
                << ", stride=" << node->other.asArray.sizeOfOneElem
                << "\n";
        }
        else
        {
            //array di struct
            out << "\n" << sIndent << "struct\n";
            out << sIndent << "{\n";
            priv_pushConst_printNode (out, node->figlio, indent+1);
            out << sIndent << "} " << arrName << ";";

            priv_pushConst_printNode_appendUsageInfo (out, node);

            out.fillRowUntilColumn (PRINT_COL3);
            out << "abs-offset=" << STRFMT("% 5d", node->absoluteOffset)
                << ", stride=" << node->other.asArray.sizeOfOneElem
                << "\n\n";            
        }
    }
    else 
    {
        if (node->fmt == eDataFormat::_unknown)
        {
            //non non categorizzato
            out << sIndent << node->name << "\n";
            priv_pushConst_printNode (out, node->figlio, indent+1);
        }
        else
        {
            //variabile semplice
            //char fmt[32];   sprintf_s (fmt, sizeof(fmt), "%-8s", gos::utils::enumToString(node->fmt));
            out << sIndent << STRFMT("%-8s", gos::utils::enumToString(node->fmt)) << node->name << ";";
            
            priv_pushConst_printNode_appendUsageInfo (out, node);

            out.fillRowUntilColumn (PRINT_COL3);
            out << "abs-offset=" << STRFMT("% 5d", node->absoluteOffset)
                << ", pad-size=" << STRFMT("% 5d", node->paddedSize)
                << "\n";
        }
    }

    priv_pushConst_printNode (out, node->fratello, indent);
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

            //tipo (vulkan) di descriptor
            switch (vars[i]->descriptor_type)
            {
            default:
                gos::logger::err ("SPVReflect::priv_parse_descriptors() => error <descriptor_type> invalid, %d\n", (int)vars[i]->descriptor_type);
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
                gos::logger::err ("SPVReflect::priv_parse_descriptors() => error <type_description> invalid, %d\n", (int)vars[i]->descriptor_type);
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
void SPVReflect::printInfo() const
{
    gos::UTF8String out;
    out.prealloc (1024);


    gos::logger::log ("SPVReflect::printInfo()\n");
    gos::logger::incIndent();

    out << "================================================\n"
        << "= VERTEX DECLARATION                           =\n"
        << "================================================\n";
    {
        if (0 == vtxDeclList.getNElem())
            out << "no info!\n";
        else
        {
            for (u32 i=0; i<vtxDeclList.getNElem(); i++)
            {
                out << utils::enumToString (vtxDeclList(i).fmt);
                out.fillRowUntilColumn (8);

                out << vtxDeclList(i).name << ";";
                out.fillRowUntilColumn (PRINT_COL1);

                out << "bindingLoc=" << vtxDeclList(i).bindingLocation
                    << ", size=" << STRFMT("% 4d", dataformat::getSize (vtxDeclList(i).fmt))
                    << "\n";
            }
        }
    }
    out << "\n\n";

    //push constant
    out << "================================================\n"
        << "= PUSH CONSTANT                                =\n"
        << "================================================\n";
    if (NULL == pushConstant_dataBlobDef)
    {
        out << "no info!\n";
    }
    else
    {
        datablob::blobDef_prinfInfo(out, pushConstant_dataBlobDef, [](UTF8String &out, const datablob::DefElem &elem) {
            if ((elem.getUserData() & 0x01) != 0)
            {
                if ((elem.getUserData() & 0x02) != 0)
                    out << "[VTX FRAG]";
                else
                    out << "[VTX     ]";
            }
            else if ((elem.getUserData() & 0x02) != 0)
            {
                out <<     "[FRAG    ]";
            }
            else
                out <<     "[        ]";
        });
    }



    out << "================================================\n"
        << "= DESCRIPTOR SETs                              =\n"
        << "================================================\n";

printf ("%s", out.getBuffer());


    //Descriptor sets
    {
        gos::logger::incIndent();
        if (0 == descrSetList.getNElem())
            gos::logger::log ("no info!\n");
        else
        {
            u8 last_descriptor_set = 0;
            for (u32 i=0; i<descrSetList.getNElem(); i++)
            {
                if (descrSetList(i).set != last_descriptor_set)
                {
                    last_descriptor_set = descrSetList(i).set;
                    gos::logger::log ("\n");
                }

                char stage[32];

                memset (stage, 0, sizeof(stage));

                if (descrSetList(i).flag & DescrSetElem::FLAG__USED_IN_VTX_SHADER)
                    strcat_s (stage, sizeof(stage), "VTX ");
                if (descrSetList(i).flag & DescrSetElem::FLAG__USED_IN_FRAG_SHADER)
                    strcat_s (stage, sizeof(stage), "FRG ");

                const sResInfo *resInfo = &descrSetList(i).resType;
                gos::logger::log ("[% -12s] name:% -32s (set=%d, binding=%d), VKtype=%s, data-type:%s", 
                        stage, 
                        descrSetList(i).name,
                        descrSetList(i).set, descrSetList(i).binding,
                        enumToString (descrSetList(i).vulkanDescrType),
                        enumToString (resInfo->type)
                        );

                
                switch (resInfo->type)
                {
                default: 
                    gos::logger::log (", ERR");
                    break;
                
                case eResourceType::_array:
                    {
                        gos::logger::log (", %s", descrSetList(i).name);
                        for (u8 t = 0; t < resInfo->info.asArray.ordine; t++)
                        {
                            gos::logger::log ("[%d]", resInfo->info.asArray.numElem[t]);
                        }
                    }
                    break;

                case eResourceType::_dynamicArray:
                    {
                        gos::logger::log (", %s", descrSetList(i).name);
                        for (u8 t = 0; t < resInfo->info.asArray.ordine; t++)
                        {
                            gos::logger::log ("[]");
                        }
                    }
                    break;
                
                case eResourceType::_struct:
                    for (u8 t = 0; t < resInfo->info.asStruct.numElem; t++)
                    {
                        const eDataFormat fmt = resInfo->info.asStruct.fmt[t];
                        gos::logger::log ("\n     % -16s, fmt=% -10s, size=%d",
                            resInfo->info.asStruct.name[t],
                            utils::enumToString (fmt), gos::dataformat::getSize(fmt));
                    }
                    break;

                }
                gos::logger::log ("\n");
            }
        }
        gos::logger::decIndent();
    }    

    gos::logger::decIndent();
}

