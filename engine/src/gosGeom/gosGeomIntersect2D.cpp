#include "gosGeomIntersect2D.h"

using namespace gos;

//*********************************************
i8 geom::line2D__which_side (const vec2f &line_start, const vec2f &line_end, const vec2f &world_point)
{
    const vec2f ba = line_end - line_start;
    const vec2f ca = world_point - line_start;
	if ((ba.x * ca.y) - (ba.y * ca.x) < 0)
        return 1;
    return -1;
}

//*********************************************
bool geom::line2D__intersect (const vec2f &o1, const vec2f &p1, const vec2f &o2, const vec2f &p2, vec2f *out)
{
	assert (NULL != out);

    const vec2f x = o2 - o1;
    const vec2f d1 = p1 - o1;
    const vec2f d2 = p2 - o2;

    float cross = d1.x*d2.y - d1.y*d2.x;
    if (abs(cross) < 1e-8f)
        return false;

    const f32 t1 = (x.x * d2.y - x.y * d2.x)/cross;
    (*out) = o1 + d1 * t1;
    return true;
}