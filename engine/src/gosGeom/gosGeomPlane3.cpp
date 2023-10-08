#include "gosGeomPlane3.h"

using namespace gos;
using namespace gos::geom;


//**********************************************************
void Plane3::setFrom3Points (const vec3f &p1, const vec3f &p2, const vec3f &p3)
{
	vec3f	vv1 = p2 - p1;
	vec3f	vv2 = p3 - p1;

	n = math::cross (vv1, vv2);
	n.normalize();
	p = p1;
}

