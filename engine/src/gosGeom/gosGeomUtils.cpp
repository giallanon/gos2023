#include "gosGeomUtils.h"

using namespace gos;


//*********************************************
void geom::circle (FastArray<vec3f> *out_vtxList, const vec3f &center, f32 radius, u32 numPoint, f32 starting_angle)
{
    assert (NULL != out_vtxList);

    const f32 rad_incr = math::DUEPI / (f32)numPoint;
    f32 rad = math::gradToRad(starting_angle);

    while (numPoint--)
    {
        vec3f v;

        v.x = cosf(rad) * radius;
        v.y = sinf(rad) * radius;
        v.z = 0;
        v += center;

        out_vtxList->append(v);
        rad += rad_incr;
    }
}

//*********************************************
void geom::circle (FastArray<vec2f> *out_vtxList, const vec2f &center, f32 radius, u32 numPoint, f32 starting_angle)
{
    assert (NULL != out_vtxList);

    const f32 rad_incr = math::DUEPI / (f32)numPoint;
    f32 rad = math::gradToRad(starting_angle);

    while (numPoint--)
    {
        vec2f v;

        v.x = cosf(rad) * radius;
        v.y = sinf(rad) * radius;
        v += center;

        out_vtxList->append(v);
        rad += rad_incr;
    }
}

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

