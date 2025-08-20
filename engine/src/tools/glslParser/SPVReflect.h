#ifndef _SPVReflect_h_
#define _SPVReflect_h_
#include "SPVReflectEnumAndDefine.h"
#include "gosFastArray.h"
#include "gosDataBlob.h"
#include "gosBit.h"
#include "string/gosUTF8String.h"

/**
 * @brief SPVReflect
 *  
 * Parsa gli shader in formato .spv
 * Gli shader devono essere compilati con l'opzione -g per generale i "nomi delle variabili"
 */
class SPVReflect
{
public:
    static const char* enumToString (eDescriptrorType s);
    static const char* enumToString (eResourceType s);

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


    void    printInfo() const;

private:
    struct VtxDeclElem
    {
    public:
                VtxDeclElem()   { reset(); }
        void    reset()         { memset(name, 0, sizeof(name)); bindingLocation=0; offsetInBuffer=0; fmt=eDataFormat::_unknown; }

    public:
        char        name[64];
        u8          bindingLocation;
        u32         offsetInBuffer;
        eDataFormat fmt;
    };

    class VtxDeclList
    {
    public:
                VtxDeclList()                               { list.setup(gos::getSysHeapAllocator(), 16); }
                ~VtxDeclList()                              { list.unsetup(); }

        void    reset()                                     { list.reset(); }
        void    add (const VtxDeclElem &elem)               { list.append(elem); }

        
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
                for (u32 i=0; i<n; i++)
                {
                    if (list(i).bindingLocation > list(i+1).bindingLocation)
                    {
                        bEsci = false;
                        VtxDeclElem swap = list[i];
                        list[i] = list[i+1];
                        list[i+1] = swap;
                    }
                }
            }
        }
        
        u32		getNElem()	const                           { return list.getNElem(); }
        const VtxDeclElem&  operator() (u32 i)	const       { return list(i); }

        
    private:
        gos::FastArray<VtxDeclElem>    list;
    };

    struct DescrSetElem
    {
    public:
        static constexpr u8     FLAG__USED_IN_VTX_SHADER     = 0x01;
        static constexpr u8     FLAG__USED_IN_FRAG_SHADER    = 0x02;

    public:
                    DescrSetElem()      { reset(); }
        void        reset()             { memset(name,0,sizeof(name)); flag=set=binding=0; vulkanDescrType=eDescriptrorType::UNKNOWN; count=0; }

    public:
        char                name[64];    
        u8                  flag;
        u8                  set;
        u8                  binding;
        eDescriptrorType    vulkanDescrType;
        u32                 count;
        sResInfo            resType;
    };

    class DescrSetList
    {
    public:
                DescrSetList()                              { list.setup(gos::getSysHeapAllocator(), 16); }
                ~DescrSetList()                             { list.unsetup(); }

        void    reset()                                         { list.reset(); }
        void    add (const DescrSetElem &elem)
        {
            for (u32 i=0; i<list.getNElem(); i++)
            {
                if (list(i).set == elem.set && list(i).binding == elem.binding)
                {
                    //ho trovato un elemeno che gia' esisteva in lista
                    list[i].flag |= elem.flag;
                    return;
                }
            }

            list.append(elem);
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
                for (u32 i=0; i<n; i++)
                {
                    bool bSwap = false;
                    if (list(i).set > list(i+1).set)
                    {
                        bSwap = true;
                    }
                    else if (list(i).set == list(i+1).set)
                    {
                        if (list(i).binding > list(i+1).binding)
                            bSwap = true;
                    }

                    if (bSwap)
                    {
                        bEsci = false;
                        DescrSetElem swap = list[i];
                        list[i] = list[i+1];
                        list[i+1] = swap;
                    }
                }
            }
        }
        
        u32		getNElem()	const	                            { return list.getNElem(); }
        const DescrSetElem&		operator() (u32 i)	const       { return list(i); }

        
    private:
        gos::FastArray<DescrSetElem>    list;
    };

    class PushConstantNode
    {
    public:
        static gos::Allocator *localAllocator;

    public:
        static PushConstantNode* createNew ()
        {
            PushConstantNode *p = GOSNEW(localAllocator, PushConstantNode)();
            return p;
        }

        static void deleteTree (PushConstantNode *root)
        {
            PushConstantNode *p = root;
            while (p)
            {
                PushConstantNode *thisNode = p;

                if (NULL != p->figlio)
                    deleteTree (p->figlio);
                p = p->fratello;

                GOSDELETE(localAllocator, thisNode);
            }
    }

    public:
        static constexpr u8     FLAG__USED_IN_VTX_SHADER    = 0x01;
        static constexpr u8     FLAG__USED_IN_FRAG_SHADER   = 0x02;
        static constexpr u8     FLAG__IS_STRUCT             = 0x04;
        static constexpr u8     FLAG__IS_ARRAY              = 0x08;

    public:
                    PushConstantNode()  { reset(); }
        void        reset()             { memset(name,0,sizeof(name)); memset(fullName,0,sizeof(fullName)); flag=0; absoluteOffset=offset=size=paddedSize=0; fmt=eDataFormat::_unknown; memset(&other, 0, sizeof(other)); figlio = fratello = NULL; numChildren=0;}
        void        mergeFlagWith (const PushConstantNode *node)
        {
            if ((node->flag & PushConstantNode::FLAG__USED_IN_VTX_SHADER) != 0) flag |= PushConstantNode::FLAG__USED_IN_VTX_SHADER;
            if ((node->flag & PushConstantNode::FLAG__USED_IN_FRAG_SHADER) != 0) flag |= PushConstantNode::FLAG__USED_IN_FRAG_SHADER;
        }

        void        appendChild (PushConstantNode *child)
        {
            numChildren++;

            PushConstantNode *p = this->figlio;
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

        bool        isArray() const                 { return (flag & FLAG__IS_ARRAY) != 0; }
        bool        isStruct() const                { return (flag & FLAG__IS_STRUCT) != 0; }
        bool        isUsedByVtxShader() const       { return (flag & FLAG__USED_IN_VTX_SHADER) != 0; }
        bool        isUsedByFragShader() const      { return (flag & FLAG__USED_IN_FRAG_SHADER) != 0; }

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
        char        fullName[128];    
        u8          flag;
        u32         offset;
        u32         size;
        u32         absoluteOffset;
        u32         paddedSize;
        eDataFormat fmt;
        eOther      other;
        u8          numChildren;
        PushConstantNode    *figlio;
        PushConstantNode    *fratello;

    friend SPVReflect;
    };

    class ArrayEnumerator
    {
    public:
                        ArrayEnumerator()   { }
        void            begin (u8 numDimensionIN, const u16 *numElemIN)     { numDimension = numDimensionIN; memcpy (numElem, numElemIN, sizeof(u16) * numDimension); memset(curElem,0,sizeof(curElem));  }
        const char*     get()
        {
            memset (s, 0, sizeof(s));
            for (u8 i=0; i<numDimension; i++)
            {
                char temp[8];
                sprintf_s (temp, sizeof(temp), "[%d]", curElem[i]);
                strcat_s (s, sizeof(s), temp);
            }
            return s;
        }
        bool            next()
        {
            u8 i = numDimension;
            while (i)
            {
                i--;
                if (curElem[i] < numElem[i] - 1)
                {
                    curElem[i]++;
                    return true;
                }
                curElem[i] = 0;
            }
            return false;
        }

    private:
        u8      numDimension;
        u16     numElem[16];
        u16     curElem[16];
        char    s[32];
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

    PushConstantNode*   priv_pushConst_parseModule (SpvReflectShaderModule *module);
    PushConstantNode*   priv_pushConst_parseVar (const SpvReflectShaderModule *module, const SpvReflectBlockVariable *var, const char *parentName);
    void                priv_pushConst_merge (PushConstantNode *&dst, PushConstantNode *&src);
    void                priv_pushConst_adjustArrayOffset (PushConstantNode *node, u16 arrayStartAbsOffset = u16MAX);
    void                priv_pushConst_adjustPaddedSize  (PushConstantNode *node, u16 arrayStride = 0);
    void                priv_pushConst_printNode (gos::UTF8String &out, const PushConstantNode *node, u32 indent) const;
    void                priv_pushConst_printNode_appendUsageInfo(gos::UTF8String &out, const PushConstantNode *node) const;
    u8*                 priv_pushConst_createGosDataBlobDef (gos::Allocator *allocator, PushConstantNode *node);
    void                priv_pushConst_createGosDataBlobDef_ric (gos::datablob::DefBuilder &builder, PushConstantNode *node);

private:
    gos::Allocator      *localAllocator;
    VtxDeclList         vtxDeclList;
    DescrSetList        descrSetList;

    PushConstantNode    *pushConstant_VS;
    PushConstantNode    *pushConstant_PS;
    PushConstantNode    *pushConstant_merged;
    u8                  *pushConstant_dataBlobDef;
};

#endif //_SPVReflect_h_