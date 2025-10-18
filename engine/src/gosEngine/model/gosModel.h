#ifndef _gosModel_h_
#define _gosModel_h_
#include "gosSkeleton.h"
#include "../gosEngineEnumAndDefine.h"

namespace gos
{
    namespace model
    {
        class ModelInstance; //fwd

        struct ShapeAndBoneLink
        {
            u16     shapeIndex;
            u16     boneIndex;
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
            void        addShape (gos::ENGShape handle);

            void        linkShapeToBone (gos::ENGShape shape, const char *boneName);

        private:
            void        priv_free();

        private:
            gos::Allocator              *allocator;
            Skeleton                    *skeleton;
            FastArray<gos::ENGShape>    shapeList;
            FastArray<ShapeAndBoneLink> shapeAndBoneLinkList;

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
                                        ModelInstance (const Model *modelIN);
                                        ~ModelInstance()                                            { priv_free(); }

            void                        applyTransform (const mat4x4f &matW)                        { sk->applyTransform(matW); }

            const SkeletonInstance*     getSkeleton () const                                        { return sk; }
            u32                         getNumBones() const                                         { return sk->getNumBones(); }
            const Bone*                 getBoneByIndex (u32 index) const                            { return sk->getBoneByIndex(index); }

            const ShapeAndBoneLink*     getShapeAndBoneList() const                                 { return model->shapeAndBoneLinkList._queryTypedPointer(); }
            gos::ENGShape               getShapeByIndex (u32 index) const                           { return model->shapeList(index); }

        private:
            void    priv_free();

        private:
            const Model         *model;
            SkeletonInstance    *sk;
        };     

    } //namespace model
} //namespace gos


#endif //_gosModel_h_