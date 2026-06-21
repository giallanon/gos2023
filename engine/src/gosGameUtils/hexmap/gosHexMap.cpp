#include "gosHexMap.h"

using namespace gos;

//*********************************************
void HexMap::coord_hexagon (const vec3f &world_center, u32 radius, vec3f *out_word_point, u32 sizeof__out_word_point)
{
	//servono 6 punti
	assert (sizeof__out_word_point >= sizeof(vec3f) * 6);

    const f32 rad_incr = 1.0471975512f; //math::DUEPI / 6
    f32 rad = 0;

    for (u32 i=0; i<6; i++)
    {
        vec3f v;

        v.x = cosf(rad) * radius;
        v.y = 0;
        v.z = sinf(rad) * radius;
        v += world_center;

        out_word_point[i] = v;
        rad += rad_incr;
    }    
}

//*************************************
void HexMap::world__set_information (const vec3f &world_centerIN, f32 world_radiusIN)
{
	world_center = world_centerIN;
	world_radius = world_radiusIN;

	x_spacing = (3.0f * world_radius) / 2.0f;
	z_spacing = 1.732f * world_radius;

	//x_spacing += world_radius/10.0f; z_spacing += world_radius/10.0f;

	z_spacing_half = z_spacing * 0.5f;
	x_spacing_half = world_radius / 2.0f;
}

//*************************************
vec3f HexMap::hex_coord_to_world (const Coord &hex_coord) const
{
	assert (x_spacing > 0 && z_spacing > 0);

	vec3f ret = world_center;
	ret.z += z_spacing * (f32)hex_coord.z;

	ret.x += x_spacing * (f32)hex_coord.x;
	ret.z += z_spacing_half * (f32)hex_coord.x;
	return ret;
}



static bool HexMap__line_isRight (const gos::vec2f &a, const gos::vec2f &b, const gos::vec2f &c)
{
	return (b.x - a.x)*(c.y - a.y) - (b.y - a.y)*(c.x - a.x) > 0;
}

//*************************************
bool HexMap::world_is_inside_hex (const vec2f &world_coord, const Coord &hex_coord) const
{
	const vec3f v3 = hex_coord_to_world(hex_coord);

	//v in coordinate locali dell'hex
	const f32 qx = world_coord.x - v3.x;
	const f32 qz = world_coord.y - v3.z;

	//sono nel "rettangolo" tra top e bottom
	if (qz > -z_spacing_half && qz < z_spacing_half && 
		qx > -x_spacing_half && qx < x_spacing_half)
		return true;

	if (qz > 0)
	{
		//sono nella parte top
		if (qz > z_spacing_half)
			return false;

		if (qx > 0)
		{
			//top right
			const vec2f a (x_spacing_half, z_spacing_half);
			const vec2f b (world_radius, 0);
			
			if (!HexMap__line_isRight(a, b, vec2f(qx,qz)))
				return true;
		}
		else
		{
			//top left
			const vec2f a (-world_radius, 0);
			const vec2f b (-x_spacing_half, z_spacing_half);
			if (!HexMap__line_isRight(a, b, vec2f(qx,qz)))
				return true;
		}
	}
	else
	{
		//sono nella parte bottom
		if (qz < -z_spacing_half)
			return false;

		if (qx > 0)
		{
			//bottom right
			const vec2f a ( world_radius, 0);
			const vec2f b ( x_spacing_half, -z_spacing_half);
			if (!HexMap__line_isRight(a, b, vec2f(qx,qz)))
				return true;
		}
		else
		{
			//bottom left

			//il bordo e' definito dalla seguente linea
			const vec2f a (-x_spacing_half, -z_spacing_half);
			const vec2f b (-world_radius, 0);
			if (!HexMap__line_isRight(a, b, vec2f(qx,qz)))
				return true;
		}
	}

	return false;
}

//*************************************
HexMap::Coord HexMap::world_coord_to_hex (f32 xIN, f32 zIN) const
{
	const vec2f v (xIN - world_center.x, zIN - world_center.z);

	//x puo' essere x1 oppure x2
	const i32 x1 = floorf(v.x / x_spacing);
	const i32 x2 = x1 +1;

	//z puo' essere z1 oppure z2
	i32 z1 = floorf(v.y / z_spacing);
	if (x1 % 2 == 0)
		z1 -= x1/2;
	else
		z1 -= (x1+1)/2;
	const i32 z2 = z1+1;

	Coord ret (x1, z1);
	if (world_is_inside_hex(vec2f(xIN, zIN), ret)) return ret;

	ret.x = x2;
	if (world_is_inside_hex(vec2f(xIN, zIN), ret)) return ret;

	ret.z = z2;
	if (world_is_inside_hex(vec2f(xIN, zIN), ret)) return ret;

	ret.x = x1;
	if (world_is_inside_hex(vec2f(xIN, zIN), ret)) return ret;

	//DBGBREAK;
	//printf ("x=%d,%d  z=%d,%d\n", x1, x2, z1, z2);
	return ret;

}

