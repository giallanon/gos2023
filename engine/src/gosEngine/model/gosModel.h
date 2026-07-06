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
	 *      16      u16     abs-offset-to MATERIAL 1
     *      18      u16     abs-offset-to NAME-TABLE
     * 
     *      20      u32     ENGSkeleton as u32
     
     *      24      u32     ENGGPUShape-1 as u32        //voglio che inizi ad un indirizzo multiplo di 8
     *      ..  
     *      ..      u32     ENGGPUShape-N as u32
	 * 
     *      ..      pad
     
	 * 		..		u32		ENGMaterialPBR-1 as u32		//MATERIAL 1 (aka ABS_OFFSET_of_MATERIAL) (voglio che inizi ad un indirizzo multiplo di 8)
	 * 		..
	 * 		..		u32		ENGMaterialPBR-N as u32
     * 
     *      ..      pad
     *                                                  
     *      ..      sizeof(Mesh)	mesh-0     			//MESH 1    (aka ABS_OFFSET_of_MESH1)  (voglio che inizi ad un indirizzo multiplo di 8)
     *      ..                                          
     *      ..      sizeof(Mesh)	mesh-N
     * 
     *      ..      pad
     *      ..      32B     name-0                      //voglio che inizi ad un indirizzo multiplo di 8
     *      ..                                          //elenco di tutti i nomi shape, meterial, mesh, ciascuno lungo 32B 0x00  incluso
     *      ..      32B     name-N                      //i primi num_shape nomi sono quelli dell shape, a seguire num_material nomi e poi num_mesh nomi
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
		bool 	alloc (gos::Allocator *allocator, u16 num_shape, u16 num_material, u16 num_meshes, Model *out);
		void    free (Model &sk);

		bool	set_skeleton (Model &m, ENGSkeleton handle);
		bool	set_gpushape (Model &m, u32 shape_num, ENGGPUShape handle, const char *name__can_be_NULL = NULL);
		bool 	set_material (Model &m, u32 material_num, ENGMaterialPBR handle, const char *name__can_be_NULL = NULL);
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

            ENGSkeleton         skeleton_get_handle() const;

            u32                 mesh_get_num() const;
            const Model::Mesh*  mesh_get_by_index (u32 index) const;

            u32                 gpushape_get_num() const;
            ENGGPUShape         gpushape_get_by_index (u32 i) const;
			const ENGGPUShape*  gpushape_get_pt_to_list () const;

            u32                 material_get_num() const;
            ENGMaterialPBR         material_get_by_index (u32 i) const;
			const ENGMaterialPBR*  material_get_pt_to_list () const;

        private:
            const Model *m;
        };
	

    } //namespace model
} //namespace gos


#endif //_gosModel_h_