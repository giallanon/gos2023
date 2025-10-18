#ifndef _gosSkeleton_h_
#define _gosSkeleton_h_
#include "../gosEngineEnumAndDefine.h"

namespace gos
{
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
     * @brief   Skeleton
     * 
     */
    class Skeleton
    {
    public:
                            Skeleton()                                         { allocator = NULL; }
                            ~Skeleton()                                        { priv_free(); }

        const Bone*         getBoneList() const                                     { return boneList; };
        u32                 getNumBones() const                                     { return numBones; }
        Bone*               getBoneByName (const char *name) const;
        u32                 getBoneIndexByName (const char *name) const;

        SkeletonInstance*   newInstance();
        gos::Allocator*     getAllocator() const                                    { return allocator; }

    private:
        void                priv_alloc (gos::Allocator *allocatorIN, u32 numBones);
        void                priv_free();

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

        Bone*       getBoneList() const                                         { return boneList; };
        u32         getNumBones() const                                         { return numBones; }
        Bone*       getBoneByName (const char *name) const;
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

    
    
    /*******************************
     * @brief   SkeletonBuilder
     * 
     */
    class SkeletonBuilder
    {
    public:    
                SkeletonBuilder();
                ~SkeletonBuilder();

        u32     begin (const char *rootName);

                //crea una nuova Bone con nome <dstBoneName> e la adda come figlio di <srcBoneIndex>.
                //Ritorna l'index della nuova bone creata
        u32     addChildTo (u32 srcBoneIndex, const char *dstBoneName, Bone **out_canBeNULL = NULL);

                //crea una nuova Bone con nome <dstBoneName> e la adda come figlio di <srcBoneIndex>.
                //Ritorna l'index della nuova bone creata
        u32     addSiblingdTo (u32 srcBoneIndex, const char *dstBoneName, Bone **out_canBeNULL= NULL);

                //filla <out> creando la struttura di bones definita dal builder.
                //Usa <allocator> per allocare le bone di <out>
        void    end (gos::Allocator *allocator, Skeleton *out);

    private:
        u32     priv_newBone (const char *name);

    private:
        u8                      numBones;
        gos::StringList         nameList;
        gos::FastArray<Bone>    boneList;

    };
} //namespace gos

#endif //_gosSkeleton_h_

