#ifndef _gosModelInstance_h_
#define _gosModelInstance_h_
#include "gosModel.h"

namespace gos
{
	/*************************************
	 * @brief	ModelInstance
	 * 			Nasce da un <model> e condivide le stesse GPUShape del model padre
	 * 			Ha inoltre una istanza privata dello skeleton del model padre
	 * 			Condivide anche gli stessi materiali
	 * 
	 * 			Le istanze sono create da engine->modelinst_create()
	 */
	class ModelInstance
	{
	public:
		void    reset()		{ 
			allocator=NULL; num_gpushapes=num_bones=num_meshes=0; listof_gpushapes=NULL; listof_bones=NULL; listof_meshes=NULL; model_listof_bones=NULL; 
			num_materials = 0; listof_materials = NULL;
		}
		void 	free()
		{
			GOSFREE(allocator, listof_bones);
			reset();
		}
		
	public:
		u32 				num_gpushapes;
		const ENGGPUShape	*listof_gpushapes;	//punta alla lista di gpushape di <model>

		u32 				num_bones;
		gos::Bone			*listof_bones;
		const gos::Bone		*model_listof_bones;	//punta alla lista di bones di <model::skeleton>

		u32 				num_meshes;
		const Model::Mesh	*listof_meshes;	//punta alla lista di mesh di <model>

		u32 				num_materials;
		const ENGMaterialPBR	*listof_materials;	//punta alla lista di ENGMaterialPBR di <model>
		
		gos::Allocator		*allocator;
	};




} //namespace gos



#endif //_gosModelInstance_h_

