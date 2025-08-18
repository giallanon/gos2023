#ifndef _SPVReflect_h_
#define _SPVReflect_h_
#include "SPVReflectEnumAndDefine.h"
#include "gosFastArray.h"
#include "gosDataBlob.h"


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

    struct PushConstantElem
    {
    public:
        static constexpr u8     FLAG__USED_IN_VTX_SHADER    = 0x01;
        static constexpr u8     FLAG__USED_IN_FRAG_SHADER   = 0x02;
        static constexpr u8     FLAG__IS_STRUCT             = 0x04;
        static constexpr u8     FLAG__IS_ARRAY              = 0x08;

    public:
                    PushConstantElem()  { reset(); }
        void        reset()             { memset(name,0,sizeof(name)); flag=0; offset=size=paddedSize=0; fmt=eDataFormat::_unknown; memset(&other, 0, sizeof(other)); }

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
        u8          flag;
        u32         offset;
        u32         size;
        u32         absoluteOffset;
        u32         paddedSize;
        eDataFormat fmt;
        eOther      other;
    };

    class PushConstantList
    {
    public:
                PushConstantList()                              { list.setup(gos::getSysHeapAllocator(), 16); }
                ~PushConstantList()                             { list.unsetup(); }

        void    reset()                                         { list.reset(); }
        void    add (const PushConstantElem &elem)
        {
            for (u32 i=0; i<list.getNElem(); i++)
            {
                if (list(i).offset == elem.offset && list(i).size == elem.size && list(i).paddedSize == elem.paddedSize && list(i).fmt == elem.fmt)
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
                    if (list(i).absoluteOffset > list(i+1).absoluteOffset)
                    {
                        bEsci = false;
                        PushConstantElem swap = list[i];
                        list[i] = list[i+1];
                        list[i+1] = swap;
                    }
                }
            }
        }
                
        u32		getNElem()	const	                            { return list.getNElem(); }
        const PushConstantElem&		operator() (u32 i)	const   { return list(i); }

        
    private:
        gos::FastArray<PushConstantElem>    list;
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
        void        reset()             { memset(name,0,sizeof(name)); flag=0; offset=size=paddedSize=0; fmt=eDataFormat::_unknown; memset(&other, 0, sizeof(other)); figlio = fratello = NULL;}

        void        appendChild (PushConstantNode *child)
        {
            PushConstantNode *p = this->figlio;
            if (NULL == p)
                this->figlio = child;
            else
            {
                while (p->fratello)
                {
                    p = p->fratello;
                }
                p->fratello = child;
            }

        }

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
        u8          flag;
        u32         offset;
        u32         size;
        u32         absoluteOffset;
        u32         paddedSize;
        eDataFormat fmt;
        eOther      other;
        PushConstantNode    *figlio;
        PushConstantNode    *fratello;

    friend SPVReflect;
    };

private:

    bool        priv_SpvReflectFormat_to_eDataFormat (SpvReflectFormat fmtIN, eDataFormat *out_fmt) const;
    bool        priv_parse_vtxShader (SpvReflectShaderModule *module);
    bool        priv_parse_vtxShader_vtxDecl (SpvReflectShaderModule *module);
    bool        priv_parse_pushConstant (SpvReflectShaderModule *module);
    bool        priv_parse_fragShader (SpvReflectShaderModule *module);
    bool        priv_parse_descriptors (SpvReflectShaderModule *module);
    eDataFormat  priv_fromSPVReflectTypeDescrToDataFormat (const SpvReflectTypeDescription *strTypeDescr) const;

    PushConstantNode* priv_parseVar (const SpvReflectShaderModule *module, const SpvReflectBlockVariable *var);
    void        print (const PushConstantNode *node) const;

private:
    gos::Allocator      *localAllocator;
    VtxDeclList         vtxDeclList;
    PushConstantList    pushConstantList;
    DescrSetList        descrSetList;

    PushConstantNode    *pushConstant_root;
     
};

#endif //_SPVReflect_h_