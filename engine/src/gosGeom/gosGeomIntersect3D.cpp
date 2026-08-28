#include "gosGeomIntersect3D.h"

using namespace gos;


/********************************************
* SLAB METHOD:  https://tavianator.com/2015/ray_box_nan.html
*/
bool geom::ray3D__intersect_AABB3 (const vec3f &rayO, const vec3f &rayDir, f32 rayLen, const AABB3 &aabb3, f32 *out__dist)
{
    const vec3f inv_ray_dir ( 1.0f / rayDir.values[0], 1.0f / rayDir.values[1], 1.0f / rayDir.values[2]);

    f32 t1 = (aabb3.vmin.values[0] - rayO.values[0]) * inv_ray_dir.values[0];
    f32 t2 = (aabb3.vmax.values[0] - rayO.values[0]) * inv_ray_dir.values[0];

    f32 tmin = GOSMIN(t1, t2);
    f32 tmax = GOSMAX(t1, t2);

    for (u8 i = 1; i < 3; ++i)
    {
        t1 = (aabb3.vmin.values[i] - rayO.values[i]) * inv_ray_dir.values[i];
        t2 = (aabb3.vmax.values[i] - rayO.values[i]) * inv_ray_dir.values[i];

        tmin = GOSMAX(tmin, GOSMIN(t1, t2));
        tmax = GOSMIN(tmax, GOSMAX(t1, t2));
    }

    *out__dist = tmin;
    return tmax >= GOSMAX(0.0f, tmin) && tmin < rayLen;
}

//**************************************************************************
eClipResult	geom::AABB3__intersect_AABB3 (const AABB3 &a, const AABB3 &b)
{
	if (a.vmax.x <= b.vmin.x || a.vmin.x >= b.vmax.x ||
		a.vmax.y <= b.vmin.y || a.vmin.y >= b.vmax.y ||
		a.vmax.z <= b.vmin.z || a.vmin.z >= b.vmax.z)
		return eClipResult::outside;

	if (a.vmin.x >= b.vmin.x && a.vmax.x <= b.vmax.x &&
		a.vmin.y >= b.vmin.y && a.vmax.y <= b.vmax.y &&
		a.vmin.z >= b.vmin.z && a.vmax.z <= b.vmax.z)
		return eClipResult::inside;

	return eClipResult::intersect;
}

//********************************************
eClipResult	geom::AABB3__intersect_plane3 (const AABB3 &aabb, const Plane3 &pl)
{
	vec3f	posPt = aabb.vmax;
	vec3f	negPt = aabb.vmin;
	
	if(pl.n.x<0)		{ posPt.x = aabb.vmin.x; negPt.x = aabb.vmax.x; }
	if(pl.n.y<0) 		{ posPt.y = aabb.vmin.y; negPt.y = aabb.vmax.y; }
	if(pl.n.z<0) 		{ posPt.z = aabb.vmin.z; negPt.z = aabb.vmax.z; }

	if (pl.signed_distance (posPt) < 0)		return eClipResult::outside;
	if (pl.signed_distance (negPt) >= 0)	return eClipResult::inside;
	return eClipResult::intersect;
}

//********************************************
eClipResult	geom::AABB3__intersect_frustum3 (const AABB3 &aabb, const Frustum3 &fr)
{
	eClipResult	ret = eClipResult::inside;

	switch (AABB3__intersect_plane3 (aabb, fr.get_plane(Frustum3::ePlane::front)))
	{
	case eClipResult::outside:		return eClipResult::outside;
	case eClipResult::intersect:	ret = eClipResult::intersect; break;
	case eClipResult::inside:	
		switch (AABB3__intersect_plane3 (aabb, fr.get_plane(Frustum3::ePlane::back)))
		{
		case eClipResult::inside:		break;
		case eClipResult::outside:		return eClipResult::outside;
		case eClipResult::intersect:	ret = eClipResult::intersect; break;
		}
		break;
	}

	switch (AABB3__intersect_plane3 (aabb, fr.get_plane(Frustum3::ePlane::left)))
	{
	case eClipResult::inside:		break;
	case eClipResult::outside:		return eClipResult::outside;
	case eClipResult::intersect:	ret = eClipResult::intersect; break;
	}

	switch (AABB3__intersect_plane3 (aabb, fr.get_plane(Frustum3::ePlane::right)))
	{
	case eClipResult::inside:		break;
	case eClipResult::outside:		return eClipResult::outside;
	case eClipResult::intersect:	ret = eClipResult::intersect; break;
	}


	switch (AABB3__intersect_plane3 (aabb, fr.get_plane(Frustum3::ePlane::top)))
	{
	case eClipResult::inside:		break;
	case eClipResult::outside:		return eClipResult::outside;
	case eClipResult::intersect:	ret = eClipResult::intersect; break;
	}

	switch (AABB3__intersect_plane3 (aabb, fr.get_plane(Frustum3::ePlane::bottom)))
	{
	case eClipResult::inside:		break;
	case eClipResult::outside:		return eClipResult::outside;
	case eClipResult::intersect:	ret = eClipResult::intersect; break;
	}

	return ret;
}