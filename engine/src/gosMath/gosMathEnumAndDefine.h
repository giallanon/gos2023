#ifndef _gosMathEnumAndDefine_h_
#define _gosMathEnumAndDefine_h_
#include "../gos/gosEnumAndDefine.h"
#include <math.h>


namespace gos
{
	namespace math
	{
		static const f32	PI					= 3.1415926535897932384626433832795028841971693993751f;
		static const f32	PIMEZZI				= 1.57079632679489f;
		static const f32	PIQUARTI			= 0.78539816339744f;
		static const f32	DUEPI				= 6.28318530717958f;

		/*=========================================================
		 * 
		 * utils
		 *
		 *=========================================================*/
		constexpr f32				gradToRad (f32 grad)									{ return grad*0.01745329251994f; }
		constexpr f32				radToGrad (f32 rad)										{ return rad*57.295779513082320876798154814105f; }
		constexpr f32				sign (f32 f)											{ if (f<0) return -1.0f; return 1.0f; }
	} //namespace math
} //namespace gos
#endif//_gosMathEnumAndDefine_h_