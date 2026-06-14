#ifndef _gosModelInstance_h_
#define _gosModelInstance_h_
#include "gosModel.h"

namespace gos
{
	/*************************************
	 * @brief	ModelInstance
	 * 			Nasce da un <model> e condivide le stesste GPUShape del model padre
	 * 			Ha inoltre una istanza privata dello skeleont del model padre
	 */
	class ModelInstance
	{
	public:
		void    reset()		{ allocator=NULL; num_gpushapes=num_bones=num_meshes=0; listof_gpushapes=NULL; listof_bones=NULL; listof_meshes=NULL; model_listof_bones=NULL; }
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

		gos::Allocator		*allocator;
	};




} //namespace gos



#endif //_gosModelInstance_h_

