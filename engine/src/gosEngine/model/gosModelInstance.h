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
		void    reset()		{ allocator=NULL; num_shapes=num_bones=0; listof_shapes=NULL; listof_bones=NULL; handle_model.setInvalid(); }
		void 	free()
		{
			GOSFREE(allocator, listof_shapes);
			GOSFREE(allocator, listof_bones);
			reset;
		}
		
	public:
		u32 				num_shapes;
		ENGGPUShape			*listof_shapes;

		u32 				num_bones;
		gos::Bone			*listof_bones;

		gos::Allocator		*allocator;
		const ENGModel3d	handle_model;
	};




} //namespace gos



#endif //_gosModelInstance_h_

