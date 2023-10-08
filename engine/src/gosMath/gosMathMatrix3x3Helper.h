#ifndef _gosMathMatrix3x3Helper_h_
#define _gosMathMatrix3x3Helper_h_
#include "gosMathEnumAndDefine.h"
#include "gosVect.h"

namespace gos
{
	namespace math
	{
#define SIN_COS_ROTAZIONE_ANTIORARIA(rad)	const T sina=sinf(rad);  const T cosa=cosf(rad);
#define SIN_COS_ROTAZIONE_ORARIA(rad)		const T sina=-sinf(rad); const T cosa=cosf(rad);



//		void	helper_Matrix3x3_buildFromXYZAxes (T *values, const Vec3<T> &ax, const Vec3<T> &ay, const Vec3<T> &az)
#define helper_Matrix3x3_buildFromXYZAxes(ax, ay, az)\
		{\
			values[ADDR(0,0)] = ax.x;		values[ADDR(0,1)] = ay.x;		values[ADDR(0,2)] = az.x;\
			values[ADDR(1,0)] = ax.y;		values[ADDR(1,1)] = ay.y;		values[ADDR(1,2)] = az.y;\
			values[ADDR(2,0)] = ax.z;		values[ADDR(2,1)] = ay.z;		values[ADDR(2,2)] = az.z;\
		}\


#define helper_Matrix3x3_buildFromAxeAndAngle(axle, rad)\
		{\
			SIN_COS_ROTAZIONE_ORARIA(rad);\
			const T oneMinusCosa = 1.0f - cosa;\
			const T uxx = axle.x * axle.x;\
			const T uyy = axle.y * axle.y;\
			const T uzz = axle.z * axle.z;\
			const T uxyOneMinusCosa = axle.x * axle.y * oneMinusCosa;\
			const T uxzOneMinusCosa = axle.x * axle.z * oneMinusCosa;\
			const T uyzOneMinusCosa = axle.y * axle.z * oneMinusCosa;\
			const T uxsina = axle.x * sina;\
			const T uysina = axle.y * sina;\
			const T uzsina = axle.z * sina;\
			values[ADDR(0,0)] = uxx*oneMinusCosa + cosa;	values[ADDR(0,1)] = uxyOneMinusCosa  - uzsina;	values[ADDR(0,2)] = uxzOneMinusCosa + uysina;\
			values[ADDR(1,0)] = uxyOneMinusCosa  + uzsina;	values[ADDR(1,1)] = uyy*oneMinusCosa + cosa; 		values[ADDR(1,2)] = uyzOneMinusCosa - uxsina;\
			values[ADDR(2,0)] = uxzOneMinusCosa  - uysina;	values[ADDR(2,1)] = uyzOneMinusCosa  + uxsina;	values[ADDR(2,2)] = uzz*oneMinusCosa + cosa;\
		}\

#define helper_Matrix3x3_buildScale(s)\
		{\
			values[ADDR(0,0)] = s.x; values[ADDR(0,1)]=0;   values[ADDR(0,2)]=0;\
			values[ADDR(1,0)] = 0;   values[ADDR(1,1)]=s.y; values[ADDR(1,2)]=0;\
			values[ADDR(2,0)] = 0;   values[ADDR(2,1)]=0;   values[ADDR(2,2)]=s.z;\
		}\

#define helper_Matrix3x3_buildRotationAboutX(rad)\
		{\
			SIN_COS_ROTAZIONE_ORARIA(rad);\
			values[ADDR(0,0)] = 1;	values[ADDR(0,1)] = 0;			values[ADDR(0,2)] = 0;\
			values[ADDR(1,0)] = 0;	values[ADDR(1,1)] = cosa;		values[ADDR(1,2)] = -sina;\
			values[ADDR(2,0)] = 0;	values[ADDR(2,1)] = sina;		values[ADDR(2,2)] = cosa;\
		}\

#define helper_Matrix3x3_buildRotationAboutY(rad)\
		{\
			SIN_COS_ROTAZIONE_ORARIA(rad);\
			values[ADDR(0,0)] = cosa;		values[ADDR(0,1)] = 0;	values[ADDR(0,2)] = sina;\
			values[ADDR(1,0)] = 0;			values[ADDR(1,1)] = 1;	values[ADDR(1,2)] = 0;\
			values[ADDR(2,0)] = -sina;		values[ADDR(2,1)] = 0;	values[ADDR(2,2)] = cosa;\
		}
		

#define helper_Matrix3x3_buildRotationAboutZ(rad)\
		{\
			SIN_COS_ROTAZIONE_ORARIA(rad);\
			values[ADDR(0,0)] = cosa;		values[ADDR(0,1)] = -sina;	values[ADDR(0,2)] = 0;\
			values[ADDR(1,0)] = sina;		values[ADDR(1,1)] = cosa;	values[ADDR(1,2)] = 0;\
			values[ADDR(2,0)] = 0;			values[ADDR(2,1)] = 0;		values[ADDR(2,2)] = 1;\
		}\

#define helper_Matrix3x3_buildLookAt(eye, at, up)\
		{\
			const Vec3<T>	zaxis = normalize (at - eye);\
			const Vec3<T>	xaxis = normalize (cross (up, zaxis));\
			const Vec3<T>	yaxis = normalize (cross (zaxis, xaxis));\
			helper_Matrix3x3_buildFromXYZAxes(xaxis, yaxis, zaxis);\
		}\


		//vedi https://en.wikipedia.org/wiki/Euler_angles
		//z1x2y3
#define helper_Matrix3x3_buildFromEulerAngles_YXZ(rad_y, rad_x, rad_z)\
		{\
			const T	c1	= cosf(rad_z);\
			const T	s1  = sinf(rad_z);\
			const T	c2	= cosf(rad_x);\
			const T	s2  = sinf(rad_x);\
			const T	c3	= cosf(rad_y);\
			const T	s3  = sinf(rad_y);\
			const T	s1s2 = s1 * s2;\
			const T	c1c3 = c1 * c3;\
			const T	c1s3 = c1 * s3;\
			values[ADDR(0,0)] = c1c3 - s1s2 * s3;\
			values[ADDR(0,1)] = c3 * s1 + c1s3 * s2;\
			values[ADDR(0,2)] = -c2 * s3;\
			values[ADDR(1,0)] = -c2 * s1;\
			values[ADDR(1,1)] = c1 * c2;\
			values[ADDR(1,2)] = s2;\
			values[ADDR(2,0)] = c1s3 + c3 * s1s2;\
			values[ADDR(2,1)] = s1 * s3 - c1c3 * s2;\
			values[ADDR(2,2)] = c2 * c3;\
		}\


	} //namespace math
}//namespace gos

#endif //_gosMathMatrix3x3Helper_h_