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
		struct Node
		{
			gos::vec2f	pos;
			u8 	material_index;
			u8	num_adj_vtx;
			u16	height;
			GVC	coord;
			GVC	connected_vtx[6];
			gos::vec2f	quad_center[6];
		};

		struct Vtx
		{
			gos::vec2f	pos;
			u8 	material_index;
			u8	num_adj_vtx;
			u16	height;
			u16	adj_vtx_list[6];
			GVC	coord;
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
		void	exa__add_with_radius (const gos::examap::Coord center_coord, u32 radius);

		//======================= utils
				//filla <outList> con tutti i vtx necessari a renderizzare l'exa.
				//Ritorna il numero di "vertici originali" dell'exa. Ogni vtx e' collegato ad altri vertici ma non tutti i vtx
				//fanno parte di <exa_coord>. Il num di vtx ritornato e' il num di vtx di <outList> che fanno effettivamente
				//parte dell'exa
		u32		get_exa_vtxList (const gos::examap::Coord &exa_coord, gos::FastArray<Vtx> &outList, bool bClear_outList=true) const;
		

		//======================= query
		gos::vec3f 			exa_coord_to_world (const gos::examap::Coord &exa_coord) const			{ return exacc.exa_coord_to_world(exa_coord); }
		gos::examap::Coord	world_coord_to_exa (const gos::vec3f &world_coord) const				{ return exacc.world_coord_to_exa (world_coord); }
		gos::examap::Coord	world_coord_to_exa (f32 x, f32 z) const									{ return exacc.world_coord_to_exa (x,z); }
		f32					get_exa_world_radius() const											{ return exacc.get_exa_world_radius(); }
		gos::vec3f			get_map_world_center() const											{ return exacc.get_map_world_center(); }
		bool				world_coord_to_GVC  (const gos::vec3f &world_coord, GVC *out) const;
		bool				GVC_to_world_coord  (const GVC gvc, gos::vec3f *out_world_coord) const;
		bool				GVC_to_node (const GVC gvc, Node *out) const;

	private:
		struct HexInfo
		{
			gos::examap::Coord	coord;
			u16					num_vtx;
		};



	private:
		typedef gos::FastHashMap<GVC, Node>			Nodemap;
		typedef gos::FastHashMap<gos::examap::Coord, HexInfo>		EXAMAP;


	private:
		void 	priv_destroy_map();
		bool	priv_vtxmap__add_vtx (const GVC gvc, const Node &vtx);
		bool 	priv_vtxmap__get_vtx (const GVC gvc, Node *out) const;
		void	priv_node_to_vtx (const Node &node, Vtx *out) const;
		void	priv_node_to_vtx (const GVC gvc, Vtx *out) const;
		void	priv_node__update_quad_center (const GVC gvc);



	private:
		gos::Allocator				*localAllocator;
		gos::Random					rnd;
		gos::examap::CoordConverter	exacc;
		Nodemap						nodemap;
		EXAMAP						examap;

	};
} //namespace Land1

#endif //_Land1_map2_h_

