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