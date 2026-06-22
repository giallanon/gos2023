#ifndef _Land1_enumAndDefine_h_
#define _Land1_enumAndDefine_h_
#include "../gosGameUtils/examap/gosExamap.h"

namespace Land1
{
	//******************************************
	struct Exa
	{
	public:
		struct Quad
		{
			u8	idx[4];
			f32 height;
			u32 material_index;
		};

	public:
		u16			num_vtx;
		u16			num_quad;
		gos::vec2f	*vtxList;
		Quad		*quadList;

	public:
		gos::vec2f	calc_quad_center (u32 quad_number) const
		{
			gos::vec2f ret = vtxList[quadList[quad_number].idx[0]]
				+ vtxList[quadList[quad_number].idx[1]]
				+ vtxList[quadList[quad_number].idx[2]]
				+ vtxList[quadList[quad_number].idx[3]];
			ret /= 4.0f;
			return ret;
		}
	};



} //namespace Land1

#endif //_Land1_enumAndDefine_h_


