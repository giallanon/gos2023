#ifndef _land_renderer_h_
#define _land_renderer_h_
#include "map.h"
#include "materialList.h"

namespace land
{
	/****************************************
	 * @brief	Renderer
	 * 
	 */
	class Renderer : public gos::engine::RenderPipe::Renderer
	{
	public:
				Renderer();
				~Renderer()																		{ }

		bool 	on__attach (const gos::engine::RenderPipe::Context &ctx, u8 renderer_UID);
		void 	on__detach (const gos::engine::RenderPipe::Context &ctx);
		void 	on__render (const gos::engine::RenderPipe::Context &ctx, gos::gpu::RenderCtx &rctx);

		void	bind_map (land::Map *map);

		void	begin();
		void 	add (const MapQTree *mapQTree, const land::ChunkCoordList &list);
		void 	end();

	private:
		static constexpr u32 	NUM_MAX_CHUNK = 8192;
		static constexpr u32	NUM_VTX_PER_LATO = 4;

	private:
		struct SBO_instance_data
		{
			struct Elem
			{
				gos::vec2f	chunk_originXZ;
				gos::vec2f	scale_XZ;
				gos::vec2f	tutv_offset;
				gos::vec2f	tutv_scale;
			};

			GPUStorageBufferHandle	handle_sbo;
			gos::gpu::sMappedBuffer	mapped_buffer;
			u32						sizeof_buffer;
		};

	private:
		void 	priv__create_block_geometry (u32 num_vtx_per_lato);

	private:
		gos::Allocator			*localAllocator;
		gos::Engine				*eng;
		gos::GPU				*gpu;
		gos::ENGPipeline 		handle_pipeline;
		GPUDescrSetInstanceHandle  handle_descrSet2;
		GPUDescrPoolHandle    	handle_descrPool;
		GPUVtxBufferHandle		handle_vb;
		GPUIdxBufferHandle		handle_ib;
		gos::ENGTexture			handle_texture_lod;


		SBO_instance_data		sbo_instance_data;
		u32 					num_block_to_render;
		land::Map				*map;
	};

} //namespace land


#endif //_land_renderer_h_

