#ifndef _gosGeomFrustum3_h_
#define _gosGeomFrustum3_h_
#include "gosGeomKDop.h"
#include "gosGeomAABB3.h"

namespace gos
{
	namespace geom
	{
		/********************************************
		 * @brief 	Frustum3
		 *			E' un KDop con 6 planes
		 */
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
			void				build_perspective (const vec3f &o, const vec3f &lookDir, const vec3f &upDir, const vec3f &rightDir,
													  f32 nearDistance, f32 farDistance, f32 fovY_rad, f32 aspect);
			void				build_ortho (const vec3f &o, const vec3f &lookDir, const vec3f &upDir, const vec3f &rightDir,
												f32 width, f32 height, f32 nearDistance, f32 farDistance, f32 zoom);

								//====================== query =========================
			Plane3				get_plane (ePlane p) const							{ return planes[(u8)p]; }
			Plane3				get_plane (u8 i) const								{ assert(i<6); return planes[i]; }
			const vec3f&		get_normal (ePlane p) const							{ return planes[(u8)p].n; }
			const vec3f&		get_normal (u8 i) const								{ assert(i<6); return planes[i].n; }

			const vec3f&		get_near_center() const								{ return planes[(u8)ePlane::front].p; }
			const vec3f&		get_far_center() const								{ return planes[(u8)ePlane::back].p; }
			f32					get_far_distance() const							{ return farDistance;}

			// ritorna un vettore che e' lungo meta' della larghezza del farplane e punto in direzione x del farplane 
			const vec3f&		get_farPlane_half_asseX ()	const					{ return farPlaneHalfAsseX; }
									
			// ritorna un vettore che e' lungo meta' dell'altezza del farplane e punto in direzione y del farplane
			const vec3f&		get_farPlane_half_asseY()	const					{ return farPlaneHalfAsseY; }
									
			// ritorna un vettore che e' lungo meta' della larghezza del farplane e punto in direzione x del farplane 
			const vec3f&		get_nearPlane_half_asseX()	const					{ return nearPlaneHalfAsseX; }
									
			// ritorna un vettore che e' lungo meta' dell'altezza del farplane e punto in direzione y del farplane
			const vec3f&		get_nearPlane_half_asseY()	const					{ return nearPlaneHalfAsseY; }
									


								//====================== utils ==========
			void				calc_8points (vec3f *out) const;
			void				calc_AABB (AABB3 *out) const;

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