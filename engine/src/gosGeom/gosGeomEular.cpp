#include "gosGeomEular.h"

using namespace gos;

//***********************************
void geom::eular_clamp_0_DUEPI (gos::vec3f *in_out__eular_rad)
{
	assert (NULL != in_out__eular_rad);
	if (in_out__eular_rad->x < 0)				in_out__eular_rad->x += math::DUEPI;
	if (in_out__eular_rad->x >= math::DUEPI)	in_out__eular_rad->x -= math::DUEPI;

	if (in_out__eular_rad->y < 0)				in_out__eular_rad->y += math::DUEPI;
	if (in_out__eular_rad->y >= math::DUEPI)	in_out__eular_rad->y -= math::DUEPI;

	if (in_out__eular_rad->z < 0)				in_out__eular_rad->z += math::DUEPI;
	if (in_out__eular_rad->z >= math::DUEPI)	in_out__eular_rad->z -= math::DUEPI;
}


//***********************************
void geom::eular_compute3x3Matrix (const gos::vec3f &eular_rad, gos::mat3x3f *out)
{
	assert (NULL != out);
	out->buildFromEulerAngles_YXZ (eular_rad.y, eular_rad.x, eular_rad.z);
}

//***********************************
void geom::eular_compute4x4Matrix (const gos::vec3f &eular_rad, gos::mat4x4f *out)
{
	assert (NULL != out);
	out->buildFromEulerAngles_YXZ (eular_rad.y, eular_rad.x, eular_rad.z);
}