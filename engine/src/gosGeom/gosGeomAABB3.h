#ifndef _gosGeomAABB3_h_
#define _gosGeomAABB3_h_
#include "../gosMath/gosMath.h"


namespace gos
{
	namespace geom
	{
		/*========================================================
		 *
		 *=======================================================*/
		class AABB3
		{
		public:
									AABB3 ()																{ }
									AABB3 (const AABB3 &b)													{ setFromAABB3(b); }
									AABB3 (const vec3f &vminIN, const vec3f &vmaxIN)						{ setByMinMax (vminIN, vmaxIN); }

									//=========================== set
					void			setFromAABB3 (const AABB3 &b)											{ vmin=b.vmin; vmax=b.vmax; }
					void			setByMinMax (const vec3f &vminIN, const vec3f &vmaxIN)					{ vmin=vminIN; vmax=vmaxIN; }
					void			setByOAndHalfDim (const vec3f &o, const vec3f &halfDim)					{ vmin = o - halfDim; vmax = o + halfDim; }

									//=========================== transform 
			static	void			quatTransform (const AABB3 &aabb, const Quat &b, AABB3 *out);
			static	void			matrixTransform (const AABB3 &aabb, const mat4x4f &b, AABB3 *out);
			static	void			matrixTransform (const AABB3 &aabb, const mat3x4f &b, AABB3 *out);

									//=========================== operators
					AABB3&			operator= (const AABB3 &b)												{ setFromAABB3(b); return *this; }
					AABB3&			operator+=(const AABB3 &b);
			friend	AABB3			operator+ (const AABB3 &a, const AABB3 &b)								{ AABB3	ret = a; ret+=b; return ret; }

									//=========================== get
					void			get8Point (vec3f *out) const;
					void			calcDim (vec3f *out) const												{ (*out) = vmax-vmin; }
					void			calcCenter (vec3f *out) const											{ (*out) = vmin + (vmax-vmin) * 0.5f; }
					f32				calcDimX() const														{ return (vmax.x - vmin.x); }
					f32				calcDimY() const														{ return (vmax.y - vmin.y); }
					f32				calcDimZ() const														{ return (vmax.z - vmin.z); }

		public:
					vec3f		vmin;
					vec3f		vmax;
		};
	} //namespace geom
} //namespace gos

#endif //_gosGeomAABB3_h_