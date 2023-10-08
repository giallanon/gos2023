#ifndef _gosGeomFrustum3_h_
#define _gosGeomFrustum3_h_
#include "gosGeomKDop.h"
#include "gosGeomAABB3.h"

namespace gos
{
	namespace geom
	{
		/*===============================================================
		 *
		 *==============================================================*/
		class Frustum3 : public KDop<6>
		{
		public:
			enum class ePlane : u8
			{
				front	= 0,
				back	= 1,
				left	= 2,
				right	= 3,
				top		= 4,
				bottom	= 5
			};

		public:
								Frustum3()										{ nPlanes = 6; }

									//====================== fn
			void				buildPerspective (const vec3f &o, const vec3f &lookDir, const vec3f &upDir, const vec3f &rightDir,
													  f32 nearDistance, f32 farDistance, f32 fovY_rad, f32 aspect);
			void				buildOrtho (const vec3f &o, const vec3f &lookDir, const vec3f &upDir, const vec3f &rightDir,
												f32 width, f32 height, f32 nearDistance, f32 farDistance, f32 zoom);

									//====================== query =========================
			const vec3f&		getNearCenter()	const							{ return planes[(u8)ePlane::front].p; }
			const vec3f&		getFarCenter()	const							{ return planes[(u8)ePlane::back].p; }
			f32					getFarDistance() const							{ return farDistance;}

			const vec3f&		getFarPlaneHalfAsseX ()	const					{ return farPlaneHalfAsseX; }
									// ritorna un vettore che è lungo metà della larghezza del farplane e punto in direzione x del farplane 
		
			const vec3f&		getFarPlaneHalfAsseY()	const					{ return farPlaneHalfAsseY; }
									// ritorna un vettore che è lungo metà dell'altezza del farplane e punto in direzione y del farplane

			const vec3f&		getNearPlaneHalfAsseX()	const					{ return nearPlaneHalfAsseX; }
									// ritorna un vettore che è lungo metà della larghezza del farplane e punto in direzione x del farplane 
		
			const vec3f&		getNearPlaneHalfAsseY()	const					{ return nearPlaneHalfAsseY; }
									// ritorna un vettore che è lungo metà dell'altezza del farplane e punto in direzione y del farplane

			const vec3f&		getNormal (ePlane p) const						{ return planes[(u8)p].n; }

								//====================== utils ==========
			void				calc8Points (vec3f *out) const;
			void				calcAABB (AABB3 &out) const;

		private:
			vec3f				nearPlaneHalfAsseX;
			vec3f				nearPlaneHalfAsseY;
			vec3f				farPlaneHalfAsseX;
			vec3f				farPlaneHalfAsseY;
			f32					farDistance;
		};
	} //namespace geom
} //namespace gos
#endif //_gosGeomFrustum3_h_