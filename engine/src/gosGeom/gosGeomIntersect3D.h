#ifndef _gosGeomIntersect3D_h_
#define _gosGeomIntersect3D_h_
#include "../gosMath/gosMath.h"
#include "gosGeomAABB3.h"
#include "gosGeomFrustum3.h"

namespace gos
{
	namespace geom
	{

		bool		ray3D__intersect_AABB3 (const vec3f &rayO, const vec3f &rayDir, f32 rayLen, const AABB3 &aabb3, f32 *out__dist);

		eClipResult	AABB3__intersect_AABB3 (const AABB3 &a, const AABB3 &b);
		eClipResult	AABB3__intersect_plane3 (const AABB3 &aabb, const Plane3 &pl);
		eClipResult	AABB3__intersect_frustum3 (const AABB3 &aabb3, const Frustum3 &fr);

    } //namespace geom
} //namespace gos



#endif //_gosGeomIntersect3D_h_

