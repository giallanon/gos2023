#include "Land1_Exa.h"
#include "../gosGeom/gosGeomUtils.h"

using namespace gos;
using namespace Land1;

//************************************************
bool Exa::priv_is_point_in_quad (const gos::vec3f &world_point, u16 idx0, u16 idx1, u16 idx2, u16 idx3) const
{
	//assumo che il quad si "piano" e che le coordinate y dei suoi vtx siano tutte uguali
	//In questo caso, il problema si riduce alla sua versione 2D
	//Per sapere se un punto 2D e' dentro un quad 2D, verifico che il punto sia sempre dallo stesso "lato" delle 
	//4 linee che formano il quad
	const vec2f v0 = vtxList[idx0];
	const vec2f v1 = vtxList[idx1];
	const vec2f v2 = vtxList[idx2];
	const vec2f v3 = vtxList[idx3];
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
bool Exa::priv_is_point_in_tris (const gos::vec3f &world_point, u16 idx0, u16 idx1, u16 idx2) const
{
	const vec2f v0 = vtxList[idx0];
	const vec2f v1 = vtxList[idx1];
	const vec2f v2 = vtxList[idx2];
	const vec2f wp (world_point.x, world_point.z);
	
	const i8 golden = geom::line2D__which_side (v0, v1, wp);
	if (golden != geom::line2D__which_side (v1, v2, wp))
		return false;
	if (golden != geom::line2D__which_side (v2, v0, wp))
		return false;
	return true;
}

//************************************************
bool Exa::priv_is_point_in_penta (const gos::vec3f &world_point, u16 idx0, u16 idx1, u16 idx2, u16 idx3, u16 idx4) const
{
	const vec2f v0 = vtxList[idx0];
	const vec2f v1 = vtxList[idx1];
	const vec2f v2 = vtxList[idx2];
	const vec2f v3 = vtxList[idx3];
	const vec2f v4 = vtxList[idx4];
	const vec2f wp (world_point.x, world_point.z);
	
	const i8 golden = geom::line2D__which_side (v0, v1, wp);
	if (golden != geom::line2D__which_side (v1, v2, wp))
		return false;
	if (golden != geom::line2D__which_side (v2, v3, wp))
		return false;
	if (golden != geom::line2D__which_side (v3, v4, wp))
		return false;
	if (golden != geom::line2D__which_side (v4, v0, wp))
		return false;
	return true;
}

//************************************************
bool Exa::priv_is_point_in_exagon (const gos::vec3f &world_point, u16 idx0, u16 idx1, u16 idx2, u16 idx3, u16 idx4, u16 idx5) const
{
	const vec2f v0 = vtxList[idx0];
	const vec2f v1 = vtxList[idx1];
	const vec2f v2 = vtxList[idx2];
	const vec2f v3 = vtxList[idx3];
	const vec2f v4 = vtxList[idx4];
	const vec2f v5 = vtxList[idx5];
	const vec2f wp (world_point.x, world_point.z);
	
	const i8 golden = geom::line2D__which_side (v0, v1, wp);
	if (golden != geom::line2D__which_side (v1, v2, wp))
		return false;
	if (golden != geom::line2D__which_side (v2, v3, wp))
		return false;
	if (golden != geom::line2D__which_side (v3, v4, wp))
		return false;
	if (golden != geom::line2D__which_side (v4, v5, wp))
		return false;
	if (golden != geom::line2D__which_side (v5, v0, wp))
		return false;
	return true;
}

//************************************************
bool Exa::priv_is_point_in_octagon (const gos::vec3f &world_point, u16 idx0, u16 idx1, u16 idx2, u16 idx3, u16 idx4, u16 idx5, u16 idx6, u16 idx7) const
{
	const vec2f v0 = vtxList[idx0];
	const vec2f v1 = vtxList[idx1];
	const vec2f v2 = vtxList[idx2];
	const vec2f v3 = vtxList[idx3];
	const vec2f v4 = vtxList[idx4];
	const vec2f v5 = vtxList[idx5];
	const vec2f v6 = vtxList[idx6];
	const vec2f v7 = vtxList[idx7];
	const vec2f wp (world_point.x, world_point.z);
	
	const i8 golden = geom::line2D__which_side (v0, v1, wp);
	if (golden != geom::line2D__which_side (v1, v2, wp))
		return false;
	if (golden != geom::line2D__which_side (v2, v3, wp))
		return false;
	if (golden != geom::line2D__which_side (v3, v4, wp))
		return false;
	if (golden != geom::line2D__which_side (v4, v5, wp))
		return false;
	if (golden != geom::line2D__which_side (v5, v6, wp))
		return false;
	if (golden != geom::line2D__which_side (v6, v7, wp))
		return false;
	if (golden != geom::line2D__which_side (v7, v0, wp))
		return false;
	return true;
}

//************************************************
bool Exa::get_closest_vtx_from_point (const gos::vec3f &world_point, u32 *out__vtx_index) const
{
	for (u32 iVtx = 0; iVtx < num_vtx_originali; iVtx++)
	{
		const u16 *idx_list = vtxInfoList[iVtx].idx_list;

		switch (vtxInfoList[iVtx].num_quad)
		{
		case 0:
			break;

		case 1:
			assert (vtxInfoList[iVtx].is_border_vtx);
			assert (vtxInfoList[iVtx].num_idx == 4);
			if (priv_is_point_in_quad (world_point, idx_list[0], idx_list[1], idx_list[2], idx_list[3]))
			{
				*out__vtx_index = iVtx;
				return true;
			}
			break;

		case 2:
			assert (vtxInfoList[iVtx].is_border_vtx);
			assert (vtxInfoList[iVtx].num_idx == 6);
			if (priv_is_point_in_quad (world_point, idx_list[1], idx_list[2], idx_list[4], idx_list[5]))
			{
				*out__vtx_index = iVtx;
				return true;
			}
			break;

		case 3:
			
			if (vtxInfoList[iVtx].is_border_vtx)
			{
				assert (vtxInfoList[iVtx].num_idx == 8);
				if (priv_is_point_in_octagon (world_point, idx_list[0], idx_list[1], idx_list[2], idx_list[3], idx_list[4], idx_list[5], idx_list[6], idx_list[7]))
				{
					*out__vtx_index = iVtx;
					return true;
				}
			}			else
			{
				assert (vtxInfoList[iVtx].num_idx == 7);
				if (priv_is_point_in_tris (world_point, idx_list[2], idx_list[4], idx_list[6]))
				{
					*out__vtx_index = iVtx;
					return true;
				}
			}
			break;

		case 4:
			assert (vtxInfoList[iVtx].num_idx == 9);
			if (priv_is_point_in_quad (world_point, idx_list[2], idx_list[4], idx_list[6], idx_list[8]))
			{
				*out__vtx_index = iVtx;
				return true;
			}
			break;

		case 5:
			assert (vtxInfoList[iVtx].num_idx == 11);
			if (priv_is_point_in_penta (world_point, idx_list[2], idx_list[4], idx_list[6], idx_list[8], idx_list[10]))
			{
				*out__vtx_index = iVtx;
				return true;
			}
			break;

		case 6:
			assert (vtxInfoList[iVtx].num_idx == 13);
			if (priv_is_point_in_exagon (world_point, idx_list[2], idx_list[4], idx_list[6], idx_list[8], idx_list[10], idx_list[12]))
			{
				*out__vtx_index = iVtx;
				return true;
			}
			break;
		}
	}

	return false;
}

//************************************************
static void Exa__utils_1 (const u16 *idx_list, u16 idx0, u16 idx1, u16 idx2, u16 idx3, u16 *out_4_index)
{
	out_4_index[0] = idx_list[idx0];
	out_4_index[1] = idx_list[idx1];
	out_4_index[2] = idx_list[idx2];
	out_4_index[3] = idx_list[idx3];
}

bool Exa::get_quad_indices (u32 vtx_index, u32 quad_index, u16 *out_4_index) const
{
	assert (vtx_index < num_vtx_originali);
	const VtxInfo *vi = &vtxInfoList[vtx_index];
	
	assert (quad_index < vi->num_quad);
	switch (vi->num_quad)
	{
	default:
		DBGBREAK;
		return false;
	case 0:
		return false;
		break;

	case 1:
		assert (quad_index == 0);
		Exa__utils_1 (vi->idx_list, 0, 1, 2, 3, out_4_index);
		break;

	case 2:
		if (0 == quad_index)	{ Exa__utils_1 (vi->idx_list, 0, 1, 2, 3, out_4_index); }
		else					{ Exa__utils_1 (vi->idx_list, 0, 3, 4, 5, out_4_index); }
		break;

	case 3:
		if (vi->is_border_vtx)
		{
			if (0 == quad_index)		{ Exa__utils_1 (vi->idx_list, 0, 1, 2, 3, out_4_index); }
			else if (1 == quad_index)	{ Exa__utils_1 (vi->idx_list, 0, 3, 4, 5, out_4_index); }
			else						{ Exa__utils_1 (vi->idx_list, 0, 5, 6, 7, out_4_index); }
		}
		else
		{
			if (0 == quad_index)		{ Exa__utils_1 (vi->idx_list, 0, 1, 2, 3, out_4_index); }
			else if (1 == quad_index)	{ Exa__utils_1 (vi->idx_list, 0, 3, 4, 5, out_4_index); }
			else						{ Exa__utils_1 (vi->idx_list, 0, 5, 6, 1, out_4_index); }
		}
		break;

	case 4:
		if (0 == quad_index)		{ Exa__utils_1 (vi->idx_list, 0, 1, 2, 3, out_4_index); }
		else if (1 == quad_index)	{ Exa__utils_1 (vi->idx_list, 0, 3, 4, 5, out_4_index); }
		else if (2 == quad_index)	{ Exa__utils_1 (vi->idx_list, 0, 5, 6, 7, out_4_index); }
		else						{ Exa__utils_1 (vi->idx_list, 0, 7, 8, 1, out_4_index); }
		break;

	case 5:
		if (0 == quad_index)		{ Exa__utils_1 (vi->idx_list, 0, 1, 2, 3, out_4_index); }
		else if (1 == quad_index)	{ Exa__utils_1 (vi->idx_list, 0, 3, 4, 5, out_4_index); }
		else if (2 == quad_index)	{ Exa__utils_1 (vi->idx_list, 0, 5, 6, 7, out_4_index); }
		else if (3 == quad_index)	{ Exa__utils_1 (vi->idx_list, 0, 7, 8, 9, out_4_index); }
		else						{ Exa__utils_1 (vi->idx_list, 0, 9, 10, 1, out_4_index); }
		break;

	case 6:
		if (0 == quad_index)		{ Exa__utils_1 (vi->idx_list, 0, 1, 2, 3, out_4_index); }
		else if (1 == quad_index)	{ Exa__utils_1 (vi->idx_list, 0, 3, 4, 5, out_4_index); }
		else if (2 == quad_index)	{ Exa__utils_1 (vi->idx_list, 0, 5, 6, 7, out_4_index); }
		else if (3 == quad_index)	{ Exa__utils_1 (vi->idx_list, 0, 7, 8, 9, out_4_index); }
		else if (4 == quad_index)	{ Exa__utils_1 (vi->idx_list, 0, 9, 10, 11, out_4_index); }
		else						{ Exa__utils_1 (vi->idx_list, 0, 11, 12, 1, out_4_index); }
		break;		
	}

	return true;
}