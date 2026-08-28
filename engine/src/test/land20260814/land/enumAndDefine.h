#ifndef _land_enumAndDefine_h_
#define _land_enumAndDefine_h_
#include "gosEngine.h"
#include "gosMath.h"

namespace land
{
	static constexpr f32 LAND__VIEW_DISTANCE_m = 1000.0f;

	struct ChunkCoord
	{
		gos::vec3f	centerWC;			//centro del chunk in woorld coordinate
		gos::vec3f	originWC;			//origine del chunk in woorld coordinate
		f32 distance2_from_pov;
		u16	chunk_x, chunk_y;		//coordinate del chunk
	};


} //namespace land



#endif //_land_enumAndDefine_h_
