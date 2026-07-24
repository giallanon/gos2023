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
