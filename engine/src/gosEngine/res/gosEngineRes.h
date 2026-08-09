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
			u32					index;
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
		struct MaterialPBR
		{
		public:
			static constexpr u32 NUM_MAX_RENDERER = 4;

		public:
			void 	set_default_material_params()						{ diffuse_col_HDR_RGBA.set (1,1,1,1); diffuse_texture_index = 0; }

		public:
			res::Descr		_descr;
			u32				renderer_bindings[NUM_MAX_RENDERER];

			//parametri di materiale
			gos::vec4f		diffuse_col_HDR_RGBA;
			u32				diffuse_texture_index;

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
			mat4x4f 			matW;
		};		

	} //namespace res
} //namespace gos

#endif //_gosEngineRes2_h_