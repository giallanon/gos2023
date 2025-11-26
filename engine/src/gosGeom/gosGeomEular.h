#ifndef _gosGeomEular_h_
#define _gosGeomEular_h_
#include "../gosMath/gosMath.h"


namespace gos
{
	namespace geom
	{
		void	eular_clamp_0_DUEPI (gos::vec3f *in_out__eular_rad);
		void	eular_compute3x3Matrix (const gos::vec3f &eular_rad, gos::mat3x3f *out);
		void	eular_compute4x4Matrix (const gos::vec3f &eular_rad, gos::mat4x4f *out);
	} //namespace geom
} //namespace gos


#endif //_gosGeomEular_h_