#ifndef _gosEngineRes2_h_
#define _gosEngineRes2_h_
#include "../gosEngineEnumAndDefine.h"
#include "gosEngineResMan.h"
#include "../model/gosModelInstance.h"



namespace gos
{
	namespace res
	{
		//************************************
		struct VtxBuffer
		{
			res::Descr			_descr;
			GPUVtxBufferHandle	vbHandle;
		};

		//************************************
		struct IdxBuffer
		{
			res::Descr			_descr;
			GPUIdxBufferHandle	ibHandle;
		};

		struct Shader
		{
			res::Descr			_descr;
			GPUShaderHandle		shaderHandle;
		};

		//************************************
		struct Pipeline
		{
		public:
			res::Descr			_descr;
			GPUPipelineHandle	pipeHandle;
		};

		//************************************
		struct Texture2d
		{
		public:
			res::Descr			_descr;
			GPUTextureHandle	texHandle;
		};

		//************************************
		struct Shape
		{
		public:
			res::Descr	_descr;
			gos::Shape	shape;
		};

		//************************************
		struct GPUShape
		{
		public:
			res::Descr			_descr;

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
		struct Skeleton
		{
		public:
			res::Descr		_descr;
			gos::Skeleton	skeleton;
		};

		//************************************
		struct Model3d
		{
		public:
			res::Descr		_descr;
			gos::Model		model;
		};		

		struct Model3dInst
		{
		public:
			res::Descr			_descr;
			gos::ModelInstance	minst;
		};		

	} //namespace res
} //namespace gos

#endif //_gosEngineRes2_h_