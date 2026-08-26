#ifndef _land_enumAndDefine_h_
#define _land_enumAndDefine_h_
#include "gosEngine.h"
#include "gosMath.h"

namespace land
{
	static constexpr f32 LAND__VIEW_DISTANCE_m = 1000.0f;

	struct ChunkCoord
	{
		i16		x;
		i16		z;
		f32 	distance2_from_pov;
		gos::vec2f	center;
		gos::vec2f	origin;
	};


	u32 	calc_visible_chunk (const gos::vec3f pov, gos::FastArray<ChunkCoord> *out);
	u32 	calc_visible_chunk (gos::geom::Camera3 *cam, gos::FastArray<ChunkCoord> *out);

} //namespace land



#endif //_land_enumAndDefine_h_
