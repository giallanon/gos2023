#ifndef _gosGeomIntersect2D_h_
#define _gosGeomIntersect2D_h_
#include "../gosMath/gosMath.h"
#include "../gos/gosFastArray.h"

namespace gos
{
	namespace geom
	{
				//data una linea 2D <line_start> - <line_end>, ritorna +1 o -1 a seconda che il punto <world_point> stia 
				//a destra o a sinistra della linea
		i8		line2D__which_side (const vec2f &line_start, const vec2f &line_end, const vec2f &world_point);

				//calcola il punto di intersezione della linea A-B contro la linea C-D
		bool	line2D__intersect (const vec2f &A, const vec2f &B, const vec2f &C, const vec2f &D, vec2f *out);

    } //namespace geom
} //namespace gos



#endif //_gosGeomIntersect2D_h_

