#ifndef _gosEngineRes2_res_h_
#define _gosEngineRes2_res_h_
#include "enumAndDefine.h"
#include "../gosEngineEnumAndDefine.h"


namespace gos
{
	namespace res2
	{
		//************************************
		struct ResVtxBuffer
		{
			struct Data
			{
				GPUVtxBufferHandle	vbHandle;
			};

			ResDescr	base;
			Data		data;
		};

		//************************************
		struct ResIdxBuffer
		{
			struct Data
			{
				GPUIdxBufferHandle	ibHandle;
			};

			ResDescr	base;
			Data		data;
		};

		//************************************
		struct ResShader
		{
			struct Data
			{
				GPUShaderHandle		shaderHandle;
			};

			ResDescr	base;
			Data		data;
		};

		//************************************
		struct ResPipeline
		{
			struct Data
			{
				GPUPipelineHandle	pipeHandle;
			};

			ResDescr	base;
			Data		data;
		};

		//************************************
		struct ResTexture2d
		{
			struct Data
			{
				GPUTextureHandle	texHandle;
				u32					index;
			};

			ResDescr	base;
			Data		data;
		};

		//************************************
		struct ResShape
		{
			struct Data
			{
				gos::Shape	shape;
			};

			ResDescr	base;
			Data		data;
		};

		//************************************
		struct ResGPUShape
		{
			struct Data
			{
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
			struct Data
			{
				gos::Skeleton	skeleton;
			};

			ResDescr	base;
			Data		data;
		};

		

	

	} //namespace res2
} //namespace gos

#endif //_gosEngineRes2_res_h_
