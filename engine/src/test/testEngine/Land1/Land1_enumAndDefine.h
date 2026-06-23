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
		gos::vec2f	calc_quad_center (u32 quad_index) const;
		bool		get_quad_from_point (const gos::vec3f &world_point, u32 *out__quad_index) const;

	public:
		u16			num_vtx;
		u16			num_quad;
		gos::vec2f	*vtxList;
		Quad		*quadList;


	private:
		bool		priv_is_point_in_quad (const gos::vec3f &world_point, u32 quad_index) const;
	};



} //namespace Land1

#endif //_Land1_enumAndDefine_h_


