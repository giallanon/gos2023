#ifndef _gosGeomPlane3_h_
#define _gosGeomPlane3_h_
#include "../gosMath/gosMath.h"

namespace gos
{
	namespace geom
	{
		/*========================================================
		 *
		 *=======================================================*/
		class Plane3
		{
		public:
						Plane3 ()																	{ }
						Plane3 (const vec3f &p, const vec3f &n)										{ setFromPointAndNormal (p, n); }
						Plane3 (const vec3f &p1, const vec3f &p2, const vec3f &p3)					{ setFrom3Points (p1, p2, p3); }

						//========================= static
			static f32	distance (const Plane3 &pl, const vec3f &p)									{ return math::dot (pl.n, (p - pl.p)); }
							/* Distanza di p dal plane. La distanza � positiva se p � in direzione della normale. */
			
						//========================= 
			void		setFromPointAndNormal (const vec3f &pIN, const vec3f &nIN)					{ p = pIN; n = nIN; }
			void		setFrom3Points (const vec3f &p1, const vec3f &p2, const vec3f &p3);

						//=========================
			vec3f	p;
			vec3f	n;
		};							
	} //namespace geom
} //namespace gos

#endif //_gosGeomPlane3_h_
