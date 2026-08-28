#include "gosGeomPlane3.h"

using namespace gos;
using namespace gos::geom;


//**********************************************************
void Plane3::set_from_point_and_normal (const vec3f pIN, const vec3f nIN)
{
	n = nIN;
	n.normalize();
	distance = math::dot(n, pIN);
}

//**********************************************************
void Plane3::set_from_3points (const vec3f p1, const vec3f p2, const vec3f p3)
{
	const vec3f	vv1 = p2 - p1;
	const vec3f	vv2 = p3 - p1;
	n = math::cross (vv1, vv2);
	set_from_point_and_normal (p1, n);
}

//**********************************************************
f32 Plane3::signed_distance (const vec3f pIN) const
{
	return math::dot(pIN, n) - distance;
}

