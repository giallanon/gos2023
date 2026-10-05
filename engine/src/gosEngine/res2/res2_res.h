#ifndef _gosEngineRes2_res2_res_h_
#define _gosEngineRes2_res2_res_h_
#include "enumAndDefine.h"
#include "../../gosGPU/gosGPU.h"
#include "../gosShape/gosShape.h"
#include "../gosShape/skeleton/gosSkeleton.h"


namespace gos
{
	namespace res2
	{
		//************************************
		struct ResVtxBuffer
		{
			static constexpr eResType type = eResType::vtx_buffer;

			struct Data
			{
				void 	reset()		{ vbHandle.setInvalid(); }
				GPUVtxBufferHandle	vbHandle;
			};

			ResDescr	base;
			Data		data;
		};

		//************************************
		struct ResIdxBuffer
		{
			static constexpr eResType type = eResType::idx_buffer;

			struct Data
			{
				void 	reset()		{ ibHandle.setInvalid(); }
				GPUIdxBufferHandle	ibHandle;
			};

			ResDescr	base;
			Data		data;
		};

		//************************************
		struct ResVtxShader
		{
			static constexpr eResType type = eResType::vtx_shader;

			struct Data
			{
				void 	reset()		{ shaderHandle.setInvalid(); }
				GPUShaderHandle		shaderHandle;
			};

			ResDescr	base;
			Data		data;
		};

		//************************************
		struct ResPxlShader
		{
			static constexpr eResType type = eResType::pxl_shader;

			struct Data
			{
				void 	reset()		{ shaderHandle.setInvalid(); }
				GPUShaderHandle		shaderHandle;
			};

			ResDescr	base;
			Data		data;
		};		

		//************************************
		struct ResPipeline
		{
			static constexpr eResType type = eResType::pipeline;

			struct Data
			{
				void 	reset()		{ pipeHandle.setInvalid(); }
				GPUPipelineHandle	pipeHandle;
			};

			ResDescr	base;
			Data		data;
		};

		//************************************
		struct ResTexture2d
		{
			static constexpr eResType type = eResType::texture_2d;

			struct Data
			{
				void 	reset()		{ texHandle.setInvalid(); index=u32MAX; }
				GPUTextureHandle	texHandle;
				u32					index;
			};

			ResDescr	base;
			Data		data;
		};

		//************************************
		struct ResShape
		{
			static constexpr eResType type = eResType::shape;

			struct Data
			{
				void 	reset()		{ shape.reset(); }
				gos::Shape	shape;
			};

			ResDescr	base;
			Data		data;
		};

		//************************************
		struct ResGPUShape
		{
			static constexpr eResType type = eResType::gpu_shape;

			struct Data
			{
				void 	reset()
				{ 
					vbHandle.setInvalid();
					ibHandle.setInvalid();
					numIndices = 0;
					numVertex=0;
					alloc_vtxbuf_offset = 0;
					alloc_vtxbuf_size = 0;
					alloc_idxbuf_offset = 0;
					alloc_idxbuf_size = 0;						
				}
				
				GPUVtxBufferHandle	vbHandle;
				GPUIdxBufferHandle	ibHandle;
				u32					indexStart;		//posizione del primo idx di questa shape all'interno di vbHandle
				u32					numIndices;
				u32					vtxStart;		//posizione del primo vtx di questa shape all'interno di vbHandle
				u32					numVertex;

				u32					alloc_vtxbuf_offset;
				u32					alloc_vtxbuf_size;
				u32					alloc_idxbuf_offset;
				u32					alloc_idxbuf_size;		
			};

			ResDescr	base;
			Data		data;
		};

		//************************************
		struct ResSkeleton
		{
			static constexpr eResType type = eResType::skeleton;

			struct Data
			{
				void 	reset()		{ skeleton.reset();; }
				gos::Skeleton	skeleton;
			};

			ResDescr	base;
			Data		data;
		};

		

	

	} //namespace res2
} //namespace gos

#endif //_gosEngineRes2_res2_res_h_
