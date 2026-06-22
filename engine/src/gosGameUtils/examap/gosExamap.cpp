#include "gosExamap.h"

using namespace gos;

//*********************************************
void examap::Coord::move (eDir direction, u32 radius)		{ examap::coord_move (this, direction, radius); }

//*********************************************
void examap::coord_move (Coord *in_out, eDir dir, u32 radius)
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

//*********************************************
u32 examap::coord_ring (const Coord &center, u32 radius, Coord *out_list, u32 num_elem_in_out_list)
{
	assert (radius > 0);

	//il num di hex di una circonferenza di raggio r e': r*6
	assert (num_elem_in_out_list >= radius * 6);

	Coord hex = center;
	hex.move (eDir::top, radius);

	u32 ct = 0;
	for (u32 i = 0; i < radius; i++) { hex.move (eDir::left_bottom, 1); out_list[ct++] = hex; }
	for (u32 i = 0; i < radius; i++) { hex.move (eDir::bottom, 1); out_list[ct++] = hex; }
	for (u32 i = 0; i < radius; i++) { hex.move (eDir::right_bottom, 1); out_list[ct++] = hex; }
	for (u32 i = 0; i < radius; i++) { hex.move (eDir::right_top, 1); out_list[ct++] = hex; }
	for (u32 i = 0; i < radius; i++) { hex.move (eDir::top, 1); out_list[ct++] = hex; }
	for (u32 i = 0; i < radius; i++) { hex.move (eDir::left_top, 1); out_list[ct++] = hex; }

	assert (ct <= num_elem_in_out_list);
	return ct;
}

//*********************************************
void examap::coord_hexagon (const vec3f &world_center, f32 hex_world_radius, vec3f *out_word_point, u32 sizeof__out_word_point)
{
	//servono 6 punti
	assert (sizeof__out_word_point >= sizeof(vec3f) * 6);

    const f32 rad_incr = 1.0471975512f; //math::DUEPI / 6
    f32 rad = 0;

    for (u32 i=0; i<6; i++)
    {
        vec3f v;

        v.x = cosf(rad) * hex_world_radius;
        v.y = 0;
        v.z = sinf(rad) * hex_world_radius;
        v += world_center;

        out_word_point[i] = v;
        rad += rad_incr;
    }    
}


