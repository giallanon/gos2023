#ifndef _gosGPUEnumAndDefine_h_
#define _gosGPUEnumAndDefine_h_
#include "../gos/gosEnumAndDefine.h"
#include "../gos/dataTypes/gosColorHDR.h"
#include "../gos/gosHandle.h"
#include "vulkan/gosGPUVulkanEnumAndDefine.h"

#define 	GOSGPU__NUM_MAX_VTXDECL_ATTR					32
#define 	GOSGPU__NUM_MAX_VXTDECL_STREAM					16
#define 	GOSGPU__NUM_MAX_ATTACHMENT						16
#define 	GOSGPU__NUM_MAX_DESCRIPTOR_PER_SET				32
#define 	GOSGPU__NUM_MAX_DESCRIPTOR_POOL_SIZE_PER_POOL	16


//A per "num max di handle", B per "num di chunk", C per "counter"
typedef gos::HandleT< 5,3,16, 0,8>	GPUDepthStencilHandle;		//2^5=32 => num totale di oggetti, divisi in chunk da 2^3=8
typedef gos::HandleT< 5,5,16, 0,6>	GPUViewportHandle;			//2^5=32 => num totale di oggetti, divisi in chunk da 2^5=32
typedef gos::HandleT< 6,5,16, 0,5>	GPURenderTargetHandle;		//2^6=64 => num totale di oggetti, divisi in chunk da 2^5=32
typedef gos::HandleT< 6,5,16, 1,4>	GPUDescrPoolHandle;			//2^6=64 => num totale di oggetti, divisi in chunk da 2^5=32
typedef gos::HandleT< 8,5,16, 0,3>	GPUPipelineHandle;			//2^8=256 => num totale di oggetti, divisi in chunk da 2^5=32
typedef gos::HandleT< 8,6,16, 0,2>	GPUCmdBufferHandle;			//2^8=256 => num totale di oggetti, divisi in chunk da 2^6=64
typedef gos::HandleT< 8,7,16, 0,1>	GPURenderLayoutHandle;		//2^8=256 => num totale di oggetti, divisi in chunk da 2^7=128
typedef gos::HandleT< 9,7,16, 0,0>	GPUFrameBufferHandle;		//2^9=512 => num totale di oggetti, divisi in chunk da 2^7=128
typedef gos::HandleT<10,5,16, 0,1>	GPUVtxDeclHandle;			//2^10=1024 => num totale di oggetti, divisi in chunk da 2^5=32
typedef gos::HandleT<10,7,14, 0,1>	GPUVtxBufferHandle;			//2^10=1024 => num totale di oggetti, divisi in chunk da 2^7=128
typedef gos::HandleT<10,7,14, 1,0>	GPUIdxBufferHandle;			//2^10=1024 => num totale di oggetti, divisi in chunk da 2^7=128
typedef gos::HandleT<10,7,13, 0,2>	GPUStgBufferHandle;			//2^10=1024 => num totale di oggetti, divisi in chunk da 2^7=128
typedef gos::HandleT<10,8,12, 0,2>	GPUDescrSetLayoutHandle;	//2^10=1024 => num totale di oggetti, divisi in chunk da 2^8=256
typedef gos::HandleT<10,8,12, 1,1>	GPUDescrSetInstancerHandle;	//2^10=1024 => num totale di oggetti, divisi in chunk da 2^8=256
typedef gos::HandleT<10,8,12, 2,0>	GPUUniformBufferHandle;		//2^10=1024 => num totale di oggetti, divisi in chunk da 2^8=256
typedef gos::HandleT<14,8,10, 0,0>	GPUShaderHandle;			//2^14=16384 => num totale di oggetti, divisi in chunk da 2^8=256


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

enum class eZFunc : u8
{
	NEVER           = 0,
	LESS            = 1,
	EQUAL           = 2,
	LESS_EQUAL      = 3,
	GREATER         = 4,
	NOT_EQUAL       = 5,
	GREATER_EQUAL   = 6,
	ALWAYS          = 7 
};

enum class eStencilOp : u8
{
	KEEP       		= 0,
	ZERO       		= 1,
	REPLACE    		= 2,
	INCR_AND_CLAMP  = 3,
	DECR_AND_CLAMP  = 4,
	INVERT     		= 5,
	INCR_AND_WRAP   = 6,
	DECR_AND_WRAP   = 7 
};

enum class eStencilFunc : u8
{
	NEVER           = 0,
	LESS            = 1,
	EQUAL           = 2,
	LESS_EQUAL      = 3,
	GREATER         = 4,
	NOT_EQUAL       = 5,
	GREATER_EQUAL   = 6,
	ALWAYS          = 7 
};

enum class eCullMode : u8
{
	NONE	= 0,
	CW		= 1,
	CCW		= 2
};

enum class eRenderTargetUsage : u8
{
	presentation = 0,   	//usato per essere present() a video
	storage,             	//viene renderizzato e, al termine del rendering,  il suo contenuto deve essere preservato
	storage_discard     	//viene renderizzato ma, al termine del rendering, il suo contenuto non ci interessa piu'
};

enum class eVIBufferMode : u8
{
	onGPU			= 0,	//risiede in memoria GPU quindi per essere updatato necessita di uno stagin buffer e di una transferQ
	mappale			= 1,	//puo' essere updatato (trampite map/unmpa), ma da non farsi molto di frequente
	unknown			= 0xff
};


struct sVtxDescriptor
{
	u8              streamIndex;
	u8              bindingLocation;
	eDataFormat     format;
	u8              offset;
};




#endif//_gosGPUEnumAndDefine_h_