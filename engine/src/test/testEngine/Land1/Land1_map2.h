#ifndef _Land1_map2_h_
#define _Land1_map2_h_
#include "Land1_enumAndDefine.h"
#include "Land1_exaGenerator.h"
#include "../gosGameUtils/examap/gosExamap.h"
#include "gosHashMap.h"

namespace Land1
{
	/*******************************
	* Map2
	*
	*/
	class Map2
	{
	public:
		struct Vtx
		{
			gos::vec2f	pos;
			u8 	material_index;
			u8	num_adj_vtx;
			u16	height;
			GVC	coord;
			GVC	connected_vtx[7];		//indici dei vtx "originali" che sono connessi a questo vtx
		};

	public:
				Map2();
				~Map2()																{ unsetup(); }

		void	setup (gos::Allocator *allocator);
		void	unsetup();

		//======================= map creation
		void	map_create (f32 exa_radius_world, u32 random_seed);

				//crea un nuovo exa e lo adda alla mappa in posizione <coord>
		void	exa__add (const gos::examap::Coord coord);

		//======================= utils
		bool	get_list_of_vtx_by_exa (const gos::examap::Coord &exa_coord, gos::FastArray<Vtx> &out, bool bClearOut=true) const;

		//======================= query
		gos::vec3f 			exa_coord_to_world (const gos::examap::Coord &exa_coord) const			{ return exacc.exa_coord_to_world(exa_coord); }
		gos::examap::Coord	world_coord_to_exa (const gos::vec3f &world_coord) const				{ return exacc.world_coord_to_exa (world_coord); }
		gos::examap::Coord	world_coord_to_exa (f32 x, f32 z) const									{ return exacc.world_coord_to_exa (x,z); }
		f32					get_exa_world_radius() const											{ return exacc.get_exa_world_radius(); }
		gos::vec3f			get_map_world_center() const											{ return exacc.get_map_world_center(); }

	private:
		struct HexInfo
		{
			gos::examap::Coord coord;
		};



	private:
		typedef gos::FastHashMap<u32, Vtx>			VTXMAP;
		typedef gos::FastHashMap<u32, HexInfo>		HEXMAP;


	private:
		void 	priv_destroy_map();
		bool	priv_vtxmap__add_vtx (const GVC gvc, const Vtx &vtx);
		bool 	priv_vtxmap__get_vtx (const GVC gvc, Vtx *out) const;



	private:
		gos::Allocator				*localAllocator;
		gos::Random					rnd;
		gos::examap::CoordConverter	exacc;
		VTXMAP						vtxmap;
		HEXMAP						hexmap;

	};
} //namespace Land1

#endif //_Land1_map2_h_

