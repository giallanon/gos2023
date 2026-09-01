#ifndef _land_renderer_h_
#define _land_renderer_h_
#include "map.h"
#include "materialList.h"
#include "Cache.h"

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
		void 	add (const land::QTreeCoordList &list);
		void 	end();

	private:
		static constexpr u32 	NUM_MAX_CHUNK_INSTANCE = 4096;

	private:
		struct SBO_instance_data
		{
			struct Elem
			{
				gos::vec2f	chunk_originXZ;
				gos::vec2f	tutv_offset;
				f32 		scale_XZ;
				u32 		chunk_data_offset;
			};

			GPUStorageBufferHandle	handle_sbo;
			gos::gpu::MappedBufW	mapped;
		};
		
		struct SBO_chunk_data
		{
			struct Elem
			{
				u32 encoded_norm;
				u32	height_and_stuff; //8bit ao, 8bit material, 16bitLSB per height
			};

			GPUStorageBufferHandle	handle_sbo;
			gos::gpu::MappedBufW	mapped;
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
		u32						num_tot_idx;
		u32						num_vtx_per_lato;


		SBO_instance_data		sbo_instance_data;
		SBO_chunk_data			sbo_chunk_data;
		u32 					num_instance_to_render;
		land::Map				*map;
		land::PointData			*pointData;
		u32 					sizeof_pointData;
		SBO_chunk_data::Elem 	*chunk_data_elem_buffer;
		CacheLRU<QTreeCoord>	cached_chunk_data_list;
	};

} //namespace land


#endif //_land_renderer_h_

