#ifndef _SPVReflect_h_
#define _SPVReflect_h_
#include "spirv_reflect.h"
#include "gosFastArray.h"
#include "gosDataBlob.h"
#include "gosBit.h"
#include "string/gosUTF8String.h"

namespace gos
{
    /**
     * @brief SPVReflect
     *
     * Parsa gli shader in formato .spv
     * Gli shader devono essere compilati con l'opzione -g per generale i "nomi delle variabili"
     */
    class SPVReflect
    {
    public:
        SPVReflect();
        ~SPVReflect();

        bool    parseFromFile (const char *vtxShaderFilename, const char *fragShaderFilename);
        //in caso di errore, gos::err contiene un messaggio specifico


        void    beginParseFromMemory();
        bool    VS_parseFromMemory (const u8 *buffer, u32 bufferSize);
        //in caso di errore, gos::err contiene un messaggio specifico

        bool    PS_parseFromMemory (const u8 *buffer, u32 bufferSize);
        //in caso di errore, gos::err contiene un messaggio specifico

        bool    endParseFromMemory();

        void    printInfo (gos::UTF8String &out) const;

        //================= query
        u32         vtxdecl_getNumElem() const { return vtxDeclList.getNElem(); }
        void        vtxdecl_getElemByIndex (u32 index, u8 *out_bindingLocation, u32 *out_offset, eDataFormat *out_fmt) const { assert (index < vtxdecl_getNumElem()); *out_bindingLocation = vtxDeclList(index).bindingLocation; *out_offset = vtxDeclList(index).offsetInBuffer; *out_fmt = vtxDeclList(index).fmt; }

        const u8*   pushconst_getDataBlobDef() const { return pushConstant_dataBlobDef; }

        u32         descrset_getNumSet() const;

        eGPUDescriptrorSetOptionBitmask descrset_getOptionsPerSet(u32 set) const;
        u32         descrset_getNumElemPerSet (u32 set) const;

                    // out_usage e' una bitmask di eGPUDescriptrorUsageFlag
        void        descrset_getElemByIndex  (u32 set, u8 index, u8 *out_binding, eGPUDescriptrorType *out_type, u32 *out_arraySize, eGPUDescriptrorUsageBitmask *out_usage) const;
        

    protected:
        static constexpr u8     USAGE__USED_IN_VTX_SHADER = 0;
        static constexpr u8     USAGE__USED_IN_FRAG_SHADER = 1;

        static constexpr u8     TYPEDESCR__IS_STRUCT = 0;
        static constexpr u8     TYPEDESCR__IS_ARRAY = 1;
        
        static constexpr u8     TYPEDESCR_SPEC__IS_DYNAMIC = 0;
        static constexpr u8     TYPEDESCR_SPEC__IS_BINDLESS_ARRAY = 1;

        static constexpr u8     DESCRIPTOR_TYPE__BLOBDEF = 0;
        static constexpr u8     DESCRIPTOR_TYPE__OTHER = 1;
        static constexpr u8     DESCRIPTOR_TYPE__SIMPLE_ARRAY = 2;
        static constexpr u8     DESCRIPTOR_TYPE__DYNAMIC_ARRAY = 3;
        static constexpr u8     DESCRIPTOR_TYPE__BINDLESS_ARRAY = 4;
        static constexpr u8     DESCRIPTOR_TYPE__BINDLESS_DYNAMIC_ARRAY = 5;
        static constexpr u8     DESCRIPTOR_TYPE__UNKNOWN = 0xff;


    protected:
        //static const char* enumToString_Usage (const gos::Flag8 usage);



    private:
        struct VtxDeclElem
        {
        public:
            VtxDeclElem() { reset(); }
            void    reset() { memset(name, 0, sizeof(name)); bindingLocation = 0; offsetInBuffer = 0; fmt = eDataFormat::_unknown; }

        public:
            char        name[64];
            u8          bindingLocation;
            u32         offsetInBuffer;
            eDataFormat fmt;
        };

        class VtxDeclList
        {
        public:
            VtxDeclList() { list.setup(gos::getSysHeapAllocator(), 16); }
            ~VtxDeclList() { list.unsetup(); }

            void    reset() { list.reset(); }
            void    add (const VtxDeclElem &elem) { list.append(elem); }


            void    sort()
            {
                u32 n = list.getNElem();
                if (0 == n)
                    return;

                bool bEsci = false;
                while (bEsci == false)
                {
                    bEsci = true;
                    n--;
                    for (u32 i = 0; i < n; i++)
                    {
                        if (list(i).bindingLocation > list(i + 1).bindingLocation)
                        {
                            bEsci = false;
                            VtxDeclElem swap = list[i];
                            list[i] = list[i + 1];
                            list[i + 1] = swap;
                        }
                    }
                }

                for (u32 i = 1; i < list.getNElem(); i++)
                {
                    list[i].offsetInBuffer = list(i-1).offsetInBuffer + gos::dataformat::getSize(list(i-1).fmt);
                }
            }

            u32		getNElem()	const { return list.getNElem(); }
            const VtxDeclElem&  operator() (u32 i)	const { return list(i); }


        private:
            gos::FastArray<VtxDeclElem>    list;
        };

        class Node
        {
        public:
            static gos::Allocator *localAllocator;

        public:
            static Node* createNew ()
            {
                Node *p = GOSNEW(localAllocator, Node)();
                return p;
            }

            static void deleteTree (Node *root)
            {
                Node *p = root;
                while (p)
                {
                    Node *thisNode = p;
                    p = p->fratello;

                    if (NULL != thisNode->figlio)
                        deleteTree (thisNode->figlio);
                    GOSDELETE(localAllocator, thisNode);
                }
            }

        public:
                        Node()                                  { reset(); }
            void        reset()                                 { memset(name, 0, sizeof(name)); usage.zero(); typeDescr.zero(); typeDescrSpecialization.zero(); absoluteOffset = offset = size = paddedSize = 0; fmt = eDataFormat::_unknown; memset(&other, 0, sizeof(other)); figlio = fratello = NULL; numChildren = 0; }
            void        mergeUsageWith (const Node *node)       { if (node->isUsedByVtxShader())  usage.set(USAGE__USED_IN_VTX_SHADER); if (node->isUsedByFragShader())  usage.set(USAGE__USED_IN_FRAG_SHADER); }

            void        appendChild (Node *child)
            {
                numChildren++;

                Node *p = this->figlio;
                if (NULL == p)
                {
                    this->figlio = child;
                }
                else
                {
                    while (p->fratello)
                    {
                        p = p->fratello;
                    }
                    p->fratello = child;
                }

            }

            bool        isUsedByVtxShader() const               { return usage.isBitSet(USAGE__USED_IN_VTX_SHADER); }
            bool        isUsedByFragShader() const              { return usage.isBitSet(USAGE__USED_IN_FRAG_SHADER); }

            bool        isType_struct() const                   { return typeDescr.isBitSet(TYPEDESCR__IS_STRUCT); }
            bool        isType_array() const                    { return typeDescr.isBitSet(TYPEDESCR__IS_ARRAY); }

            bool        isType_dynamic() const                  { return typeDescrSpecialization.isBitSet(TYPEDESCR_SPEC__IS_DYNAMIC); }
            bool        isType_bindlessArray() const            { return typeDescrSpecialization.isBitSet(TYPEDESCR_SPEC__IS_BINDLESS_ARRAY); }
            

        public:
            struct sAsStruct
            {
                u8  numMembers;
            };
            struct sAsArray
            {
                u8  numDimension;
                u16 sizeOfOneElem;
                u16 numElem[8];
            };

            union eOther
            {
                sAsStruct   asStruct;
                sAsArray    asArray;
            };

        public:
            char        name[64];
            gos::Flag8  usage;
            gos::Flag8  typeDescr;
            gos::Flag8  typeDescrSpecialization;
            u32         offset;
            u32         size;
            u32         absoluteOffset;
            u32         paddedSize;
            eDataFormat fmt;
            eOther      other;
            u8          numChildren;

            Node        *figlio;
            Node        *fratello;

            friend SPVReflect;
        };

        struct DescrSetElem
        {
        public:
                        DescrSetElem()                                      { reset(); }
            void        reset()                                             { root = NULL; blobDef = NULL; usage.zero(); set = binding = 0; vulkanDescrType = eGPUDescriptrorType::UNKNOWN; }

            bool        isUsedByVtxShader() const                           { return usage.isBitSet(USAGE__USED_IN_VTX_SHADER); }
            bool        isUsedByFragShader() const                          { return usage.isBitSet(USAGE__USED_IN_FRAG_SHADER); }

        public:
            gos::Flag8              usage;
            u8                      set;
            u8                      binding;
            eGPUDescriptrorType     vulkanDescrType;
            Node                    *root;
            u8                      *blobDef;
        };

        class DescrSetList
        {
        public:
                    DescrSetList()              { list.setup(gos::getSysHeapAllocator(), 16); dsOptionList.setup(gos::getSysHeapAllocator(), 16); }
                    ~DescrSetList()             { reset(); list.unsetup(); }

            void    reset()
            {
                for (u32 i = 0; i < list.getNElem(); i++)
                {
                    if (NULL != list(i).root)
                    {
                        if (NULL != list[i].root)
                            Node::deleteTree (list[i].root);
                    }
                    if (NULL != list(i).blobDef)
                    {
                        GOSFREE(Node::localAllocator, list[i].blobDef);
                        list[i].blobDef = NULL;
                    }
                }
                list.reset();
                dsOptionList.reset();
            }

            u32     addIfNotExists (DescrSetElem &elem)
            {
                for (u32 i = 0; i < list.getNElem(); i++)
                {
                    if (list(i).set == elem.set && list(i).binding == elem.binding)
                    {
                        //ho trovato un elemeno che gia' esisteva in lista
                        list[i].usage |= elem.usage;
                        return i;;
                    }
                }

                list.append(elem);

                const u32 n = list.getNElem() -1;
                dsOptionList[n] = eGPUDescriptrorSetOption::none;
                if (elem.root->typeDescrSpecialization.isBitSet (SPVReflect::TYPEDESCR_SPEC__IS_BINDLESS_ARRAY))
                    dsOptionList[n] = eGPUDescriptrorSetOption::bindless;
                return u32MAX;
            }

            void    sort()
            {
                u32 n = list.getNElem();
                if (0 == n)
                    return;

                bool bEsci = false;
                while (bEsci == false)
                {
                    bEsci = true;
                    n--;
                    for (u32 i = 0; i < n; i++)
                    {
                        bool bSwap = false;
                        if (list(i).set > list(i + 1).set)
                        {
                            bSwap = true;
                        }
                        else if (list(i).set == list(i + 1).set)
                        {
                            if (list(i).binding > list(i + 1).binding)
                                bSwap = true;
                        }

                        if (bSwap)
                        {
                            bEsci = false;
                            DescrSetElem swap = list[i];
                            list[i] = list[i + 1];
                            list[i + 1] = swap;

                            auto swap2 = dsOptionList[i];
                            dsOptionList[i] = dsOptionList[i+1];
                            dsOptionList[i+1] = swap2;
                        }
                    }
                }
            }

            u32		                            getNElem()	const                   { return list.getNElem(); }
            const DescrSetElem&		            operator() (u32 i)	const           { return list(i); }
            DescrSetElem&		                operator[] (u32 i)                  { return list[i]; }
            eGPUDescriptrorSetOptionBitmask     getOptions (u32 i) const            { return dsOptionList(i); }

        private:
            gos::FastArray<eGPUDescriptrorSetOptionBitmask> dsOptionList;
            gos::FastArray<DescrSetElem>                    list;
        };

    private:
        static constexpr u8 PRINT_COL1 = 50;
        static constexpr u8 PRINT_COL2 = 62;
        static constexpr u8 PRINT_COL3 = 100;

    private:
        void                priv_free();
        bool                priv_SpvReflectFormat_to_eDataFormat (SpvReflectFormat fmtIN, eDataFormat *out_fmt) const;
        eDataFormat         priv_fromSPVReflectTypeDescrToDataFormat (const SpvReflectTypeDescription *strTypeDescr) const;

        bool                priv_parse_vtxShader (SpvReflectShaderModule *module);
        bool                priv_parse_vtxShader_vtxDecl (SpvReflectShaderModule *module);
        bool                priv_parse_fragShader (SpvReflectShaderModule *module);
        bool                priv_parse_descriptors (SpvReflectShaderModule *module);

        Node*               priv_parse_BlockVariable (const SpvReflectShaderModule *module, const SpvReflectBlockVariable *var);
        void                priv_parse_TypeDescriptionForArray (const SpvReflectShaderModule *module, const SpvReflectTypeDescription *var, Node *node);

        void                priv_descriptor_parseVar (const SpvReflectShaderModule *module, const SpvReflectDescriptorBinding *var);

        Node*               priv_pushConst_parseModule (SpvReflectShaderModule *module);

        void                priv_descrset_getElemInfo  (u32 descrSetIndex, u8 *out_binding, eGPUDescriptrorType *out_type, u32 *out_arraySize, eGPUDescriptrorUsageBitmask *out_usage) const;
        
        u8*                 priv_nodeTree_createGosDataBlobDef (gos::Allocator *allocator, Node *node);
        void                priv_nodeTree_createGosDataBlobDef_ric (gos::datablob::DefBuilder &builder, Node *node);
        void                priv_nodeTree_merge (Node *&dst, Node *&src);
        void                priv_nodeTree_adjustArrayOffset (Node *node, u16 arrayStartAbsOffset = u16MAX);
        void                priv_nodeTree_adjustPaddedSize  (Node *node, u16 arrayStride = 0);

        void                priv_printNode (gos::UTF8String &out, const Node *node, u32 indent) const;
        void                priv_printNode_appendUsageInfo(gos::UTF8String &out, const Node *node) const;


    private:
        gos::Allocator      *localAllocator;
        VtxDeclList         vtxDeclList;
        DescrSetList        descrSetList;

        Node    *pushConstant_VS;
        Node    *pushConstant_PS;
        Node    *pushConstant_merged;
        u8      *pushConstant_dataBlobDef;
    };
} //namespace gos
#endif //_SPVReflect_h_
