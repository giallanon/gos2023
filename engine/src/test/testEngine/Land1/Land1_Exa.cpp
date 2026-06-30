#include "Land1_Exa.h"
#include "../gosGeom/gosGeomUtils.h"

using namespace gos;
using namespace Land1;


//************************************************
gos::vec2f Exa::utils__calc_quad_center (u32 quad_index) const
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
void Exa::utils__get_quad_vertex (u32 quad_index, gos::vec2f *out__vtx4) const
{
	assert (quad_index < num_quad);
	out__vtx4[0] = vtxList[quadList[quad_index].idx[0]];
	out__vtx4[1] = vtxList[quadList[quad_index].idx[1]];
	out__vtx4[2] = vtxList[quadList[quad_index].idx[2]];
	out__vtx4[3] = vtxList[quadList[quad_index].idx[3]];
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
	
	const i8 golden = geom::line2D__which_side (v0, v1, wp);
	if (golden != geom::line2D__which_side (v1, v2, wp))
		return false;
	if (golden != geom::line2D__which_side (v2, v3, wp))
		return false;
	if (golden != geom::line2D__which_side (v3, v0, wp))
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

//************************************************
bool Exa::get_closest_vtx_from_point (const gos::vec3f &world_point, u32 *out__vtx_index) const
{
	u32 quad_index;
	if (!get_quad_from_point (world_point, &quad_index))
		return false;

	//so che <world_point> e' all'interno di <quad_index>
	//A questo punto il closest vtx e' uno dei 4 vtx del quad
	vec2f p (world_point.x, world_point.z);
	f32 min_dist = 1e36f;
	for (u32 i = 0; i < 4; i++)
	{
		u32 vtx_index = quadList[quad_index].idx[i];
		const f32 d = (p - vtxList[vtx_index] ).length2();
		if (d < min_dist)
		{
			min_dist = d;
			*out__vtx_index = vtx_index;
		}
	}

	return true;
}

//************************************************
u32 Exa::get_quad_from_vtx (u32 vtx_index, u32 *out__quadList, u32 num_elem_in_quad_list) const
{
	assert (vtx_index < num_vtx);

	u32 ret = 0;
	for (u32 i = 0; i < num_quad; i++)
	{
		for (u8 i2 = 0; i2 < 4; i2++)
		{
			if (vtx_index == quadList[i].idx[i2])
			{
				if (ret < num_elem_in_quad_list)
					out__quadList[ret++] = i;
			}
		}
	}

	//ordino il risulato in senso orario
	if (ret > 1)
	{
		assert (ret <=8);
		const vec2f center = vtxList[vtx_index];
		
		f32 angle_list[8];
		for (u32 i = 0; i < ret; i++)
		{
			const vec2f  quad_center = quadCenterList[out__quadList[i]];
			const vec2f p = quad_center - center;
			angle_list[i] = atan2f (p.y, p.x);
		}

		bool bEsci = false;
		u32 n = ret;
		while (bEsci == false)
		{
			bEsci = true;
			n--;
			
			for (u32 i = 0; i < n; i++)
			{
				if (angle_list[i] < angle_list[i + 1])
				{
					bEsci = false;
					GOSSWAP(angle_list[i], angle_list[i + 1]);
					GOSSWAP(out__quadList[i], out__quadList[i + 1]);
				}
			}
		}

	}

	return ret;
}