#ifndef _Land1_map2_h_
#define _Land1_map2_h_
#include "Land1_enumAndDefine.h"
#include "Land1_exaGenerator.h"
#include "../gosGameUtils/examap/gosExamap.h"
#include "gosHashMap.h"

namespace Land1
{
	/*******************************
	* Exa2
	*
	*/
	struct Exa2
	{
	public:
		struct VtxInfo
		{
			u8	num_quad;		//num quad centrati sul vtx i-esimo
			u8 	material_index;
			u8	num_idx;		//num idx in idx_list
			u8	is_border_vtx;
			u16	height;
			u16	idx_list[16];			//idx0=centro, gli altri 2*num_quad in senso orario
			eMeshType	mesh_type[8];	//uno per ogni quad
			GVC	connected_vtx[8];		//indici dei vtx "originali" che sono connessi a questo vtx
		};

	public:
		gos::examap::Coord coord;		//coordinata di questo exa
		u16			num_vtx_originali;	//quelli che compongono l'exa originale
		u16			num_vtx_tot;		//tutti quelli che sono in vtxList
		VtxInfo		*vtxInfoList;		//una VtxInfo per ogni vtx-originale
		gos::vec2f	*vtxList;			//tutti i vtx utili al rendering

	};

	/*******************************
	* Map2
	*
	*/
	class Map2
	{
	public:
		/**********************************
		* Result
		* 
		* risultato di una query di visibilita'
		*/
		class Result
		{
		public:
					Result()											{ }
					~Result()											{ unsetup(); }

			void	setup (gos::Allocator *allocator)					{ coordList.setup(allocator, 128); exaList.setup(allocator, 128); }
			void	unsetup()											{ coordList.unsetup(); exaList.unsetup(); }
			
			u32					get_num() const							{ return coordList.getNElem(); }
			const Exa2*			get_exa_by_index (u32 i) const			{ return exaList(i); }
			gos::examap::Coord	get_coord_by_index (u32 i) const		{ return coordList(i); }

		private:
			void	priv_reset()										{ coordList.reset(); exaList.reset(); }

		private:
			gos::FastArray<gos::examap::Coord>	coordList;
			gos::FastArray<const Land1::Exa2*>	exaList;

		friend Map2;
		};

	public:
				Map2();
				~Map2()																{ unsetup(); }

		void	setup (gos::Allocator *allocator);
		void	unsetup();

		void	map_create (f32 exa_radius_world, u32 random_seed);

				//crea un nuovo exa e lo adda alla mappa in posizione <coord>
		void	exa__add (const gos::examap::Coord coord);

		//==================== query
		void				query_visible_exa (Result *out) const;
		bool				exa_query (const gos::examap::Coord &c, const Exa2 **out) const;
		gos::vec3f 			exa_coord_to_world (const gos::examap::Coord &hex_coord) const			{ return exacc.exa_coord_to_world(hex_coord); }
		gos::examap::Coord	world_coord_to_exa (const gos::vec3f &world_coord) const				{ return exacc.world_coord_to_exa (world_coord); }
		gos::examap::Coord	world_coord_to_exa (f32 x, f32 z) const									{ return exacc.world_coord_to_exa (x,z); }
		f32					get_exa_world_radius() const											{ return exacc.get_exa_world_radius(); }
		gos::vec3f			get_map_world_center() const											{ return exacc.get_map_world_center(); }
		bool				get_vertex_from_GVC (const GVC &gvc, gos::vec2f *out) const;

	private:
		typedef gos::FastHashMap<u32, Exa2*>	HASHMAP;

	private:
		void	priv_destroy_map();
		void	priv_add_exa_to_map(Exa2 *exa);
		Exa2*	priv_exa_get (const gos::examap::Coord &c) const;
		void	priv_exa_free (Exa2 *exa);
		bool	priv_find_adj_exa_with_shared_vtx (const GVC gvc, GVC *out) const;
		bool	priv_find_adj_exa_with_shared_vtx (gos::examap::Coord exa_cood, gos::vec2f v, GVC *out) const;

	private:
		gos::Allocator				*localAllocator;
		gos::Random					rnd;
		gos::examap::CoordConverter	exacc;
		HASHMAP						exaList;

	};
} //namespace Land1

#endif //_Land1_map2_h_

