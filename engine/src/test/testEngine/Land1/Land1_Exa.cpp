#include "Land1_enumAndDefine.h"
#include "../gosGeom/gosGeomUtils.h"

using namespace gos;
using namespace Land1;


//************************************************
gos::vec2f Exa::calc_quad_center (u32 quad_index) const
{
	assert (quad_index < num_quad);
	gos::vec2f ret = vtxList[quadList[quad_index].idx[0]]
		+ vtxList[quadList[quad_index].idx[1]]
		+ vtxList[quadList[quad_index].idx[2]]
		+ vtxList[quadList[quad_index].idx[3]];
	ret /= 4.0f;
	return ret;
}

//************************************************
bool Exa::priv_is_point_in_quad (const gos::vec3f &world_point, u32 quad_index) const
{
	assert (quad_index < num_quad);

	//assumo che il quad si "piano" e che le coordinate y dei suoi vtx siano tutte uguali
	//In questo caso, il problema si riduce alla sua versione 2D
	//Per sapere se un punto 2D e' dentro un quad 2D, verifico che il punto sia sempre dallo stesso "lato" delle 
	//4 linee che formano il quad

	const vec2f v0 = vtxList[quadList[quad_index].idx[0]];
	const vec2f v1 = vtxList[quadList[quad_index].idx[1]];
	const vec2f v2 = vtxList[quadList[quad_index].idx[2]];
	const vec2f v3 = vtxList[quadList[quad_index].idx[3]];
	const vec2f wp (world_point.x, world_point.z);
	
	const i8 golden = geom::which_side_of_line2D (v0, v1, wp);
	if (golden != geom::which_side_of_line2D (v1, v2, wp))
		return false;
	if (golden != geom::which_side_of_line2D (v2, v3, wp))
		return false;
	if (golden != geom::which_side_of_line2D (v3, v0, wp))
		return false;
	return true;
}

//************************************************
bool Exa::get_quad_from_point (const gos::vec3f &world_point, u32 *out__quad_index) const
{
	for (u32 i = 0; i < num_quad; i++)
	{
		if (priv_is_point_in_quad(world_point, i))
		{
			*out__quad_index = i;
			return true;
		}
	}

	return false;
}