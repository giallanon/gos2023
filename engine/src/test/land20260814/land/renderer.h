#ifndef _land_renderer_h_
#define _land_renderer_h_
#include "map.h"

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
				~Renderer();

		void 	map__bind (const land::Map *map);

		bool 	on__attach (const gos::engine::RenderPipe::Context &ctx, u8 renderer_UID);
		void 	on__detach (const gos::engine::RenderPipe::Context &ctx);
		void 	on__render (const gos::engine::RenderPipe::Context &ctx, gos::gpu::RenderCtx &rctx);

		void	begin();
		void 	add (const gos::FastArray<ChunkCoord> &list);
		void 	end();

	private:
		static constexpr u32 NUM_MAX_CHUNK = 8192;
		static constexpr u8 MAX_LOD = 8;

	private:
		struct LODInfo
		{
			u32		num_indices;
			u32		starting_index;
			f32		max_distance_sq__m;
		};

		struct LODBucket
		{
			gos::FastArray<u32>	*chunk_index_list;
		};

		struct SBO_instance_data
		{
			struct Elem
			{
				gos::vec2f	chunk_originXZ;
				u32			chunk_data_offset;
				u32			pad0;
			};

			GPUStorageBufferHandle	handle_sbo;
			gos::gpu::sMappedBuffer	mapped_buffer;
			u32						sizeof_buffer;
		};

		struct SBO_chunk_data
		{
			struct Elem
			{
				u32			height_and_pad;
			};

			GPUStorageBufferHandle	handle_sbo;
			gos::gpu::sMappedBuffer	mapped_buffer;
			u32						sizeof_buffer;
		};

	private:
		void	priv__calc_LOD_details (u32 num_vtx_per_lato_max_LOD, f32 chunk_border_len__m);
		void	priv__create_block_geometry (u32 num_vtx_per_lato_max_LOD, f32 scala_XZ);


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

		const land::Map 		*map;
		u8						num_lod;
		LODInfo					lod_info_list[MAX_LOD];
		LODBucket				lod_bucket_list[MAX_LOD];

		SBO_instance_data		sbo_instance_data;
		SBO_chunk_data			sbo_chunk_data;
		u32 					num_block_to_render;
	};

} //namespace land


#endif //_land_renderer_h_

