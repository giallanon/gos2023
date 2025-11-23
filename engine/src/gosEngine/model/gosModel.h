#ifndef _gosModel_h_
#define _gosModel_h_
#include "gosSkeleton.h"
#include "../gosEngineEnumAndDefine.h"

namespace gos
{
    namespace model
    {
        class ModelInstance; //fwd

        struct Mesh
        {
            gos::ENGShape   shape_handle;
            u16             bone_index;
            u16             material_index;
        };        

        /******************************************
         * @brief   Model
         * 
         * 
         */
        class Model
        {
        public:
                        Model();
                        ~Model()                                                    { priv_free(); }

            void        setSkeleton (Skeleton *sk)                                  { this->skeleton = sk; }
            void        addMesh (gos::ENGShape shape, u32 material_index, const char *boneName);

        private:
            void        priv_free();

        private:
            gos::Allocator              *allocator;
            Skeleton                    *skeleton;
            FastArray<Mesh>             meshList;

        friend ModelInstance;
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

            void                        setup (const Model *modelIN)                                { model = modelIN; sk = model->skeleton->newInstance(); }
            void                        unsetup()                                                   { priv_free(); }
            void                        applyTransform (const mat4x4f &matW)                        { sk->applyTransform(matW); }

            const SkeletonInstance*     skeleton_get () const                                       { return sk; }
            u32                         skeleton_getNumBones() const                                { return sk->getNumBones(); }
            const Bone*                 skeleton_getBoneByIndex (u32 index) const                   { return sk->getBoneByIndex(index); }

            u32                         meshList_getNumElem() const                                 { return model->meshList.getNElem(); }
            const Mesh*                 meshList_getByIndex(u32 i) const                            { return &model->meshList.queryElem(i); }

        private:
            void                        priv_free()                                                 { SkeletonInstance::free (sk); }

        private:
            const Model         *model;
            SkeletonInstance    *sk;
        };     

    } //namespace model
} //namespace gos


#endif //_gosModel_h_