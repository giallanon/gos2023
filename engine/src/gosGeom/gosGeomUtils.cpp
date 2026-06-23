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
i8 geom::which_side_of_line2D (const vec2f &line_start, const vec2f &line_end, const vec2f &world_point)
{
    const vec2f ba = line_end - line_start;
    const vec2f ca = world_point - line_start;
	if ((ba.x * ca.y) - (ba.y * ca.x) < 0)
        return 1;
    return -1;
}