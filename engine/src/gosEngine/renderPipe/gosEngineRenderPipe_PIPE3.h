#ifndef _gosEngineRenderPipe_PIPE3_h_
#define _gosEngineRenderPipe_PIPE3_h_
#include "gosEngineRenderPipe.h"
#include "../entity/gosEntityDefaultComponents.h"

namespace gos
{
	class Engine; //fwd

	namespace engine
	{
		/**********************************************
		* Renderer_PIPE3
		* 
		*/
		class Renderer_PIPE3 : public gos::engine::RenderPipe::Renderer
		{
		public:
			using RPIPE = gos::engine::RenderPipe;

		public:
			struct Material
			{
				vec3f	diffuse_col;
				u32		texture_index;
			};	

		public:
					Renderer_PIPE3();
					~Renderer_PIPE3()                                                                           { priv_unsetup(); }

			bool 	on__attach (const RPIPE::Context &ctx, u8 renderer_UID) final;
			void 	on__detach (const RPIPE::Context &ctx) final                                                { priv_unsetup(); }
			void 	on__render (const RPIPE::Context &ctx, gos::gpu::RenderCtx &rctx) final						{ priv_do_render (ctx, rctx); }

			void    begin ();
			void    add (const ENGGPUShape shape, const mat4x4f &m, u32 material_index);
			void 	add (gos::ENGModel3dInst handle);
			void    add (const ent::CompModelInstance *mi);
			void    end ();

			//==================== gestione materiali
			u32             material_create (u32 texture_index, const vec3f diffuse_col);
			void            material_delete (u32 material_index);
			Material*       material_getForUpdate (u32 material_index);
			const Material* material_query (u32 material_index) const;

		private:
			static constexpr u32    NUM_MAX_MATERIAL    = 1024;
			static constexpr u32    NUM_MAX_MATRIX      = 150000;
						
		private:
			void    priv_unsetup();		
			u64     priv_pack_renderable (ENGGPUShape shape, u32 material_index, u32 matrix_index) const;
			void    priv_unpack_renderable (u64 packed, ENGGPUShape *out_shape, u32 *out_material_index, u32 *out_matrix_index) const;
			void    priv_do_render (const RPIPE::Context &ctx, gpu::RenderCtx &rctx);

		private:
			gos::Allocator              *localAllocator;
			gos::Engine                 *engine;
			gos::GPU                    *gpu;
			u8							renderer_UID;

			ENGPipeline 				handle_pipeline;
			GPUDescrSetInstanceHandle   handle_descrSet2;
			GPUStorageBufferHandle      handle_sbo_matrixList;
			GPUStorageBufferHandle      handle_sbo_materiaList;
			GPUStorageBufferHandle      handle_sbo_instanceData;
			
			mat4x4f                   	matrix_default;
			gpu::sMappedBuffer			matrix_buffer;
			u32							matrix_sizeof_buffer;
			u32							matrix_nextIndex;
			
			Material					material_default;
			Material					*material_buffer;
			u32							material_sizeof_buffer;
			gos::Bitfield				material_bitmask;
			u32							material_wasUpdated;

			gpu::sMappedBuffer			instance_buffer;
			u32							instance_sizeof_buffer;

			u64							*pRenderableList;
			u32							nRenderable;
		};
	} //namespace engine
} //namespace gos


#endif //_gosEngineRenderPipe_PIPE3_h_
