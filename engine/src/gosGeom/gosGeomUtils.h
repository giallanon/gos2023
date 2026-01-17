#ifndef _gosGeomUtils_h_
#define _gosGeomUtils_h_
#include "../gosMath/gosMath.h"
#include "../gos/gosFastArray.h"

namespace gos
{
	namespace geom
	{
        void    circle (FastArray<vec3f> *out_vtxList, const vec3f &center, f32 radius, u32 numPoint, f32 starting_angle_grad = 0);

    } //namespace geom
} //namespace gos

#endif //_gosGeomUtils_h_