#ifndef _gosGPUEnumAndDefine_h_
#define _gosGPUEnumAndDefine_h_
#include "../gos/gosEnumAndDefine.h"
#include "../gos/gosHandle.h"
#include "vulkan/gosGPUVulkanEnumAndDefine.h"


//A per "chunk", B per "user", C per "index", D per "counter".
typedef gos::HandleT<8,1,14,9>	GPUShaderHandle;		//2^14=16384 => num totale di oggetti, divisi in chunc da 2^8=256
typedef gos::HandleT<5,1,10,16>	GPUVtxDeclHandle;		//2^10=2014 => num totale di oggetti, divisi in chunc da 2^5=32



#define 	GOSGPU__NUM_MAX_VTXDECL_ATTR	32
#define 	GOSGPU__NUM_MAX_VXTDECL_STREAM	16


enum class eVtxStreamInputRate : u8
{
	perVertex = 0,
	perInstance = 1
};

enum class eDrawPrimitive : u8
{
	pointList = 0,
	
	lineList = 1,
	lineStrip = 2,
	
	trisList = 3,
	trisStrip = 4,
	trisFan = 5
};

enum class eShaderType : u8
{
	vertexShader = 0,
	fragmentShader = 1,
	unknown = 0xff
};

struct sVtxDescriptor
{
	u8              streamIndex;
	u8              bindingLocation;
	eDataFormat     format;
	u8              offset;
};




#endif//_gosGPUEnumAndDefine_h_