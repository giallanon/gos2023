#ifndef _Land1_Exa_h_
#define _Land1_Exa_h_
#include "Land1_enumAndDefine.h"

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

					//dato un punto in world coordinate, ritorna (se esiste) l'indice del vtx + vicino
		bool		get_closest_vtx_from_point (const gos::vec3f &world_point, u32 *out__vtx_index) const;
		
					//dato un vtx_index, ritorna l'elenco dei quad che sharano lo stesso vtx
					//Ritorna il num di quad_index inseriti in <out__quadList>
		u32			get_quad_from_vtx (u32 vtx_index, u32 *out__quadList, u32 num_elem_in_quad_list) const;

	public:
		u16			num_vtx;
		u16			num_quad;
		gos::vec2f	*vtxList;
		Quad		*quadList;


	private:
		bool		priv_is_point_in_quad (const gos::vec3f &world_point, u32 quad_index) const;
	};

} //namespace Land1

#endif //_Land1_Exa_h_

