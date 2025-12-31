#ifndef _gosModel_h_
#define _gosModel_h_
#include "gosSkeleton.h"
#include "../gosEngineEnumAndDefine.h"

namespace gos
{
    namespace model
    {
        class Model;
        class ModelInstance; //fwd

        struct Mesh
        {
            gos::ENGGPUShape   shape_handle;
            u16             bone_index;
            u16             material_index;
        };        


        /*******************************
         * @brief   model::Builder
         *          Classe di comodo per la costruzione di Model
         */
        class Builder
        {
        public:    
                    Builder (u32 preallocNumMesh = 64);
                    ~Builder();

            void    begin (Skeleton *skeleton);
            void    addMeshToBone (gos::ENGGPUShape shape, u32 material_index, const char *boneName);
            Model*  end (gos::Allocator *allocator);

        private:
            Skeleton        *skeleton;
            FastArray<Mesh> meshList;
        };         


        /******************************************
         * @brief   Model
         *          E' composta da uno <Skeleton> a da <numMeshes> Mesh
         *          E' creato tramite un model::Builder
         */
        class Model
        {
        public:
                            ~Model();

            const Skeleton* skeleton_query() const              { return skeleton; }

            u32             mesh_getNum() const                 { return numMeshes; }
            const Mesh*     mesh_query(u32 i) const             { assert(i<numMeshes); return &meshList[i]; }
            

        private:
                            Model (gos::Allocator *allocatorIN, Skeleton *sk, const Mesh *meshList, u32 numMeshes);

        private:
            gos::Allocator  *allocator;
            Skeleton        *skeleton;
            Mesh            *meshList;
            u32             numMeshes;

        friend model::Builder;
        }; 



        /******************************************
         * @brief   ModelInstance
         * 
         * 
         */
        class ModelInstance
        {
        public:
                                        ModelInstance ()                                            { model=NULL; sk=NULL; }
                                        ~ModelInstance()                                            { priv_free(); }

            void                        setup (const Model *modelIN)                                { model = modelIN; sk = model->skeleton_query()->newInstance(); }
            void                        unsetup()                                                   { priv_free(); }
            void                        applyTransform (const mat4x4f &matW)                        { sk->applyTransform(matW); }

            SkeletonInstance*           skeleton_get ()                                             { return sk; }
            const SkeletonInstance*     skeleton_query () const                                     { return sk; }
            u32                         skeleton_getNumBones() const                                { return sk->bone_getNum(); }
            Bone*                       skeleton_getBoneByIndex (u32 index)                         { return sk->getBoneByIndex(index); }

            u32                         meshList_getNumElem() const                                 { return model->mesh_getNum(); }
            const Mesh*                 meshList_getByIndex(u32 i) const                            { return model->mesh_query(i); }

        private:
            void                        priv_free()                                                 { SkeletonInstance::free (sk); }

        private:
            const Model         *model;
            SkeletonInstance    *sk;
        };     

    } //namespace model
} //namespace gos


#endif //_gosModel_h_