#ifndef _gosExamap_h_
#define _gosExamap_h_
#include "../gosGameUtils.h"

namespace gos
{
	/******************************************
	* vedi anche: https://www.redblobgames.com/grids/hexagons/
	* 
					 top
					(0,+1)
					/---\ 
				   /  0  \                            
			   /---\     /---\
	 left-top /  1  \___/  5  \  right-top
	  (-1,1)  \     /   \     /   (+1, 0)
			   \___/     \___/
			   /   \     /   \
	left-bott /  2  \___/  4  \  right-bottom
	  (-1, 0) \     /   \     /    (+1, -1)
			   \___/  3  \___/
				   \     /
					\___/
					bottom
					(0, -1)



					 _____
					/     \
				   /       \              
				   \       /
					\_____/
					   |---> world-radius

					 _____
					/     \
				   /       \_____        _
				   \       /     \       |
					\_____/       \      | z-spacing (in world coordinate)
					/     \       /      |
				   /       \_____/       |       
				   \       /             
					\_____/
				  
					   |----->  x-spacing (in world coordinate)
					   |--|     x-spacing-half
                                               
	*/
	namespace examap
	{
		//******************************************
		enum class eDir : u8
		{
			top = 0,
			left_top = 1,
			left_bottom = 2,
			bottom = 3,
			right_bottom = 4,
			right_top = 5
		};

		/******************************************
		* Coord
		* 
		* rappresenta una coordinata su una mappa esagonale
		*/
		struct Coord
		{
		public:
					Coord ()									{ set(0, 0); }
					Coord (i32 xIN, i32 zIN)					{ set(xIN, zIN); }
			void 	set  (i32 xIN, i32 zIN)						{ x = (i16)xIN; z = (i16)zIN; }
			void	move (eDir direction, u32 radius = 1);

			u32		pack_coord_u32() const						{ u32 ret = ((u32)x & 0x0000FFFF); ret |= ((u32)z & 0x0000FFFF) << 16; return ret; }
			void	set_from_packed_coord_u32 (u32 packed)		{ this->x = (i16)(packed & 0x0000FFFF); this->z = (i16)((packed & 0xFFFF0000) >> 16); }

			int		compare (const Coord &b) const				{ const u32 p1 = pack_coord_u32(); const u32 p2 = b.pack_coord_u32(); if (p1 == p2) return 0; if (p1 > p2) return 1; return -1; }
			bool	operator== (const Coord &b) const			{ return ( x==b.x && z==b.z ); }
			bool	operator!= (const Coord &b) const			{ return ( x!=b.x || z!=b.z ); }

		public:
			i16 x;
			i16 z;
		};


		//muove <in_out> nella direzione <dir> di <radius> caselle
		void	coord_move (Coord *in_out, eDir dir, u32 radius);

		//ritorna in <out_list> un elenco di Coord che rappresenta l'anello centrato in <center> e avente raggio <radius>
		//il numero di vtx di un ring e' 6*radius per cui <out_list> deve essere dimensionata in modo appropritato
		u32		coord_ring (const Coord &center, u32 radius, Coord *out_list, u32 num_elem_in_out_list);

		//ritorna in <out_word_point> i 6 vertici in world coordinate che definiscono un hex centrato in <world_center> e
		//avente raggio <hex_world_radius>
		void	coord_hexagon (const vec3f &world_center, f32 hex_world_radius, vec3f *out_word_point, u32 sizeof__out_word_point);


		/******************************************
		* CoordConverter
		* 
		* class di comodo per convertire coordinate da Exa a World e viceversa
		*/
		class CoordConverter
		{
		public:
					CoordConverter()																	{ x_spacing = z_spacing = 0; }

			void 	world__set_information (const vec3f &map_world_center, f32 exa_world_radius);

			vec3f 	exa_coord_to_world (const Coord &hex_coord) const;
			Coord	world_coord_to_exa (const vec3f &world_coord) const									{ return world_coord_to_exa (world_coord.x, world_coord.z); }
			Coord	world_coord_to_exa (f32 x, f32 z) const;
			
					//ritorna true se il punto <world_coord> e' all'interno dell'esagono il cui centro
					//e' <hex_coord>
			bool	world_is_inside_hex (const vec2f &world_coord, const Coord &hex_coord) const;

			f32		get_exa_world_radius() const														{ return exa_world_radius; }
			vec3f	get_map_world_center() const														{ return map_world_center; }

		private:
			vec3f 	map_world_center;
			f32		exa_world_radius;
			f32 	x_spacing;
			f32 	z_spacing;
			f32 	z_spacing_half;
			f32 	x_spacing_half;

		};


	} //namespace examap
} //namespace gos


#endif //_gosExamap_h_

