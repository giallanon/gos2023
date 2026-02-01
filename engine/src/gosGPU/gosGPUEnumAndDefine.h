#ifndef _gosGPUEnumAndDefine_h_
#define _gosGPUEnumAndDefine_h_
#include "../gos/gosEnumAndDefine.h"
#include "../gos/dataTypes/gosColorHDR.h"
#include "../gos/gosHandle.h"
#include "../gosShape/gosShapeEnumAndDefine.h"
//#include "vulkan/gosGPUVulkanEnumAndDefine.h"

#define 	GOSGPU__NUM_MAX_VTXDECL_ATTR					32
#define 	GOSGPU__NUM_MAX_VXTDECL_STREAM					16
#define 	GOSGPU__NUM_MAX_ATTACHMENT						16
#define 	GOSGPU__NUM_MAX_DESCRIPTOR_PER_SET				8
#define 	GOSGPU__NUM_MAX_DESCRIPTOR_SETS					8
#define 	GOSGPU__NUM_MAX_DESCRIPTOR_POOL_SIZE_PER_POOL	16
#define 	GOSGPU__NUM_MAX_PUSH_CONSTANT_PER_PIPELINE		16
#define 	GOSGPU__NUM_MAX_PUSH_CONSTANT_RANGE_PER_PIPELINE	4
#define 	GOSGPU__NUM_MAX_WRITE_DESCRIPTORS_PER_CMDBUFFER	8
#define 	GOSGPU__NUM_MAX_SUBPASSES						4
#define 	GOSGPU__NUM_MAX_SHADER_PER_PIPELINE				8


GOS_DECL_HANDLE( 32,8, GPUZBufferHandle);				//32 => num totale di oggetti, divisi in chunk da 8
GOS_DECL_HANDLE( 32,16, GPUViewportHandle);				//32 => num totale di oggetti, divisi in chunk da 16
GOS_DECL_HANDLE( 64,32, GPURenderTargetHandle);			//64 => num totale di oggetti, divisi in chunk da 32
GOS_DECL_HANDLE( 64,32, GPUDescrPoolHandle);			//64 => num totale di oggetti, divisi in chunk da 32
GOS_DECL_HANDLE( 256,32, GPUPipelineHandle);			//256 => num totale di oggetti, divisi in chunk da 32
GOS_DECL_HANDLE( 256,32, GPUSamplerHandle);				//256 => num totale di oggetti, divisi in chunk da 32
GOS_DECL_HANDLE( 256,64, GPUCmdBufferHandle);			//256 => num totale di oggetti, divisi in chunk da 64
GOS_DECL_HANDLE(1024,128, GPUVtxBufferHandle);			//1024 => num totale di oggetti, divisi in chunk da 128
GOS_DECL_HANDLE(1024,128, GPUIdxBufferHandle);			//1024 => num totale di oggetti, divisi in chunk da 128
GOS_DECL_HANDLE(1024,128, GPUStgBufferHandle);			//1024 => num totale di oggetti, divisi in chunk da 128
GOS_DECL_HANDLE(1024,256, GPUDescrSetLayoutHandle);		//1024 => num totale di oggetti, divisi in chunk da 256
GOS_DECL_HANDLE(1024,256, GPUDescrSetInstanceHandle);	//1024 => num totale di oggetti, divisi in chunk da 256
GOS_DECL_HANDLE(1024,256, GPUUniformBufferHandle);		//1024 => num totale di oggetti, divisi in chunk da 256
GOS_DECL_HANDLE(16384,256, GPUStorageBufferHandle);		//16384 => num totale di oggetti, divisi in chunk da 256
GOS_DECL_HANDLE(16384,256, GPUShaderHandle);			//16384 => num totale di oggetti, divisi in chunk da 256
GOS_DECL_HANDLE(65536,1024, GPUTextureHandle);			//65536 => num totale di oggetti, divisi in chunk da 1024


enum class eVtxStreamInputRate : u8
{
	perVertex = 0,
	perInstance = 1
};

enum class eMemAccessMode : u8
{
	invalid 				= 0,
	onGPU					= 1,	//risiede in memoria GPU quindi per essere updatato necessita di uno stagin buffer e di una transferQ
	shared_cpuW_autoSync	= 2,	//cpu puo' scrivere nel buffer tramite writeAndSync()
									//Questa modalita' e' buona per buffer piccoli, tipo gli uniform

	shared_cpuW_manualSync	= 3,	//cpu puo' scrivere nel buffer ma deve prima map()/unmap() e infine chiamare buffer_manualSync_cpuWrite()
									//Utile per buffer di grosse dimensioni che vengono (raramente) aggiornati a "pezzi"

	readback				= 4,	//cpu puo' leggere ma deve prima map()/unmap() e infine chiamare buffer_manualSync_cpuRead()
};

enum class eSamplerFilter : u8
{
	point = 0,
	linear = 1
};

enum class eSamplerMipFilter : u8
{
	nearest = 0,
	linear = 1
};

enum class eSamplerCompFunc : u8
{
    NEVER = 0,
    LESS = 1,
    EQUAL = 2,
    LESS_OR_EQUAL = 3,
    GREATER = 4,
    NOT_EQUAL = 5,
    GREATER_OR_EQUAL = 6,
    ALWAYS = 7,	
	DISABLED = 8
};

enum class eSamplerAddressMode : u8
{
	REPEAT = 0,
    MIRRORED_REPEAT = 1,
    CLAMP_TO_EDGE = 2,
    CLAMP_TO_BORDER = 3,
    MIRROR_CLAMP_TO_EDGE = 4
};   


#endif//_gosGPUEnumAndDefine_h_