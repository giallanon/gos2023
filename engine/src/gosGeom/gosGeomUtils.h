#ifndef _gosGeomUtils_h_
#define _gosGeomUtils_h_
#include "../gosMath/gosMath.h"
#include "../gos/gosFastArray.h"

namespace gos
{
	namespace geom
	{
        void    circle (FastArray<vec3f> *out_vtxList, const vec3f &center, f32 radius, u32 numPoint, f32 starting_angle_grad = 0);


				//data una linea 2D <line_start> - <line_end>, ritorna +1 o -1 a seconda che il punto <world_point> stia 
				//a destra o a sinistra della linea
		i8		which_side_of_line2D (const vec2f &line_start, const vec2f &line_end, const vec2f &world_point);
    } //namespace geom
} //namespace gos

#endif //_gosGeomUtils_h_