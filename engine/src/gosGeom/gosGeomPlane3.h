#ifndef _gosGeomPlane3_h_
#define _gosGeomPlane3_h_
#include "../gosMath/gosMath.h"

namespace gos
{
	namespace geom
	{
		/***********************************************
		 * @brief	Plane3
		 */
		class Plane3
		{
		public:
						Plane3 ()																	{ }
						Plane3 (const vec3f &p, const vec3f &n)										{ set_from_point_and_normal (p, n); }
						Plane3 (const vec3f &p1, const vec3f &p2, const vec3f &p3)					{ set_from_3points (p1, p2, p3); }

			/* Distanza di p dal plane. La distanza e' positiva se p e' in direzione della normale. */
			f32			distance (const vec3f &pIN)	const											{ return math::dot (this->n, (pIN - this->p)); }
						
			
			void		set_from_point_and_normal (const vec3f &pIN, const vec3f &nIN)				{ p = pIN; n = nIN; }
			void		set_from_3points (const vec3f &p1, const vec3f &p2, const vec3f &p3);

		public:
			vec3f	p;
			vec3f	n;
		};							
	} //namespace geom
} //namespace gos

#endif //_gosGeomPlane3_h_
