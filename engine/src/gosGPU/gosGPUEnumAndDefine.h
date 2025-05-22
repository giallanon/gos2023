#ifndef _gosGPUEnumAndDefine_h_
#define _gosGPUEnumAndDefine_h_
#include "../gos/gosEnumAndDefine.h"
#include "../gos/dataTypes/gosColorHDR.h"
#include "../gos/gosHandle.h"
#include "../gosShape/gosShapeEnumAndDefine.h"
#include "vulkan/gosGPUVulkanEnumAndDefine.h"

#define 	GOSGPU__NUM_MAX_VTXDECL_ATTR					32
#define 	GOSGPU__NUM_MAX_VXTDECL_STREAM					16
#define 	GOSGPU__NUM_MAX_ATTACHMENT						16
#define 	GOSGPU__NUM_MAX_DESCRIPTOR_PER_SET				32
#define 	GOSGPU__NUM_MAX_DESCRIPTOR_POOL_SIZE_PER_POOL	16
#define 	GOSGPU__NUM_MAX_PUSH_CONSTANT_PER_PIPELINE		16
#define 	GOSGPU__NUM_MAX_WRITE_DESCRIPTORS_PER_CMDBUFFER	8


//A per "num max di handle", B per "num di chunk", C per "counter"
typedef gos::HandleT< 5,3,16, 0,8>	GPUDepthStencilHandle;		//2^5=32 => num totale di oggetti, divisi in chunk da 2^3=8
typedef gos::HandleT< 5,5,16, 0,6>	GPUViewportHandle;			//2^5=32 => num totale di oggetti, divisi in chunk da 2^5=32
typedef gos::HandleT< 6,5,16, 0,5>	GPURenderTargetHandle;		//2^6=64 => num totale di oggetti, divisi in chunk da 2^5=32
typedef gos::HandleT< 6,5,16, 1,4>	GPUDescrPoolHandle;			//2^6=64 => num totale di oggetti, divisi in chunk da 2^5=32
typedef gos::HandleT< 8,5,16, 0,3>	GPUPipelineHandle;			//2^8=256 => num totale di oggetti, divisi in chunk da 2^5=32
typedef gos::HandleT< 8,5,15, 0,4>	GPUSamplerHandle;			//2^8=256 => num totale di oggetti, divisi in chunk da 2^5=32
typedef gos::HandleT< 8,6,16, 0,2>	GPUCmdBufferHandle;			//2^8=256 => num totale di oggetti, divisi in chunk da 2^6=64
typedef gos::HandleT< 8,7,16, 0,1>	GPURenderLayoutHandle;		//2^8=256 => num totale di oggetti, divisi in chunk da 2^7=128
typedef gos::HandleT< 9,7,16, 0,0>	GPUFrameBufferHandle;		//2^9=512 => num totale di oggetti, divisi in chunk da 2^7=128
typedef gos::HandleT<10,5,16, 0,1>	GPUVtxDeclHandle;			//2^10=1024 => num totale di oggetti, divisi in chunk da 2^5=32
typedef gos::HandleT<10,7,14, 0,1>	GPUVtxBufferHandle;			//2^10=1024 => num totale di oggetti, divisi in chunk da 2^7=128
typedef gos::HandleT<10,7,14, 1,0>	GPUIdxBufferHandle;			//2^10=1024 => num totale di oggetti, divisi in chunk da 2^7=128
typedef gos::HandleT<10,7,13, 0,2>	GPUStgBufferHandle;			//2^10=1024 => num totale di oggetti, divisi in chunk da 2^7=128
typedef gos::HandleT<10,8,12, 0,2>	GPUDescrSetLayoutHandle;	//2^10=1024 => num totale di oggetti, divisi in chunk da 2^8=256
typedef gos::HandleT<10,8,12, 1,1>	GPUDescrSetInstanceHandle;	//2^10=1024 => num totale di oggetti, divisi in chunk da 2^8=256
typedef gos::HandleT<10,8,12, 2,0>	GPUUniformBufferHandle;		//2^10=1024 => num totale di oggetti, divisi in chunk da 2^8=256
typedef gos::HandleT<14,8,8,  0,2>	GPUStorageBufferHandle;		//2^10=1024 => num totale di oggetti, divisi in chunk da 2^8=256
typedef gos::HandleT<14,8,10, 0,0>	GPUShaderHandle;			//2^14=16384 => num totale di oggetti, divisi in chunk da 2^8=256
typedef gos::HandleT<16,10,6, 0,0>	GPUTextureHandle;			//2^16=65536 => num totale di oggetti, divisi in chunk da 2^10=1024


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

enum class eImageLayout : u8
{
	undefined = 0,					//VK_IMAGE_LAYOUT_UNDEFINED
	general,						//VK_IMAGE_LAYOUT_GENERAL
    color_attachment_optimal, 		//VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    shader_readonly,				//VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	transfer_src,					//VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
    transfer_dst,					//VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
	presentation,					//VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
};

enum class eAttachmentLoadOp : u8
{
    load = 0, 		//VK_ATTACHMENT_LOAD_OP_LOAD
    clear,			//VK_ATTACHMENT_LOAD_OP_CLEAR
    dont_care		//VK_ATTACHMENT_LOAD_OP_DONT_CARE
};

enum class eAttachmentStoreOp : u8
{
	store = 0, 		//VK_ATTACHMENT_STORE_OP_STORE
	dont_care,		//VK_ATTACHMENT_STORE_OP_DONT_CARE
    none			//VK_ATTACHMENT_STORE_OP_NONE
};

enum class eDepthStencilLayout : u8
{
	undefined = 0,						//VK_IMAGE_LAYOUT_UNDEFINED
    depth_attachment_optimal, 			//VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    depth_shader_readonly,				//VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL;
};

enum class eVIBufferMode : u8
{
	onGPU					= 0,	//risiede in memoria GPU quindi per essere updatato necessita di uno stagin buffer e di una transferQ
	shared_cpuW_autoSync	= 1,	//cpu puo' scrivere nel buffer tramite writeAndSync()
									//Questa modalita' e' buona per buffer piccoli, tipo gli uniform

	shared_cpuW_manualSync	= 2,	//cpu puo' scrivere nel buffer ma deve prima map()/unmap() e infine chiamare manualSync()
									//Utile per buffer di grosse dimensioni che vengono (raramente) aggiornati a "pezzi"
	unknown					= 0xff
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

namespace gos
{
	namespace gpu
	{
		struct sVtxDescriptor
		{
			u8              streamIndex;
			u8              bindingLocation;
			eDataFormat     format;
			u8              offset;
		};

		struct sPipeline
		{
			void    reset ()                        { vkPipelineLayoutHandle = VK_NULL_HANDLE; vkPipelineHandle = VK_NULL_HANDLE; memset (pushContantList, 0, sizeof(pushContantList)); }

			VkPipelineLayout    vkPipelineLayoutHandle;
			VkPipeline          vkPipelineHandle;
			VkPushConstantRange pushContantList[GOSGPU__NUM_MAX_PUSH_CONSTANT_PER_PIPELINE];
		};  


		struct sMappedBuffer
		{
            void            *host_pt;
			VkDeviceMemory  _vkMemHandle;
            u32             offset;
            u32             size;
		};
	} //namespace gpu
} //namespace gos


#endif//_gosGPUEnumAndDefine_h_