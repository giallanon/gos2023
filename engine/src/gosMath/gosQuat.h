#ifndef _gosQuat_h_
#define _gosQuat_h_
#include "gosMathEnumAndDefine.h"
#include "gosMatrix.h"
#include "gosVect.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
namespace gos
{
	/*=========================================================================
		* Unit quaternion
		*/
	class Quat
	{
	public:
		union
		{
			struct
			{
				f32	x,y,z,w;
			};
			f32	values[4];
		};

	public:
							//==================== costruttori =================
							Quat (f32 xIN=0, f32 yIN=0, f32 zIN=0, f32 wIN=1) : x(xIN), y(yIN), z(zIN), w(wIN) {}
						
							Quat (const mat3x3f &m)														{ buildFromMatrix3x3(m);}
							Quat (const vec3f &ax, f32 rad)												{ buildRotation(ax, rad);}
							Quat (const vec3f &ax, const vec3f &ay, const vec3f &az)					{ buildFromXYZAxes(ax, ay, az);}

							//================= operatori =========================================
		f32&				operator [] (const i32 i)													{assert( i>=0 && i<4);	return values[i];}
		const f32&			operator [] (const i32 i) const												{assert( i>=0 && i<4);	return values[i];}
		Quat				operator-()	const															{return Quat(-x, -y, -z, -w);}

							//==================== fn =================
		void				identity()																	{x=y=z=0; w=1;}
		void				buildFromMatrix3x3(const mat3x3f &m);
		void				buildFromXYZAxes (const vec3f &x, const vec3f &y, const vec3f &z);
		void				buildRotation (const vec3f &ax, f32 angle_rad);
							/* ax deve essere normalizzato
								Crea una quat che esprime la rotazione attorno all'asse ax*/
		void				buildFromEuler_YXZ (f32 rad_ay, f32 rad_ax, f32 rad_az);
			
		void				buildRotationAboutAsseX (f32 angle_rad);
		void				buildRotationAboutAsseY (f32 angle_rad);
		void				buildRotationAboutAsseZ (f32 angle_rad);
								/* si intendono i canonici assi (1,0,0) (0,1,0) (0,0,1) */

		void				toMatrix3x3 (mat3x3f *out_m) const;
		void				toAxis (vec3f *out_ax, vec3f *out_ay, vec3f *out_az) const;
		void				toEuler (f32 *rad_ax, f32 *rad_ay, f32 *rad_az) const;

		void				normalize();
		void				inverse();
		void				rotateMeAbout (const vec3f &ax, f32 angle_rad);
							/* ax deve essere normalizzato.
								Attenzione che questa fn ruota *this about ax
								Se vuoi una quat che esprima una rotazione attorno ad un asse, usa buildFromAxisAndAngle()
								*/


		vec3f				vec3Transform (const vec3f &v) const;
	};		
} //namespace gos
#pragma GCC diagnostic pop

#endif //_gosQuat_h_