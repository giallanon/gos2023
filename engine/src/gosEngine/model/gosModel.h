#ifndef _gosModel_h_
#define _gosModel_h_
#include "../gosShape/skeleton/gosSkeleton.h"
#include "../gosEngineEnumAndDefine.h"

namespace gos
{
    class Engine;

    /*******************************
     * @brief   Model
     * 
     * Un model in sostanza e' un raggruppamento di shape, material e skeleton.
     * 
     * E' un blob di memoria formattato come segue:
     *      0       u32     magic
     *      4       u32     total_size_of_blob
     *      8       u16     num_shape
     *      10      u16     num_material
     *      12      u16     num_meshes
     *      14      u16     abs-offset-to MESH 1
     * 
     *      16      u32     ENGSkeleton as u32
     *      20      u32     ENGGPUShape-1 as u32
     *      ..  
     *      ..      u32     ENGGPUShape-N as u32
     * 
     *      ..      u16     shape_index             //MESH 1    (aka ABS_OFFSET_of_MESH1)
     *      ..      u16     bone_index
     *      ..      u16     material_index
     *      ..      u16     pad
     *      ..
     *      ..      u16     shape_index             //MESH N
     *      ..      u16     bone_index
     *      ..      u16     material_index
     *      ..      u16     pad
     */


    struct Model
    {
    public:
        struct Mesh
        {
            u16 shape_index;
            u16 bone_index;
            u16 material_index;
            u16 pad;
        };

    public:
        gos::Allocator  *allocator;
        u8              *blob;

	public:
		void    reset()		{ allocator=NULL; blob=NULL; }
    };


    
    namespace model
    {
        bool    isValid (const Model &sk);
        void    free (Model &sk);


        /*******************************
         * @brief   model::Builder
         *          Classe di comodo per la costruzione di Model
         */
        class Builder
        {
        public:    
                        Builder ();
                        ~Builder();

            Builder&    begin (Engine *eng);
            Builder&    skeleton_set (ENGSkeleton handle);
            Builder&    mesh_add (ENGGPUShape handle_shape, u32 material_index, const char *boneName);
            bool        end (gos::Allocator *allocator, Model *out);

            bool        anyErr() const                          { return bAnyErr; }

        private:
            struct Mesh
            {
                u32 shape_index;
                u32 bone_index;
                u32 material_index;
            };

        private:
            ENGSkeleton             handle_skeleton;
            Engine                  *eng;
            FastArray<ENGGPUShape>  listof_shape;
            FastArray<Mesh>         listof_mesh;
            bool                    bAnyErr;
        };         


        /*******************************
         * @brief   ModelReader
         *          Classe di comodo per la lettura delle info di un modello
         */
        class Reader
        {
        public:
                                Reader()                        { m = NULL; }
                                Reader(const Model *m)          { setup(m); }
                                ~Reader()                       { }

            void                setup (const Model *m);

            ENGSkeleton         skeleton_get_handle() const;

            u32                 mesh_get_num() const;
            const Model::Mesh*  mesh_get_by_index (u32 index) const;

            u32                 shape_get_num() const;
            ENGGPUShape         shape_get_by_index (u32 i) const;

            u32                 material_get_num() const;

        private:
            const Model *m;
        };

        /******************************************
         * @brief   Model
         *          E' composta da uno <Skeleton> a da <numMeshes> Mesh
         *          E' creato tramite un model::Builder
         */
 /*       class Model
        {
        public:
                            ~Model();

            const Skeleton* skeleton_query() const;

            u32             mesh_getNum() const                 { return numMeshes; }
            const Mesh*     mesh_query(u32 i) const             { assert(i<numMeshes); return &meshList[i]; }
            

        private:
                            Model (gos::Allocator *allocatorIN, Engine *engIN, ENGSkeleton handle_skeleton, const Mesh *meshList, u32 numMeshes);

        private:
            gos::Allocator  *allocator;
            Engine          *eng;
            ENGSkeleton     handle_skeleton;
            Mesh            *meshList;
            u32             numMeshes;

        friend model::Builder;
        }; */



        /******************************************
         * @brief   ModelInstance
         * 
         * 
         */
        //class ModelInstance
        //{
        //public:
        //                                ModelInstance ()                                            { model=NULL; sk=NULL; }
        //                                ~ModelInstance()                                            { priv_free(); }

        //    void                        setup (const Model *modelIN)                                { model = modelIN; sk = model->skeleton_query()->newInstance(); }
        //    void                        unsetup()                                                   { priv_free(); }
        //    void                        applyTransform (const mat4x4f &matW)                        { sk->applyTransform(matW); }

        //    SkeletonInstance*           skeleton_get ()                                             { return sk; }
        //    const SkeletonInstance*     skeleton_query () const                                     { return sk; }
        //    u32                         skeleton_getNumBones() const                                { return sk->bone_getNum(); }
        //    Bone*                       skeleton_getBoneByIndex (u32 index)                         { return sk->getBoneByIndex(index); }

        //    u32                         meshList_getNumElem() const                                 { return model->mesh_getNum(); }
        //    const Mesh*                 meshList_getByIndex(u32 i) const                            { return model->mesh_query(i); }

        //private:
        //    void                        priv_free()                                                 { SkeletonInstance::free (sk); }

        //private:
        //    const Model         *model;
        //    SkeletonInstance    *sk;
        //};     

    } //namespace model
} //namespace gos


#endif //_gosModel_h_