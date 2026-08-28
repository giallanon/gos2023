#ifndef _gosGeomPlane3_h_
#define _gosGeomPlane3_h_
#include "../gosMath/gosMath.h"

namespace gos
{
	namespace geom
	{
		/***********************************************
		 * @brief	Plane3
		 * 
		 *  https://gdbooks.gitbooks.io/3dcollisions/content/Chapter1/point_on_plane.html
		 *	Equazione del piano
		 *		Ax + By + Cz + D = 0 
		 *	essendo (A.B,C) la normale del piano.
		 * 	Una equazione equivalente e':
		 * 		dot(Point, Normal) = Distance
		 *  che implica che un punto e' sul piano se 
		 * 		dot(Point, Normal) - Distance = 0
		 */
		class Plane3
		{
		public:
						Plane3 ()																{ }
						Plane3 (const vec3f p, const vec3f n)									{ set_from_point_and_normal (p, n); }
						Plane3 (const vec3f p1, const vec3f p2, const vec3f p3)					{ set_from_3points (p1, p2, p3); }


			f32 		signed_distance (const vec3f pIN) const;
			
			void		set_from_point_and_normal (const vec3f pIN, const vec3f nIN);
			void		set_from_3points (const vec3f p1, const vec3f p2, const vec3f p3);

		public:
			//vec3f	p;
			vec3f	n;
			f32		distance;
		};							
	} //namespace geom
} //namespace gos

#endif //_gosGeomPlane3_h_
