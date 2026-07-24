#ifndef _gosGeomUtils_h_
#define _gosGeomUtils_h_
#include "../gosMath/gosMath.h"
#include "../gos/gosFastArray.h"

namespace gos
{
	namespace geom
	{
        void    circle (FastArray<vec3f> *out_vtxList, const vec3f &center, f32 radius, u32 numPoint, f32 starting_angle_grad = 0);
		void    circle (FastArray<vec2f> *out_vtxList, const vec2f &center, f32 radius, u32 numPoint, f32 starting_angle_grad = 0);

				//ordina tutti i <num_point> presenti in <point_list> in senso orario rispetto a <center> e ritorna
				//in <out_oder> gli indici dei vtx ordinati
		void	point2D_order_clockwise (const vec2f center, const vec2f *point_list, u32 num_point, u32 *out_oder, u32 sizeof_out_order);
    } //namespace geom
} //namespace gos

#endif //_gosGeomUtils_h_