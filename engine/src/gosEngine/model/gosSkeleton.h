#ifndef _gosSkeleton_h_
#define _gosSkeleton_h_
#include "../gosEngineEnumAndDefine.h"

namespace gos
{
    class Skeleton; //fwd decl
    class SkeletonBuilder; //fwd decl
    class SkeletonInstance; //fwd decl


    /*******************************
     * @brief   Bone
     * 
     */
    struct Bone
    {
    public:
        mat4x4f matrix;
        u8      firstChildIndex;
        u8      sigblinIndex;
        u16     nameIndex;
    };


    /*******************************
     * @brief   SkeletonBuilder
     *          Classe di comodo per la costruzione di Skeleton
     */
    class SkeletonBuilder
    {
    public:    
                SkeletonBuilder();
                ~SkeletonBuilder();

        u32     begin (const char *rootName, Bone **out_canBeNULL = NULL);

                //crea una nuova Bone con nome <dstBoneName> e la adda come figlio di <srcBoneIndex>.
                //Ritorna l'index della nuova bone creata
        u32     addChildTo (u32 srcBoneIndex, const char *dstBoneName, Bone **out_canBeNULL = NULL);

                //crea una nuova Bone con nome <dstBoneName> e la adda come fratello di <srcBoneIndex>.
                //Ritorna l'index della nuova bone creata
        u32     addSiblingTo (u32 srcBoneIndex, const char *dstBoneName, Bone **out_canBeNULL= NULL);

        Skeleton*   end (gos::Allocator *allocator);

    private:
        u32     priv_newBone (const char *name);

    private:
        u8                      numBones;
        gos::StringList         nameList;
        gos::FastArray<Bone>    boneList;

    };    
    
    
    /*******************************
     * @brief   Skeleton
     * 
     */
    class Skeleton
    {
    public:
                            //createFromMemory
                            //ritorna NULL in caso di errore
                            //in caso di successo, ritorna una nuova istanza di Skeleton e filla <out__numByteUsed> con il num di byte di <buffer> utilizzati
        static Skeleton*    createFromMemory (gos::Allocator *allocatorIN, const u8 *buffer, u32 sizeof_buffer, u32 *out__numByteUsed);

    public:
                            ~Skeleton()                                             { priv_free(); }

                            //serialize_toMemory
                            //se [out_buffer] == NULL ritorna il num di byte necessari alla serializzazione
                            //se [out_buffer] != NULL ritorna 0 in caso di errore oppure il num di byte memcpyati in [buffer]
        u32                 serialize_toMemory (u8 *out_buffer, u32 sizeof_buffer) const;
        

        const Bone*         bone_getList() const                                     { return boneList; };
        u32                 bone_getNum() const                                     { return numBones; }
        Bone*               bone_getByName (const char *name) const;
        u32                 bone_getIndexByName (const char *name) const;

        SkeletonInstance*   newInstance() const;
        gos::Allocator*     getAllocator() const                                    { return allocator; }


        void                debug__print (gos::Logger *logger) const;


    private:
                            //una istanza di Skeleton la puoi ottenere via SkeletonBuilder oppure tramite Skeleton::createFromMemory()
                            Skeleton (gos::Allocator *allocatorIN, u32 numBones);
        void                priv_free();
        void                debug__print_rec (gos::Logger *logger, const Bone *bone) const;

    private:
        gos::Allocator      *allocator;
        gos::StringList     nameList;
        Bone                *boneList;
        u32                 numBones;

        friend SkeletonBuilder;
        friend SkeletonInstance;
    };

    
    
    /*******************************
     * @brief   SkeletonInstance
     * 
     */
    class SkeletonInstance
    {
    public:
        static void free(SkeletonInstance *&inst)                               { gos::Allocator *a = inst->model->getAllocator(); GOSDELETE(a, inst); inst = NULL; }

    public:
                    ~SkeletonInstance()                                         { priv_free(); }

        Bone*       bone_getList() const                                         { return boneList; };
        u32         bone_getNum() const                                         { return numBones; }
        Bone*       bone_getByName (const char *name) const;
        Bone*       getBoneByIndex (u32 index) const                            { assert(index < numBones); return &boneList[index]; }

        void        applyTransform (const mat4x4f &matW);

    protected:
                    SkeletonInstance (const Skeleton *model);

    private:
        void        priv_free();
        void        priv_applyTransform_ric (u32 boneIndex, const mat4x4f &parent_matW);
    
    private:
        const Skeleton      *model;
        Bone                *boneList;
        u32                 numBones;


        friend Skeleton;
    };


} //namespace gos

#endif //_gosSkeleton_h_

