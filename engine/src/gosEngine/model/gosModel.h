#ifndef _gosModel_h_
#define _gosModel_h_
#include "gosModelEnumAndDefine.h"
#include "../gosShape/skeleton/gosSkeleton.h"
#include "../gosEngineEnumAndDefine.h"

namespace gos
{
    class Engine;

    namespace model
    {
        bool    isValid (const Model &sk);
		bool 	alloc (gos::Allocator *allocator, u16 num_shape, u16 num_material, u16 num_meshes, Model *out);
		void    free (Model &sk);

		bool	set_skeleton (Model &m, SkeletonH handle);
		bool	set_gpushape (Model &m, u32 shape_num, GPUShapeH handle, const char *name__can_be_NULL = NULL);
		bool 	set_material (Model &m, u32 material_num, MaterialPBRH handle, const char *name__can_be_NULL = NULL);
		bool 	set_mesh  (Model &m, u32 mesh_num, u16 shape_index, u16 bone_index, u16 material_index, const char *name__can_be_NULL = NULL);


        /*******************************
         * @brief   ModelReader
         *          Classe di comodo per la lettura delle info di un modello
         */
        class Reader
        {
		public:
            static constexpr u32 SIZE_OF_A_NAME = 32;
			static constexpr u32 NUM_SHAPES = 8;
			static constexpr u32 NUM_MATERIAL = 10;
			static constexpr u32 NUM_MESHES = 12;
			static constexpr u32 OFFSET_TO_START_OF_MESHES = 14;
			static constexpr u32 OFFSET_TO_START_OF_MATERIALS = 16;
            static constexpr u32 OFFSET_TO_START_OF_NAME_TABLE = 18;
			static constexpr u32 SKELETON	= 20;
			static constexpr u32 ENGSHAPE = 24;

        public:
                                Reader()                        { m = NULL; }
                                Reader(const Model *m)          { setup(m); }
                                ~Reader()                       { }

            void                setup (const Model *m);

            SkeletonH         	skeleton_get_handle() const;

            u32                 mesh_get_num() const;
            const Model::Mesh*  mesh_get_by_index (u32 index) const;

            u32                 gpushape_get_num() const;
            GPUShapeH         	gpushape_get_by_index (u32 i) const;
			const GPUShapeH*  	gpushape_get_pt_to_list () const;

            u32                 material_get_num() const;
            MaterialPBRH        material_get_by_index (u32 i) const;
			const MaterialPBRH* material_get_pt_to_list () const;

        private:
            const Model *m;
        };
	

    } //namespace model
} //namespace gos


#endif //_gosModel_h_