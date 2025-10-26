#include "SPVReflect.h"
#include "gosUtils.h"

using namespace gos;

gos::Allocator *SPVReflect::Node::localAllocator = NULL;


//***************************************************
SPVReflect::SPVReflect()
{
    localAllocator = gos::getSysHeapAllocator();
    SPVReflect::Node::localAllocator = localAllocator;

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
    vtxDeclList.reset();
    descrSetList.reset();

    Node::deleteTree (pushConstant_VS);
    Node::deleteTree (pushConstant_PS);
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
// const char* SPVReflect::enumToString_Usage (const gos::Flag8 usage)
// {
//     if (usage.isBitSet(TYPEDESCR__IS_STRUCT))          return "struct";
//     if (usage.isBitSet(TYPEDESCR__IS_ARRAY))           return "array";
//     if (usage.isBitSet(TYPEDESCR__IS_DYNAMIC_ARRAY))   return "dynamicArray";

//     DBGBREAK;
//     return "!!UNKNOWN TYPE!!";
// }

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
        if (NULL == strTypeDescr->struct_type_description)
            numCol = 1;
        else
        {
            numCol = 1;
            DBGBREAK;
        }
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
        if (0x00 != vtxShaderFilename[0])
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
    }

    if (!ret)
        return false;

    if (NULL != fragShaderFilename)
    {
        if (0x00 != fragShaderFilename[0])
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
    }
    if (!ret)
        return false;
    
    return endParseFromMemory();
}

 //***************************************************
void SPVReflect::beginParseFromMemory()
{
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
            priv_nodeTree_merge(pushConstant_merged, pushConstant_PS);
    }
    priv_nodeTree_adjustArrayOffset (pushConstant_merged);
    priv_nodeTree_adjustPaddedSize (pushConstant_merged);

    //out << "\nMERGED\n"; priv_pushConst_printNode (out, pushConstant_merged, 0);
    //printf ("%s", out.getBuffer());

    //Creo la blobDef per le push constant
    pushConstant_dataBlobDef = priv_nodeTree_createGosDataBlobDef(localAllocator, pushConstant_merged);


    //creo blobDef per i descriptor
    for (u32 i=0; i<descrSetList.getNElem(); i++)
    {
        switch (descrSetList(i).vulkanDescrType)
        {
        default:
            break;

        case eGPUDescriptrorType::UNIFORM_BUFFER:
        case eGPUDescriptrorType::STORAGE_BUFFER:
        case eGPUDescriptrorType::DYNAMIC_UNIFORM_BUFFER:
        case eGPUDescriptrorType::DYNAMIC_STORAGE_BUFFER:
            descrSetList[i].blobDef = priv_nodeTree_createGosDataBlobDef(Node::localAllocator, descrSetList(i).root);
            break;
        }
    }


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
SPVReflect::Node* SPVReflect::priv_parse_BlockVariable (const SpvReflectShaderModule *module, const SpvReflectBlockVariable *var)
{
    Node *node = Node::createNew();
    sprintf_s (node->name, sizeof(node->name), "%s", var->name);

    if ( (var->flags & SPV_REFLECT_VARIABLE_FLAGS_UNUSED) == 0)
    {
        if (SPV_REFLECT_SHADER_STAGE_VERTEX_BIT == module->shader_stage)
            node->usage.set (USAGE__USED_IN_VTX_SHADER);
        if (SPV_REFLECT_SHADER_STAGE_FRAGMENT_BIT == module->shader_stage)
            node->usage.set (USAGE__USED_IN_FRAG_SHADER);
    }

    node->offset = var->offset;
    node->absoluteOffset = var->absolute_offset;
    node->paddedSize = var->padded_size;

    switch (var->type_description->op)
    {
    case SpvOpTypeStruct:
        node->typeDescr.set (TYPEDESCR__IS_STRUCT);
        node->other.asStruct.numMembers = var->member_count;
        break;

    case SpvOpTypeArray:
    case SpvOpTypeRuntimeArray:
        //e' certamente un array
        node->typeDescr.set (TYPEDESCR__IS_ARRAY);

        //e' anche bindless?  (lo e' se e' stato dichiarato come pippo[])
        if (SpvOpTypeRuntimeArray == var->type_description->op)
            node->typeDescrSpecialization.set (TYPEDESCR_SPEC__IS_BINDLESS_ARRAY);

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

    for (u32 i=0; i<var->member_count; i++)
    {
        const SpvReflectBlockVariable *info = &var->members[i];
        Node *figlio = priv_parse_BlockVariable(module, info);
        node->appendChild (figlio);

        if (figlio->isUsedByVtxShader())
            node->usage.set (USAGE__USED_IN_VTX_SHADER);
        if (figlio->isUsedByFragShader())
            node->usage.set (USAGE__USED_IN_FRAG_SHADER);
    }
    
    //e' dinamico? (lo e' se il nome inizia con dyn_)
    if (strlen(var->name) > 4)
    {
        if (memcmp(var->name, "dyn_", 4) == 0)
            node->typeDescrSpecialization.set (TYPEDESCR_SPEC__IS_DYNAMIC);
    }  

    return node;
}

//***************************************************
void SPVReflect::priv_parse_TypeDescriptionForArray (const SpvReflectShaderModule *module, const SpvReflectTypeDescription *var, Node *padre)
{
    assert (NULL != padre);
    u32 offset = 0;
    for (u32 i=0; i<var->member_count; i++)
    {
        const SpvReflectTypeDescription *info = &var->members[i];

        Node *node = Node::createNew();
        sprintf_s (node->name, sizeof(node->name), "%s", info->struct_member_name);
        node->fmt = priv_fromSPVReflectTypeDescrToDataFormat (info);
        const u8 sizeof_fmt = gos::dataformat::getSize (node->fmt);

        node->offset = offset;
        node->absoluteOffset = offset;
        node->paddedSize = sizeof_fmt;
        offset += sizeof_fmt;


        if ( (info->type_flags & SPV_REFLECT_VARIABLE_FLAGS_UNUSED) == 0)
        {
            if (SPV_REFLECT_SHADER_STAGE_VERTEX_BIT == module->shader_stage)
                node->usage.set (USAGE__USED_IN_VTX_SHADER);
            if (SPV_REFLECT_SHADER_STAGE_FRAGMENT_BIT == module->shader_stage)
                node->usage.set (USAGE__USED_IN_FRAG_SHADER);
        }


        padre->appendChild (node);

        /*if (figlio->isUsedByVtxShader())
            node->usage.set (USAGE__USED_IN_VTX_SHADER);
        if (figlio->isUsedByFragShader())
            node->usage.set (USAGE__USED_IN_FRAG_SHADER);
            */
    }
}

//***************************************************
SPVReflect::Node* SPVReflect::priv_pushConst_parseModule (SpvReflectShaderModule *module)
{
    Node *root = NULL;

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
                    Node *node = priv_parse_BlockVariable (module, vars[i]);
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
void SPVReflect::priv_nodeTree_merge (Node *&dstIN, Node *&srcIN)
{
    if (NULL == srcIN)
        return;

    Node *src_prev = NULL;
    Node *src = srcIN;
    while (src)
    {
        Node *found = src;
        Node *dst = dstIN;
        while (dst)
        {
            if (src->offset == dst->offset)
            {
                //l'elemento di src esiste gia' in dst
                dst->mergeUsageWith (src);
                priv_nodeTree_merge (dst->figlio, src->figlio);
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
            Node *prev = NULL;
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
            priv_nodeTree_merge (dstIN, srcIN);
            return;
        }


        
    }
}

//***************************************************
void SPVReflect::priv_nodeTree_adjustPaddedSize  (Node *node, u16 arrayStride)
{
    Node *first = node;
    Node *last = NULL;
    while (node)
    {
        if (node->isType_array())
        {
            if (0 != node->numChildren)
                priv_nodeTree_adjustPaddedSize (node->figlio, node->other.asArray.sizeOfOneElem);
        }
        else if (node->isType_struct())
        {
            if (NULL != node->figlio)
            {
                if (node->absoluteOffset < node->figlio->absoluteOffset)
                    node->absoluteOffset = node->figlio->absoluteOffset;
                priv_nodeTree_adjustPaddedSize (node->figlio);
            }
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
void SPVReflect::priv_nodeTree_adjustArrayOffset (Node *node ,u16 arrayStartAbsOffset)
{
    while (node)
    {
        if (u16MAX != arrayStartAbsOffset)
        {
            if (0 == node->absoluteOffset)
                node->absoluteOffset = arrayStartAbsOffset + node->offset;
        }

        if (node->isType_array())
        {
            if (0 != node->numChildren)
                priv_nodeTree_adjustArrayOffset (node->figlio, node->absoluteOffset);
        }
        else if (node->isType_struct())
        {
            priv_nodeTree_adjustArrayOffset (node->figlio);
        }

        node = node->fratello;
    }
}

//***************************************************
void SPVReflect::priv_nodeTree_createGosDataBlobDef_ric (gos::datablob::DefBuilder &builder, Node *node)
 {
    while (node)
    {
        u32 vtx_frag = 0;
        if (node->isUsedByVtxShader())  vtx_frag |= 0x01;
        if (node->isUsedByFragShader())  vtx_frag |= 0x02;

        if (node->isType_struct())
        {
            builder.struct_beginAtOffset (node->absoluteOffset, node->name, vtx_frag);
            priv_nodeTree_createGosDataBlobDef_ric (builder, node->figlio);
            builder.struct_end();
        }
        else if (node->isType_array())
        {
            switch (node->other.asArray.numDimension)
            {
            default:
                DBGBREAK;
                break;

            case 0: //caso di array senza dimensioni (pippo[])
                node->other.asArray.numDimension = 1;
                node->other.asArray.numElem[0] = 1;
                builder.array_begin1DAtOffset (node->absoluteOffset, node->name, node->other.asArray.numElem[0], vtx_frag);
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
                priv_nodeTree_createGosDataBlobDef_ric (builder, node->figlio);
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
u8* SPVReflect::priv_nodeTree_createGosDataBlobDef (gos::Allocator *allocator, Node *node)
{
    if (NULL == node)
        return NULL;
    datablob::DefBuilder builder;

    builder.begin();
    priv_nodeTree_createGosDataBlobDef_ric (builder, node);
    builder.end();
    if (builder.isValid())
        return builder.allocDataBlobDef (allocator);
    return NULL;
}

//***************************************************
void SPVReflect::priv_printNode_appendUsageInfo (gos::UTF8String &out, const Node *node) const
{
    out.fillRowUntilColumn (PRINT_COL1);
    out << "[";
    if (node->isUsedByVtxShader())
        out << "VTX ";
    if (node->isUsedByFragShader())
        out << "FRAG";
    out.fillRowUntilColumn (PRINT_COL1+9);
    out << "]";
}

//***************************************************
void SPVReflect::priv_printNode (gos::UTF8String &out, const Node *node, u32 indent) const
{
    if (NULL == node)
        return;

    char sIndent[128];
    memset (sIndent, 0, sizeof(sIndent));
    if (indent)
        memset(sIndent, ' ', indent*4);

    if (node->isType_struct())
    {
        out << "\n" << sIndent << "struct";
        out.fillRowUntilColumn (PRINT_COL3);
        out << "abs-offset=" << STRFMT("% 5d", node->absoluteOffset) << "\n";
        
        out << sIndent << "{\n";
        priv_printNode (out, node->figlio, indent+1);
        out << sIndent << "} " << node->name << ";";
        
        priv_printNode_appendUsageInfo (out, node);

        out.fillRowUntilColumn (PRINT_COL3);
        out << "abs-offset=" << STRFMT("% 5d", node->absoluteOffset)
            << ", pad-size=" << STRFMT("% 5d", node->offset)
            << "\n";
    }
    else if (node->isType_array())
    {
        char arrName[64];
        sprintf_s (arrName, sizeof(arrName), "%s", node->name);
        if (node->isType_bindlessArray())
            strcat_s (arrName, sizeof(arrName), "[]");
        else
        {
            for (u8 i=0; i<node->other.asArray.numDimension; i++)
            {
                char s[32];
                sprintf_s (s, sizeof(s), "[%d]", node->other.asArray.numElem[i]);
                strcat_s (arrName, sizeof(arrName), s);
            }
        }        

        if (0 == node->numChildren)
        {
            //array semplice
            //char fmt[32];   sprintf_s (fmt, sizeof(fmt), "%-8s", gos::utils::enumToString(node->fmt));
            out << sIndent << STRFMT("%-8s", gos::utils::enumToString(node->fmt))  << arrName << ";";

            priv_printNode_appendUsageInfo (out, node);

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
            priv_printNode (out, node->figlio, indent+1);
            out << sIndent << "} " << arrName << ";";

            priv_printNode_appendUsageInfo (out, node);

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
            priv_printNode (out, node->figlio, indent+1);
        }
        else
        {
            //variabile semplice
            //char fmt[32];   sprintf_s (fmt, sizeof(fmt), "%-8s", gos::utils::enumToString(node->fmt));
            out << sIndent << STRFMT("%-8s", gos::utils::enumToString(node->fmt)) << node->name << ";";
            
            priv_printNode_appendUsageInfo (out, node);

            out.fillRowUntilColumn (PRINT_COL3);
            out << "abs-offset=" << STRFMT("% 5d", node->absoluteOffset)
                << ", pad-size=" << STRFMT("% 5d", node->paddedSize)
                << "\n";
        }
    }

    priv_printNode (out, node->fratello, indent);
}

//***************************************************
void SPVReflect::priv_descriptor_parseVar (const SpvReflectShaderModule *module, const SpvReflectDescriptorBinding *var)
{
    DescrSetElem e;
    e.set = var->set;
    e.binding = var->binding;
    if (0 != var->accessed)
    {
        if (SPV_REFLECT_SHADER_STAGE_VERTEX_BIT == module->shader_stage)
            e.usage.set (USAGE__USED_IN_VTX_SHADER);
        if (SPV_REFLECT_SHADER_STAGE_FRAGMENT_BIT == module->shader_stage)
            e.usage.set (USAGE__USED_IN_FRAG_SHADER);
    }

    //tipo (vulkan) di descriptor
    switch (var->descriptor_type)
    {
    default:
        gos::logger::err ("SPVReflect::priv_descriptor_parseVar() => error <descriptor_type> invalid, %d\n", (int)var->descriptor_type);
        return;

    case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER:                   e.vulkanDescrType = eGPUDescriptrorType::SAMPLER; break;
    case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:    e.vulkanDescrType = eGPUDescriptrorType::COMBINED_IMAGE_SAMPLER; break;
    case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE:             e.vulkanDescrType = eGPUDescriptrorType::TEXTURE2D; break;
    case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE:             e.vulkanDescrType = eGPUDescriptrorType::STORAGE_IMAGE; break;
    case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:      e.vulkanDescrType = eGPUDescriptrorType::UNIFORM_TEXEL_BUFFER; break;
    case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:      e.vulkanDescrType = eGPUDescriptrorType::STORAGE_TEXEL_BUFFER; break;
    
    case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:            e.vulkanDescrType = eGPUDescriptrorType::UNIFORM_BUFFER; break;
    case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER:            e.vulkanDescrType = eGPUDescriptrorType::STORAGE_BUFFER; break;
    case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
        e.vulkanDescrType = eGPUDescriptrorType::DYNAMIC_UNIFORM_BUFFER;
        break;
    
    case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
        e.vulkanDescrType = eGPUDescriptrorType::DYNAMIC_STORAGE_BUFFER; 
        break;
    
    case SPV_REFLECT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:          e.vulkanDescrType = eGPUDescriptrorType::INPUT_ATTACHMENT; break;
    }


    switch (var->type_description->op)
    {
    default:
        gos::logger::err ("SPVReflect::priv_descriptor_parseVar() => error <type_description> invalid, %d\n", (int)var->descriptor_type);
        return;

    case SpvOpTypeSampler:
    case SpvOpTypeImage:
    case SpvOpTypeSampledImage:
        e.root = Node::createNew();
        sprintf_s (e.root->name, sizeof(e.root->name), "%s", var->name);
        e.root->usage = e.usage;
        break;

    case SpvOpTypeArray:
    case SpvOpTypeRuntimeArray:
        {
            e.root = Node::createNew();
            sprintf_s (e.root->name, sizeof(e.root->name), "%s", var->name);
            e.root->usage = e.usage;
            
            //e' certamente un array
            e.root->typeDescr.set(TYPEDESCR__IS_ARRAY);

            //e' anche bindless?  (lo e' se e' stato dichiarato come pippo[])
            if (SpvOpTypeRuntimeArray == var->type_description->op)
                e.root->typeDescrSpecialization.set (TYPEDESCR_SPEC__IS_BINDLESS_ARRAY);

            e.root->other.asArray.numDimension = var->array.dims_count;
            for (u8 t = 0; t < e.root->other.asArray.numDimension; t++)
            {
                e.root->other.asArray.numElem[t] = var->array.dims[t];
            }

            if (NULL != var->type_description->struct_type_description)
            {
                //e' un array di struct
                priv_parse_TypeDescriptionForArray (module, var->type_description->struct_type_description, e.root);
            }
            else
            {
                //Solitamente indica un array di un tipo non convenzionale, tipo "texture"
                //DBGBREAK;
            }
        }
        break;

    case SpvOpTypeStruct:
        {
            e.root = Node::createNew();
            e.root->usage = e.usage;

            sprintf_s (e.root->name, sizeof(e.root->name), "%s", var->name);
            e.root->typeDescr.set (TYPEDESCR__IS_STRUCT);

            for (u8 t = 0; t < var->block.member_count; t++)
            {
                Node *node = priv_parse_BlockVariable (module, &var->block.members[t]);
                e.root->appendChild (node);

                if (node->typeDescrSpecialization.isBitSet(TYPEDESCR_SPEC__IS_BINDLESS_ARRAY))
                    e.root->typeDescrSpecialization.set (TYPEDESCR_SPEC__IS_BINDLESS_ARRAY);
            }
        }
        break;

    }

    //e' dinamico? (lo e' se il nome inizia con dyn_)
    if (strlen(var->name) > 4)
    {
        if (memcmp(var->name, "dyn_", 4) == 0)
        {
            switch (e.vulkanDescrType)
            {
            default:
                break;

            case eGPUDescriptrorType::UNIFORM_BUFFER:
                e.vulkanDescrType = eGPUDescriptrorType::DYNAMIC_UNIFORM_BUFFER;
                e.root->typeDescrSpecialization.set (TYPEDESCR_SPEC__IS_DYNAMIC);
                break;

            case eGPUDescriptrorType::STORAGE_BUFFER:
                e.vulkanDescrType = eGPUDescriptrorType::DYNAMIC_STORAGE_BUFFER;
                e.root->typeDescrSpecialization.set (TYPEDESCR_SPEC__IS_DYNAMIC);
                break;
            }
        }
    }    


    u32 index = descrSetList.addIfNotExists(e);
    if (u32MAX != index)
    {
        if (NULL != e.root)
        {
            priv_nodeTree_merge (descrSetList[index].root, e.root);
            Node::deleteTree (e.root);
            e.root = NULL;
        }
    }
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
            priv_descriptor_parseVar (module, vars[i]);
        }
        free (vars);
    }
    

    descrSetList.sort();

    for (u32 i=0; i<descrSetList.getNElem(); i++)
    {
        priv_nodeTree_adjustArrayOffset (descrSetList[i].root);
        priv_nodeTree_adjustPaddedSize (descrSetList[i].root);
    }
    return true;
}

//***************************************************
u32 SPVReflect::descrset_getNumSet() const
{
    if (0 == descrSetList.getNElem())
        return 0;

    u32 ret = descrSetList(0).set;
    for (u32 i = 1; i < descrSetList.getNElem(); i++)
    {
        if (descrSetList(i).set > ret)
            ret = descrSetList(i).set;
    }
    return ret+1;
}

//***************************************************
u32 SPVReflect::descrset_getNumElemPerSet (u32 set) const
{
    u32 ret = 0;
    for (u32 i = 0; i < descrSetList.getNElem(); i++)
    {
        if (descrSetList(i).set == set)
            ret++;
    }
    return ret;
}

//***************************************************
eGPUDescriptrorSetOptionBitmask SPVReflect::descrset_getOptionsPerSet(u32 set) const
{
    for (u32 i = 0; i < descrSetList.getNElem(); i++)
    {
        if (descrSetList(i).set == set)
            return descrSetList.getOptions(i);
    }
    return eGPUDescriptrorSetOption::none;
}

//***************************************************
void SPVReflect::descrset_getElemByIndex  (u32 set, u8 index, u8 *out_binding, eGPUDescriptrorType *out_type, u32 *out_arraySize, eGPUDescriptrorUsageBitmask *out_usage) const
{
    for (u32 i = 0; i < descrSetList.getNElem(); i++)
    {
        if (descrSetList(i).set == set)
        {
            while (index--)
            {
                i++;
            }

            assert (i < descrSetList.getNElem());
            assert (descrSetList(i).set == set);

            priv_descrset_getElemInfo (i, out_binding, out_type, out_arraySize, out_usage);
            return;
        }
    }
    DBGBREAK;
}

//***************************************************
void SPVReflect::priv_descrset_getElemInfo  (u32 i, u8 *out_binding, eGPUDescriptrorType *out_type, u32 *out_arraySize, eGPUDescriptrorUsageBitmask *out_usage) const
{
    *out_binding = descrSetList(i).binding;
    *out_type = descrSetList(i).vulkanDescrType;

    *out_arraySize = 1;
    if (descrSetList(i).root->isType_array())
    {
        if (descrSetList(i).root->isType_bindlessArray())
        {
            switch (descrSetList(i).vulkanDescrType)
            {
            default:
                *out_arraySize = u32MAX;
                break;

            case eGPUDescriptrorType::TEXTURE2D:
                *out_arraySize = 1024;
                break;
            }
        }
        else
            *out_arraySize = descrSetList(i).root->other.asArray.numElem[0];
    }

    out_usage->zero();
    if (descrSetList(i).usage.isBitSet(USAGE__USED_IN_VTX_SHADER))
        *out_usage |= eGPUDescriptrorUsage::vtx_shader;
    if (descrSetList(i).usage.isBitSet(USAGE__USED_IN_FRAG_SHADER))
        *out_usage |= eGPUDescriptrorUsage::pxl_shader;
}


//***************************************************
void SPVReflect::printInfo (gos::UTF8String &out) const
{
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
                    << ", offset=" << STRFMT("% 4d", vtxDeclList(i).offsetInBuffer)
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

    if (0 == descrSetList.getNElem())
        out << "no info!\n";
    else
    {
        for (u32 i=0; i<descrSetList.getNElem(); i++)
        {
            u8 binding;
            eGPUDescriptrorType vktype;
            u32 arraySize;
            eGPUDescriptrorUsageBitmask usage;
            priv_descrset_getElemInfo  (i, &binding, &vktype, &arraySize, &usage);


            out << "\n----------------------------------------------------------------\n";
            out << "set=" << descrSetList(i).set << ", binding=" << binding << ", vktype=" << utils::enumToString(vktype)
                << ", numElem=" << arraySize <<", usage=" << STRFMT("%08X", usage);

            out << ", bindless=";
            descrSetList(i).root->isType_bindlessArray() ? out << "Y" : out << "N";

            out << "\n";
                
            
            //priv_printNode (out, descrSetList(i).root, 0);
            if (NULL == descrSetList(i).blobDef)
            {
                out << descrSetList(i).root->name;
                
                if (descrSetList(i).root->isType_array())
                {
                    if (descrSetList(i).root->isType_bindlessArray())
                    {
                        out << "[];\n";
                    }
                    else
                    {
                        for (u8 i2=0; i2<descrSetList(i).root->other.asArray.numDimension; i2++)
                            out << "[" << descrSetList(i).root->other.asArray.numElem[i2] << "]";
                        out << ";\n";
                    }
                }
            }
            else
            {
                out << "\n";
                datablob::blobDef_prinfInfo(out, descrSetList(i).blobDef, [](UTF8String &out, const datablob::DefElem &elem) {
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
        }
    }
}

