#ifndef _gosGeomPos3_h_
#define _gosGeomPos3_h_
#include "../gosMath/gosMath.h"
#include "../gosMath/gosQuat.h"

namespace gos
{ 
	namespace geom
	{ 
		/*===============================================================================
		 * Pos3
		 * 
		 *==============================================================================*/
		class Pos3
		{ 
		public:
									Pos3()																		{ }
									Pos3(const Pos3 &b)															{ priv_copyFrom(b); }

			Pos3&					operator= (const Pos3 &b)													{ priv_copyFrom(b); return *this; }

			void					identity ()																	{ o.set (0,0,0); rot.identity(); }
			void					setFromMatrix4x4 (const mat4x4f &m);
			void					setFromMatrix3x4 (const mat3x4f &m);
			void					setFromXYZAxes (const vec3f &ax, const vec3f &ay, const vec3f &az)			{ rot.buildFromXYZAxes (ax, ay, az); }
			void					setFromEulerAngles_YXZ (f32 rad_y, f32 rad_x, f32 rad_z)					{ rot.buildFromEulerAngles_YXZ (rad_y, rad_x, rad_z); }
			void					setRotationFromMatrix3x3 (const mat3x3f &r)									{ rot=r; }
			void					setFromQuat (const Quat &q)													{ q.toMatrix3x3(&rot);}
			void					warp (f32 x, f32 y, f32 z)													{ warp(vec3f(x,y,z)); }
			void					warp (const vec3f &p)														{ o=p; }

			void					moveRelAlongX (f32 howMuch)													{ o += getAsseX() * howMuch; };
			void					moveRelAlongY (f32 howMuch)													{ o += getAsseY() * howMuch; };
			void					moveRelAlongZ (f32 howMuch)													{ o += getAsseZ() * howMuch; };
			void					rotateMeAbout (const vec3f &ax, f32 angle_rad)								
									{ 
										mat3x3f m; 
										m.buildFromAxeAndAngle (ax, angle_rad); 
										rot = m * rot; 

#if 0
										for (u8 r = 0; r < 3; r++)
										{
											for (u8 c = 0; c < 3; c++)
											{
												if (fabsf(rot(r, c)) < 1e-06f)
													rot(r, c) = 0;
												else if (fabsf(1.0f - rot(r, c)) < 1e-06f)
												{
													if (rot(r, c) > 0)
														rot(r, c) = 1;
													else
														rot(r, c) = -1;
												}
											}
										}
#endif
									}
			void					rotateMeAboutMyX (f32 angle_rad)											{ rotateMeAbout (getAsseX(), angle_rad); }
			void					rotateMeAboutMyY (f32 angle_rad)											{ rotateMeAbout (getAsseY(), angle_rad); }
			void					rotateMeAboutMyZ (f32 angle_rad)											{ rotateMeAbout (getAsseZ(), angle_rad); }
			
			void					lookAt (const vec3f &at)													{ alignAsseZ (at -o); }
			void					alignAsseX (const vec3f &align);
			void					alignAsseY (const vec3f &align);
			void					alignAsseZ (const vec3f &align);
			void					normalizeRotMatrix();
			
			//un vect espresso in word coordinate [vIn] viene trasformato in local coordinate [vOut]
			void					vect_ToLocal (const vec3f *vIn, vec3f *vOut, u32 n) const;
										
			//un vect espresso in local coordinate [vIn] viene trasformato in world coordinate [vOut]
			void					vect_ToWorld (const vec3f *vIn, vec3f *vOut, u32 n) const;
										
			//come sopra, ma per i punti
			void					point_ToLocal (const vec3f *vIn, vec3f *vOut, u32 n) const;
			void					point_ToWorld (const vec3f *vIn, vec3f *vOut, u32 n) const;

			vec3f					getAsseX () const															{ return vec3f(rot(0,0), rot(1,0), rot(2,0)); }
			vec3f					getAsseY () const															{ return vec3f(rot(0,1), rot(1,1), rot(2,1)); }
			vec3f					getAsseZ () const															{ return vec3f(rot(0,2), rot(1,2), rot(2,2)); }
			void					getMatrix3x4 (mat3x4f *out) const;
			void					getMatrix4x4 (mat4x4f *out) const;

		public:
			vec3f					o;
			mat3x3f					rot;

		private:
			void					priv_copyFrom (const Pos3 &b)												{ o = b.o; rot = b.rot; }
		};
	 } //namespace geom
 } //namespace gos

#endif //_gosGeomPos3_h_