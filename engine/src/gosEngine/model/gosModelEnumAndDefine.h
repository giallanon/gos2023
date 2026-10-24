#ifndef _model_gosModelEnumAndDefine_h_
#define _model_gosModelEnumAndDefine_h_
#include "../../gos/gos.h"

namespace gos
{
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

}//namespace gos
#endif //_model_gosModelEnumAndDefine_h_

