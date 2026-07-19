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


				//data una linea 2D <line_start> - <line_end>, ritorna +1 o -1 a seconda che il punto <world_point> stia 
				//a destra o a sinistra della linea
		i8		line2D__which_side (const vec2f &line_start, const vec2f &line_end, const vec2f &world_point);

				//calcola il punto di intersezione della linea A-B contro la linea C-D
		bool	line2D__intersect (const vec2f &A, const vec2f &B, const vec2f &C, const vec2f &D, vec2f *out);

				//ordina tutti i <num_point> presenti in <point_list> in senso orario rispetto a <center> e ritorna
				//in <out_oder> gli indici dei vtx ordinati
		void	point2D_order_clockwise (const vec2f center, const vec2f *point_list, u32 num_point, u32 *out_oder, u32 sizeof_out_order);
    } //namespace geom
} //namespace gos

#endif //_gosGeomUtils_h_