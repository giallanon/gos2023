#ifndef _gosGameUtils_h_
#define _gosGameUtils_h_
#include "../gosMath/gosMath.h"

namespace gos
{
	namespace utils
	{
		//encoda/decoda una normale
		u32		normal_encode_octahedral (vec3f norm);
		vec3f	normal_decode_octahedral (u32 encoded_value, bool bNormalizeResult);
	} //namespace utils
} //namespace gos

#endif //_gosGameUtils_h_
