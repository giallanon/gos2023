#ifndef _gosEngineRes_h_
#define _gosEngineRes_h_
#include "gosEngineEnumAndDefine.h"
#include "model/gosModelInstance.h"


namespace gos
{
	namespace engine
	{
		//************************************
		struct ResVtxBuffer
		{
		public:
			void reset()		{ brh.reset(); vbHandle.setInvalid(); }

		public:
			Resource		brh;
			GPUVtxBufferHandle	vbHandle;
		};


		//************************************
		struct ResIdxBuffer
		{
		public:
			void reset()		{ brh.reset(); ibHandle.setInvalid(); }

		public:
			Resource		brh;
			GPUIdxBufferHandle	ibHandle;
		};


		//************************************
		struct ResShader
		{
		public:
			struct Data
			{
				GPUShaderHandle	shaderHandle;
				
				void 	reset()													{ shaderHandle.setInvalid(); }
				void 	destroy (gos::Allocator *allocator, gos::GPU *gpu)		{ gpu->deleteResource (shaderHandle); reset(); }
			};

		public:
			void reset()		{ brh.reset(); data.reset(); }

		public:
			Resource		brh;
			Data				data;
		};	


		//************************************
		struct ResTexture
		{
		public:
			struct Data
			{
				void 	reset()													{ texHandle.setInvalid(); }
				void 	destroy (gos::Allocator *allocator, gos::GPU *gpu)		{ gpu->deleteResource (texHandle); reset(); }

				GPUTextureHandle	texHandle;
			};

		public:
			void reset()		{ brh.reset(); data.reset(); }

		public:
			Resource		brh;
			Data				data;
		};
				
		
		//************************************
		struct ResGPUShape
		{
		public:
			void reset()		{ brh.reset(); handle_shape.setInvalid(); vbHandle.setInvalid(); ibHandle.setInvalid(); numIndices=numVertex=0; alloc_vtxbuf_offset=alloc_vtxbuf_size=alloc_idxbuf_offset=alloc_idxbuf_size=0; }

		public:
			Resource		brh;

			ENGShape			handle_shape;	//se valid, indica la ENGShape dalla quale this e' stata creata
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


		//************************************
		struct ResShape
		{
		public:
			struct Data
			{
				void 	reset()													{ shape.reset(); }
				void 	destroy (gos::Allocator *allocator, gos::GPU *gpu)		{ shape::shapeFree (allocator, &shape); reset(); }

				gos::Shape	shape;
			};

		public:
			void reset()		{ brh.reset(); data.reset(); }

		public:
			Resource	brh;
			Data			data;
		};


		//************************************
		struct ResPipeline
		{
		public:
			struct Data
			{
				GPUPipelineHandle	pipeHandle;
				
				void 	reset()													{ pipeHandle.setInvalid(); }
				void 	destroy (gos::Allocator *allocator, gos::GPU *gpu)		{ gpu->deleteResource (pipeHandle); reset(); }
			};

		public:
			void reset()		{ brh.reset(); data.reset(); }

		public:
			Resource		brh;
			Data				data;
		};


		//************************************
		struct ResSkeleton
		{
		public:
			struct Data
			{
				void 	reset()													{ skeleton.reset(); }
				void 	destroy (gos::Allocator *allocator, gos::GPU *gpu)		{ skeleton::free (skeleton); reset(); }

				gos::Skeleton	skeleton;
			};

		public:
			void reset()		{ brh.reset(); data.reset(); }

		public:
			Resource	brh;
			Data			data;
		};


		//************************************
		struct ResModel3d
		{
		public:
			struct Data
			{
				void 	reset()													{ model.reset(); }
				void 	destroy (gos::Allocator *allocator, gos::GPU *gpu)		{ model::free(model); reset(); }

				gos::Model	model;
			};

		public:
			void reset()		{ brh.reset(); data.reset(); }

		public:
			Resource	brh;
			Data			data;
		};
		

		//************************************
		struct ResModel3dInst
		{
		public:
			struct Data
			{
				void 	reset()													{ minst.reset(); }
				void 	destroy (gos::Allocator *allocator, gos::GPU *gpu)		{ minst.free(); reset(); }

				gos::ModelInstance	minst;
			};

		public:
			void reset()		{ brh.reset(); data.reset(); }

		public:
			Resource	brh;
			Data			data;
		};		
	} //namespace engine

} //namespace gos


#endif //_gosEngineRes_h_

