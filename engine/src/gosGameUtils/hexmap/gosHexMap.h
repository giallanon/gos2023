#ifndef _gosHexMap_h_
#define _gosHexMap_h_
#include "../gosGameUtils.h"


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

namespace gos
{
	class HexMap
	{
	public:
		enum class eDir : u8
		{
			top = 0,
			left_top = 1,
			left_bottom = 2,                
			bottom = 3,
			right_bottom = 4,
			right_top = 5
		};

	public:
		struct Coord
		{
		public:
					Coord ()								{ set(0,0); }
					Coord (i32 xIN, i32 zIN)				{ set(xIN, zIN); }
			void 	set  (i32 xIN, i32 zIN)					{ x=xIN; z=zIN; }
			void	move (eDir direction, u32 radius=1)		{ HexMap::coord_move (this, direction, radius); }

		public:
			i32 x;
			i32 z;
		};

	public:
		//muove <in_out> nella direzione <dir> di <radius> casella
		static void coord_move (Coord *in_out, eDir dir, u32 radius)
		{ 
			switch (dir)
			{
			case eDir::top:				in_out->z += radius; break;
			case eDir::left_top:		in_out->x -= radius; in_out->z += radius; break;
			case eDir::left_bottom:		in_out->x -= radius; break;
			case eDir::bottom:			in_out->z -= radius; break;
			case eDir::right_bottom:	in_out->x += radius; in_out->z -= radius; break;
			case eDir::right_top:		in_out->x += radius; break;
			}
		}


		//ritorna in <out_list> un elenco di Coord che rappresenta l'anello centrato in <center> e avente raggio <radius>
		static u32 coord_ring (const Coord &center, u32 radius, Coord *out_list, u32 num_elem_in_out_list)
		{
			assert (radius > 0);

			//il num di hex di una circonferenza di raggio r e': r*6
			assert (num_elem_in_out_list >= radius * 6);

			Coord hex = center;
			hex.move (eDir::top, radius);
			
			u32 ct = 0;
			for (u32 i=0; i<radius; i++)	{ hex.move (eDir::left_bottom, 1); out_list[ct++] = hex; }
			for (u32 i=0; i<radius; i++)	{ hex.move (eDir::bottom, 1); out_list[ct++] = hex; }
			for (u32 i=0; i<radius; i++)	{ hex.move (eDir::right_bottom, 1); out_list[ct++] = hex; }
			for (u32 i=0; i<radius; i++)	{ hex.move (eDir::right_top, 1); out_list[ct++] = hex; }
			for (u32 i=0; i<radius; i++)	{ hex.move (eDir::top, 1); out_list[ct++] = hex; }
			for (u32 i=0; i<radius; i++)	{ hex.move (eDir::left_top, 1); out_list[ct++] = hex; }

			assert (ct <= num_elem_in_out_list);
			return ct;
		}

		//ritorna in <out_word_point> i 6 vertici in world coordinate che definiscono un hex centrato in <world_center> e
		//avente raggio = radius
		static void coord_hexagon (const vec3f &world_center, u32 radius, vec3f *out_word_point, u32 sizeof__out_word_point);

	public:
				HexMap()																	{ x_spacing = z_spacing = 0; }

		void 	world__set_information (const vec3f &world_center, f32 world_radius);
		vec3f 	hex_coord_to_world (const Coord &hex_coord) const;
		
		Coord	world_coord_to_hex (const vec3f &world_coord) const							{ return world_coord_to_hex (world_coord.x, world_coord.z); }
		Coord	world_coord_to_hex (f32 x, f32 z) const;
		bool	world_is_inside_hex (const vec2f &world_coord, const Coord &hex_coord) const;

	private:
		vec3f 	world_center;
		f32		world_radius;
		f32 	x_spacing;
		f32 	z_spacing;
		f32 	z_spacing_half;
		f32 	x_spacing_half;

	};	



} //namespace gos


#endif //_gosHexMap_h_