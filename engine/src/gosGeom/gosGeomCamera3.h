#ifndef _gosGeomCamera3_h_
#define _gosGeomCamera3_h_
#include "gosGeomPos3.h"
#include "gosGeomFrustum3.h"


namespace gos
{ 
	namespace geom
	{ 
		/*===============================================================================
		 * Camera3
		 * 
		 *==============================================================================*/
		class Camera3
		{ 
		public:
							Camera3 ()												{ priv_init(); }
							Camera3 (const Camera3 &b)								{ priv_init(); priv_copyFrom(b); }

							//========================== operatori
			Camera3&		operator= (const Camera3 &b)							{ priv_copyFrom(b); return *this; }

							//========================== fn
							//aspect = width / height
			void			set_perspective_FOV_LH (f32 aspect, f32 fovY_rad, f32 nearplane, f32 farplane);
										
			void			set_ortho_LH (f32 width, f32 height, f32 nearplane, f32 farplane, f32 zoom);

			void			projectI (f32 viewportDimx, f32 viewportDimy, const vec3f *points3D, vec2i *point2D, u32 nPoints);
			void			projectI (u32 viewportDimx, u32 viewportDimy, const vec3f *points3D, vec2i *point2D, u32 nPoints)				{ projectI (static_cast<f32>(viewportDimx), static_cast<f32>(viewportDimy), points3D, point2D, nPoints); }

			void			projectF (f32 viewportDimx, f32 viewportDimy, const vec3f *points3D, vec2f *point2D, u32 nPoints);
			void			projectF (u32 viewportDimx, u32 viewportDimy, const vec3f *points3D, vec2f *point2D, u32 nPoints)				{ projectF (static_cast<f32>(viewportDimx), static_cast<f32>(viewportDimy), points3D, point2D, nPoints); }
			

							//dato una serie di punti in 2D, ritorna la "DIREZIONE" del raggio 3D che va dall'origine della camera
							//e che passa per il punto 2D
			void			unproject (f32 viewportDimx, f32 viewportDimy, const vec2f *points2D, vec3f *out_direction3D, u32 nPoints);
			void			unproject (u32 viewportDimx, u32 viewportDimy, const vec2f *points2D, vec3f *out_direction3D, u32 nPoints)		{ unproject (static_cast<f32>(viewportDimx), static_cast<f32>(viewportDimy), points2D, out_direction3D, nPoints); }

			void			change_aspectRatio_perspective_FOV_LH (f32 newAspect)															{ set_perspective_FOV_LH (newAspect, get_FOV_y_rad(), get_near_distance(), get_far_distance()); }

							//========================== position
			Pos3			pos;

							/* ogni volta che modifichi pos, ricordati di chiamare 
								markUpdated() in modo che Camera possa sapere che la sua
								posizione e' stata modificata e quindi aggiornare le
								sue matrici*/
			void			mark_updated()											{ ++lastTimeUpdated; }

							//========================== query 
			f32				get_ortho_width() const									{ return ortoWidth; }
			f32				get_ortho_height() const								{ return ortoHeight; }
			f32				get_aspectRatio() const									{ return aspectRatio; }
			f32				get_near_distance() const								{ return nearDistance; }
			f32				get_far_distance() const								{ return farDistance; }
			f32				get_FOV_y_rad () const									{ assert(bIsPerspective); return fovy_rad; }
			f32				get_orhto_zoom () const									{ assert(!bIsPerspective); return fovy_rad; }
			bool			is_perspective() const									{ return bIsPerspective; }

			Frustum3		get_frustumWC()											{ if (!priv_isFrustumUpToDate()) priv_calcFrustum(); return frustumWC; }
			mat4x4f			get_matP () const										{ return matP; }
			mat4x4f			get_matV ();
			mat4x4f			get_matVP ();

		private:
			void			priv_init()												{ pos.identity();  lastTimeUpdated=0; lastTimeFrustumUpdated = lastTimeMatVPUpdated = lastTimeMatVUpdated = u32MAX; }
			void			priv_copyFrom (const Camera3 &b);
			void			priv_calcFrustum();
			bool			priv_isFrustumUpToDate() const							{ return lastTimeUpdated==lastTimeFrustumUpdated; }

		private:
			mat4x4f			matP;		//projection matrix
			u32				lastTimeUpdated;
			u32				lastTimeFrustumUpdated;
			u32				lastTimeMatVPUpdated;
			u32				lastTimeMatVUpdated;

			f32				ortoWidth;
			f32				ortoHeight;
			f32				aspectRatio;
			f32				nearDistance;
			f32				farDistance;
			f32				fovy_rad;
			bool			bIsPerspective;
			mat4x4f			matVP;
			Frustum3		frustumWC;	//in World Coordinate, ovvero in base all'attuale posizione/rotazione
			mat4x4f			matV;		//view matrix
		};
	 } //namespace geom
 } //namespace gos

#endif //_gosGeomCamera3_h_