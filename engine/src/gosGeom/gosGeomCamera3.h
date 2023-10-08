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
			Camera3&				operator= (const Camera3 &b)							{ priv_copyFrom(b); return *this; }

									//========================== fn
			void					setPerspectiveFovLH (f32 aspect, f32 fovY_rad, f32 nearplane, f32 farplane);
										//aspect = width / height
			void					setOrthoLH (f32 width, f32 height, f32 nearplane, f32 farplane, f32 zoom);

			void					projectI (f32 viewportDimx, f32 viewportDimy, const vec3f *points3D, vec2i *point2D, u32 nPoints);
			void					projectF (f32 viewportDimx, f32 viewportDimy, const vec3f *points3D, vec2f *point2D, u32 nPoints);
			void					unproject (f32 viewportDimx, f32 viewportDimy, const vec2f *points2D, vec3f *OUTpoints3D, u32 nPoints);

			void					changeAspectRatioPerspectiveFovLH (f32 newAspect)					{ setPerspectiveFovLH (newAspect, getFOV_y_rad(), getNearDistance(), getFarDistance()); }

									//========================== position
			Pos3					pos;
			void					markUpdated()											{ ++lastTimeUpdated; }
									/* ogni volta che modifichi pos, ricordati di chiamare 
										markUpdated() in modo che Camera possa sapere che la sua
										posizione è stata modificata e quindi aggiornare le
										sue matrici
									*/

									//========================== query 
			f32						getOrthoWidth() const									{ return ortoWidth; }
			f32						getOrthoHeight() const									{ return ortoHeight; }
			f32						getAspectRatio() const									{ return aspectRatio; }
			f32						getNearDistance() const									{ return nearDistance; }
			f32						getFarDistance() const									{ return farDistance; }
			f32						getFOV_y_rad () const									{ assert(bIsPerspective); return fovy_rad; }
			f32						getOrhtoZoom () const									{ assert(!bIsPerspective); return fovy_rad; }
			bool					isPerspective() const									{ return bIsPerspective; }

			const Frustum3&			getFrustumWC()											{ if (!priv_isFrustumUpToDate()) priv_calcFrustum(); return frustumWC; }
			const mat4x4f&			getMatP () const										{ return matP; }
			const mat4x4f&			getMatV ();
			const mat4x4f&			getMatVP ();

		private:
			void					priv_init()												{ pos.identity();  lastTimeUpdated=0; lastTimeFrustumUpdated = lastTimeMatVPUpdated = lastTimeMatVUpdated = u32MAX; }
			void					priv_copyFrom (const Camera3 &b);
			void					priv_calcFrustum();
			bool					priv_isFrustumUpToDate() const							{ return lastTimeUpdated==lastTimeFrustumUpdated; }

		private:
			mat4x4f					matP;		//projection matrix
			u32						lastTimeUpdated;
			u32						lastTimeFrustumUpdated;
			u32						lastTimeMatVPUpdated;
			u32						lastTimeMatVUpdated;

			f32						ortoWidth;
			f32						ortoHeight;
			f32						aspectRatio;
			f32						nearDistance;
			f32						farDistance;
			f32						fovy_rad;
			bool					bIsPerspective;
			mat4x4f					matVP;
			Frustum3				frustumWC;	//in World Coordinate, ovvero in base all'attuale posizione/rotazione
			mat4x4f					matV;		//view matrix
		};
	 } //namespace geom
 } //namespace gos

#endif //_gosGeomCamera3_h_