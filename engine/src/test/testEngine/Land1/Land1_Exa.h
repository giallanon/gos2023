#ifndef _Land1_Exa_h_
#define _Land1_Exa_h_
#include "Land1_enumAndDefine.h"

namespace Land1
{
	//******************************************
	struct Exa
	{
	public:
		struct VtxInfo
		{
			u8	num_quad;		//num quad centrati sul vtx i-esimo
			u8 	material_index;
			u8	num_idx;		//num idx in idx_list
			u8	is_border_vtx;
			u16	height;
			u16	idx_list[16];		//idx0=centro, gli altri 2*num_quad in senso orario
			u16	adj_vtx_list[8];
		};


	public:
					//dato un punto in world coordinate, ritorna (se esiste) l'indice del vtx + vicino
		bool		get_closest_vtx_from_point (const gos::vec3f &world_point, u32 *out__vtx_index) const;
		
	public:
		u16			num_vtx_originali;	//quelli che compongono l'exa originale
		u16			num_vtx_tot;		//tutti quelli che sono in vtxList
		VtxInfo		*vtxInfoList;		//una VtxInfo per ogni vtx-originale
		gos::vec2f	*vtxList;			//tutti i vtx utili al rendering


	private:
		bool		priv_is_point_in_tris (const gos::vec3f &world_point, u16 idx0, u16 idx1, u16 idx2) const;
		bool		priv_is_point_in_quad (const gos::vec3f &world_point, u16 idx0, u16 idx1, u16 idx2, u16 idx3) const;
		bool		priv_is_point_in_penta (const gos::vec3f &world_point, u16 idx0, u16 idx1, u16 idx2, u16 idx3, u16 idx4) const;
		bool		priv_is_point_in_exagon (const gos::vec3f &world_point, u16 idx0, u16 idx1, u16 idx2, u16 idx3, u16 idx4, u16 idx5) const;
		
	};

} //namespace Land1

#endif //_Land1_Exa_h_

