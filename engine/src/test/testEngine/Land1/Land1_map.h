#ifndef _Land1_map_h_
#define _Land1_map_h_
#include "Land1_Exa.h"
#include "Land1_exaGenerator.h"
#include "../gosGameUtils/examap/gosExamap.h"
#include "gosHashMap.h"

namespace Land1
{
	/*******************************
	* Map
	* 
	*/
	class Map
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
			const Exa*			get_exa_by_index (u32 i) const			{ return exaList(i); }
			gos::examap::Coord	get_coord_by_index (u32 i) const		{ return coordList(i); }

		private:
			void	priv_reset()										{ coordList.reset(); exaList.reset(); }

		private:
			gos::FastArray<gos::examap::Coord>	coordList;
			gos::FastArray<const Land1::Exa*>	exaList;

		friend Map;
		};

	public:
				Map();
				~Map()																{ unsetup(); }

		void	setup (gos::Allocator *allocator);
		void	unsetup();

				//crea una nuova mappa "circolare" di raggio <map_radius>
				//Ogni exa ha raggio <exa_radius_world> in word coordinate
		void	map_create (f32 exa_radius_world, u32 map_radius);


		//======================= query
		void				query_visible_exa (Result *out) const;
		bool				exa_query (const gos::examap::Coord &c, const Exa **out) const;

		gos::vec3f 			exa_coord_to_world (const gos::examap::Coord &hex_coord) const			{ return exacc.exa_coord_to_world(hex_coord); }
		gos::examap::Coord	world_coord_to_exa (const gos::vec3f &world_coord) const				{ return exacc.world_coord_to_exa (world_coord); }
		gos::examap::Coord	world_coord_to_exa (f32 x, f32 z) const									{ return exacc.world_coord_to_exa (x,z); }
		f32					get_exa_world_radius() const											{ return exacc.get_exa_world_radius(); }
		gos::vec3f			get_map_world_center() const											{ return exacc.get_map_world_center(); }
	
	private:
		typedef gos::FastHashMap<u32, Exa*>	HASHMAP;

	private:
		Exa*		priv_exa_alloc (ExaGenerator &exagen, const gos::vec3f &world_coord);
		void		priv_exa_calc_v2 (const Land1::ExaGenerator &exagen, Exa *exa) const;
		void		priv_exa_free (Exa *exa);
		void		priv_exa_add_to_map (const gos::examap::Coord &coord, Exa *exa);
		void		priv_map_destroy();
		Exa*		priv_exa_get (const gos::examap::Coord &c) const;

	private:
		gos::Allocator				*localAllocator;
		gos::examap::CoordConverter	exacc;
		HASHMAP						exaList;
	};
} //namespace Land1

#endif //_Land1_map_h_