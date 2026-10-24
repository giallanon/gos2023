#ifndef _gosEngineRes2_res2_res_h_
#define _gosEngineRes2_res2_res_h_
#include "enumAndDefine.h"
#include "../../gosGPU/gosGPU.h"
#include "../gosShape/gosShape.h"
#include "../gosShape/skeleton/gosSkeleton.h"
#include "../model/gosModelEnumAndDefine.h"

namespace gos
{
	namespace res2
	{
		//************************************
		struct VtxBuffer
		{
			static constexpr eResType type = eResType::vtx_buffer;

			struct Data
			{
				void 	reset()		{ vbHandle.setInvalid(); }
				GPUVtxBufferHandle	vbHandle;
			};

			struct Resource
			{
				static constexpr eResType	get_type() 		{ return type; }
				void 						data_reset()	{ data.reset(); }
				ResDescr	base;
				Data		data;
			};

			struct Asset
			{
				static constexpr eResType	get_type() 		{ return type; }
				void 						data_reset()	{ res_data_pt = NULL; }
				ResDescr	base;
				Data		*res_data_pt;
			};
		};

		//************************************
		struct IdxBuffer
		{
			static constexpr eResType type = eResType::idx_buffer;

			struct Data
			{
				void 	reset()		{ ibHandle.setInvalid(); }
				GPUIdxBufferHandle	ibHandle;
			};

			struct Resource
			{
				static constexpr eResType	get_type() 		{ return type; }
				void 						data_reset()	{ data.reset(); }
				ResDescr	base;
				Data		data;
			};

			struct Asset
			{
				static constexpr eResType	get_type() 		{ return type; }
				void 						data_reset()	{ res_data_pt = NULL; }
				ResDescr	base;
				Data		*res_data_pt;
			};
		};

		//************************************
		struct Texture2d
		{
			static constexpr eResType type = eResType::texture_2d;
			static constexpr eResType	get_type() 		{ return type; }

			struct Data
			{
				void 	reset()		{ texHandle.setInvalid(); index=u32MAX; }
				GPUTextureHandle	texHandle;
				u32					index;
			};

			struct Resource
			{
				static constexpr eResType	get_type() 		{ return type; }
				void 						data_reset()	{ data.reset(); }
				ResDescr	base;
				Data		data;
			};

			struct Asset
			{
				static constexpr eResType	get_type() 		{ return type; }
				void 						data_reset()	{ res_data_pt = NULL; }
				ResDescr	base;
				Data		*res_data_pt;
			};
		};

		//************************************
		struct MaterialPBR
		{
			static constexpr eResType type = eResType::materialPBR;
			static constexpr eResType	get_type() 		{ return type; }

			struct Data
			{
				static constexpr u32 NUM_MAX_RENDERER = 4;
				void 	reset()		{ memset(renderer_bindings, 0xff, sizeof(renderer_bindings)); update_ct=0; diffuse_col_HDR_RGBA.set(1,1,1,1); diffuse_texture = normal_texture = NULL; }
				
				u32				renderer_bindings[NUM_MAX_RENDERER];
				u32				update_ct; //viene incrementato da engine ogni volta che si chiama materialPRB_get_for_updatre
				vec4f			diffuse_col_HDR_RGBA;
				Texture2d::Data	*diffuse_texture;
				Texture2d::Data	*normal_texture;
			};

			struct Updatable
			{
				ResDescr	*base;
				Data		*data;
			};

			struct Resource
			{
				static constexpr eResType	get_type() 		{ return type; }
				void 						data_reset()	{ data.reset(); }
				ResDescr	base;
				Data		data;
			};

			struct Asset
			{
				static constexpr eResType	get_type() 		{ return type; }
				void 						data_reset()	{ res_data_pt = NULL; }
				ResDescr	base;
				Data		*res_data_pt;
			};
		};
		
		//************************************
		struct VtxShader
		{
			static constexpr eResType type = eResType::vtx_shader;
			static constexpr eResType	get_type() 		{ return type; }


			struct Data
			{
				void 	reset()		{ vtxshader_handle.setInvalid(); }
				GPUShaderHandle		vtxshader_handle;
			};

			struct Resource
			{
				static constexpr eResType	get_type() 		{ return type; }
				void 						data_reset()	{ data.reset(); }
				ResDescr	base;
				Data		data;
			};

			struct Asset
			{
				static constexpr eResType	get_type() 		{ return type; }
				void 						data_reset()	{ res_data_pt = NULL; }
				ResDescr	base;
				Data		*res_data_pt;
			};
		};

		//************************************
		struct PxlShader
		{
			static constexpr eResType type = eResType::pxl_shader;
			static constexpr eResType	get_type() 		{ return type; }


			struct Data
			{
				void 	reset()		{ pxlshader_handle.setInvalid(); }
				GPUShaderHandle		pxlshader_handle;
			};

			struct Resource
			{
				static constexpr eResType	get_type() 		{ return type; }
				void 						data_reset()	{ data.reset(); }
				ResDescr	base;
				Data		data;
			};

			struct Asset
			{
				static constexpr eResType	get_type() 		{ return type; }
				void 						data_reset()	{ res_data_pt = NULL; }
				ResDescr	base;
				Data		*res_data_pt;
			};
		};

		//************************************
		struct Pipeline
		{
			static constexpr eResType type = eResType::pipeline;
			static constexpr eResType	get_type() 		{ return type; }


			struct Data
			{
				void 	reset()		{ pipe_handle.setInvalid(); }
				GPUPipelineHandle	pipe_handle;
			};

			struct Resource
			{
				static constexpr eResType	get_type() 		{ return type; }
				void 						data_reset()	{ data.reset(); }
				ResDescr	base;
				Data		data;
			};

			struct Asset
			{
				static constexpr eResType	get_type() 		{ return type; }
				void 						data_reset()	{ res_data_pt = NULL; }
				ResDescr	base;
				Data		*res_data_pt;
			};
		};		

		//************************************
		struct GPUShape
		{
			static constexpr eResType type = eResType::gpu_shape;
			static constexpr eResType	get_type() 		{ return type; }

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

			struct Resource
			{
				static constexpr eResType	get_type() 		{ return type; }
				void 						data_reset()	{ data.reset(); }
				ResDescr	base;
				Data		data;
			};

			struct Asset
			{
				static constexpr eResType	get_type() 		{ return type; }
				void 						data_reset()	{ res_data_pt = NULL; }
				ResDescr	base;
				Data		*res_data_pt;
			};
		};
		
		//************************************
		struct Skeleton
		{
			static constexpr eResType type = eResType::skeleton;
			static constexpr eResType	get_type() 		{ return type; }

			struct Data
			{
				void 	reset()		{ skeleton.reset();; }
				gos::Skeleton	skeleton;
			};

			struct Resource
			{
				static constexpr eResType	get_type() 		{ return type; }
				void 						data_reset()	{ data.reset(); }
				ResDescr	base;
				Data		data;
			};

			struct Asset
			{
				static constexpr eResType	get_type() 		{ return type; }
				void 						data_reset()	{ res_data_pt = NULL; }
				ResDescr	base;
				Data		*res_data_pt;
			};
		};		

		//************************************
		struct Model3d
		{
			static constexpr eResType type = eResType::model_3d;
			static constexpr eResType	get_type() 		{ return type; }

			struct Data
			{
				void 	reset()		{ model.reset(); }
				gos::Model	model;
			};

			struct Updatable
			{
				ResDescr	*base;
				Data		*data;
				u32			num_mesh;
				u32			num_material;
				u32 		num_gpushape;
			};

			struct Resource
			{
				static constexpr eResType	get_type() 		{ return type; }
				void 						data_reset()	{ data.reset(); }
				ResDescr	base;
				Data		data;
			};

			struct Asset
			{
				static constexpr eResType	get_type() 		{ return type; }
				void 						data_reset()	{ res_data_pt = NULL; }
				ResDescr	base;
				Data		*res_data_pt;
			};
		};

		//************************************
		struct ModelInstance
		{
			static constexpr eResType type = eResType::model_instance;
			static constexpr eResType	get_type() 		{ return type; }

			struct Data
			{
				void 	reset()		{ matW.identity(); listof_bones=NULL; model_data=NULL; skeleton_data=NULL; }
				
				gos::mat4x4f			matW;
				gos::Bone				*listof_bones;
				const Model3d::Data		*model_data;
				const Skeleton::Data	*skeleton_data;
			};

			struct Resource
			{
				static constexpr eResType	get_type() 		{ return type; }
				void 						data_reset()	{ data.reset(); }
				ResDescr	base;
				Data		data;
			};

			struct Asset
			{
				static constexpr eResType	get_type() 		{ return type; }
				void 						data_reset()	{ res_data_pt = NULL; }
				ResDescr	base;
				Data		*res_data_pt;
			};			
		};

		
	
		/************************************
		struct ResModelInstance
		{
			static constexpr eResType type = eResType::model_instance;

			struct Data
			{
				void 	reset()		{ matW.identity(); model.reset(); }
				mat4x4f 			matW;
				ResModel3d::Data	model;
			};

			ResDescr	base;
			Data		data;
		};		*/

	} //namespace res2
} //namespace gos

#endif //_gosEngineRes2_res2_res_h_
