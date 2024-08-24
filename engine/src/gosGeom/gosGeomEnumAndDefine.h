#ifndef _gosGeomEnumAndDefine_h_
#define _gosGeomEnumAndDefine_h_
#include "../gos/gosEnumAndDefine.h"



namespace gos
{ 
	enum class eVtxMapSemantic : u8
	{
		position = 0,
		normal = 1,
		texCoord = 2
		//max 16 elementi
	};

	enum class eVtxMapType : u8
	{
		f32 = 0,
		i32 = 1,
		u32 = 2
		//max 16 elementi
	};
 } //namespace gos

#endif //_gosGeomEnumAndDefine_h_