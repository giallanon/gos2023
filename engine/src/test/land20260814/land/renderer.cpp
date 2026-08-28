#include "renderer.h"

using namespace gos;
using namespace land;


//********************************
Renderer::Renderer()
{
	localAllocator = gos::getSysHeapAllocator();
	eng = NULL;
	gpu = NULL;
	num_block_to_render = 0;
	map = NULL;
}

//********************************
Renderer::~Renderer()
{
}

//********************************
void Renderer::map__bind (const land::Map *mapIN)
{
	//TODO:  se map != NULL, bisogna fare il free di VB/IB prima di ricrearlo
	//		bisogna fare anche il free degli SBO se gia' esistenti
	map = mapIN;
	
	assert (handle_vb.isInvalid());
	priv__calc_LOD_details (map->chunk__get_num_vtx_per_lato(), map->chunk__get_border_length__m());
	priv__create_block_geometry (map->chunk__get_num_vtx_per_lato(), map->chunk__get_lod0_scala_xz__m());

	const u32 NUM_VTX_PER_CHUNK = map->chunk__get_num_vtx_per_lato() * map->chunk__get_num_vtx_per_lato();

	//SBO instance data
	assert (sbo_instance_data.handle_sbo.isInvalid());
	{
    	sbo_instance_data.sizeof_buffer = NUM_MAX_CHUNK * sizeof(SBO_instance_data::Elem);
	    gpu->storageBuffer_create (sbo_instance_data.sizeof_buffer, eMemAccessMode::shared_cpuW_manualSync, &sbo_instance_data.handle_sbo);
    	gpu->map (sbo_instance_data.handle_sbo, 0, u32MAX, &sbo_instance_data.mapped_buffer);
	}

	//SBO chunk data
	assert (sbo_chunk_data.handle_sbo.isInvalid());
	{
    	sbo_chunk_data.sizeof_buffer = NUM_MAX_CHUNK * NUM_VTX_PER_CHUNK * sizeof(SBO_chunk_data::Elem);
	    gpu->storageBuffer_create (sbo_chunk_data.sizeof_buffer, eMemAccessMode::shared_cpuW_manualSync, &sbo_chunk_data.handle_sbo);
    	gpu->map (sbo_chunk_data.handle_sbo, 0, u32MAX, &sbo_chunk_data.mapped_buffer);
	}	

	//mi serve che la pipe sia loaded
    const res::Pipeline *res_pipeline;
    if (eng->get (handle_pipeline, &res_pipeline, 5000))
    {
		//creo i descriptor set
		gos::gpu::DescrSetInstanceWriter dsw;

		//descriptor set 2        
		if (!gpu->descrSetInstance_create (handle_descrPool, res_pipeline->pipeHandle, 2, &handle_descrSet2))
		{
			gos::logger::err ("Renderer::setup() => can't create an instance of descriptorSet_2\n");
			return;
		}
		else
		{
			dsw.begin (gpu, handle_descrSet2)
				.bindStorageBuffer (0, sbo_instance_data.handle_sbo)
				.bindStorageBuffer (1, sbo_chunk_data.handle_sbo)
				//.bindStorageBuffer (2, sbo_packedInstanceData.handle_sbo)
				//.bindStorageBuffer (3, sbo_meshInstanceData.handle_sbo)
				.end();
		}
	}
}

//********************************
void Renderer::on__detach (const gos::engine::RenderPipe::Context &ctx)
{
	for (u8 lod=0; lod<num_lod; lod++)
	{
		GOSDELETE(localAllocator, lod_bucket_list[lod].chunk_index_list);
	}

	gpu->buffer_unmap (sbo_instance_data.mapped_buffer);
	gpu->deleteResource(sbo_instance_data.handle_sbo);

	gpu->buffer_unmap (sbo_chunk_data.mapped_buffer);
	gpu->deleteResource(sbo_chunk_data.handle_sbo);

	gpu->deleteResource(handle_descrSet2);
	gpu->deleteResource(handle_vb);
	gpu->deleteResource(handle_ib);
	eng->release(handle_pipeline);
	eng->release(handle_texture_lod);
}

//********************************
bool Renderer::on__attach (const gos::engine::RenderPipe::Context &ctx, u8 renderer_UID)
{
	this->eng = ctx.engine;
	this->gpu = ctx.engine->gpu;
	this->handle_descrPool = ctx.handle_descrPool;

	//load pipe
	if (!ctx.engine->pipeline_createFromAsset ("land20260814", &handle_pipeline, res::eLoadMode::asap))
	{
		logger::err ("Renderer::on__attach() => can't load pipeline\n");
        return false; 
	}	

	//load texture LOD
	eng->texture2D_createFromAsset ("tex_lod", &handle_texture_lod, res::eLoadMode::asap);

	return true;
}

//********************************
void Renderer::priv__calc_LOD_details (u32 num_vtx_per_lato_max_LOD, f32 chunk_border_len__m)
{
	assert(GOS_IS_POWER_OF_TWO(num_vtx_per_lato_max_LOD-1));
	memset (lod_info_list, 0, sizeof(lod_info_list));
	
	u32 n = num_vtx_per_lato_max_LOD;
	u32 num_tot_indices = 0;
	num_lod = 0;
	while (n > 8)
	{
		lod_info_list[num_lod].starting_index = num_tot_indices;
		lod_info_list[num_lod].num_indices = (n-1) * (n-1) * 6;
		num_tot_indices += lod_info_list[num_lod].num_indices;

		num_lod++;
		n = (n-1) / 2;
		n++;

		if (num_lod == MAX_LOD)
			break;
	}
	assert (num_lod <= MAX_LOD);

	for (u32 i=0; i<num_lod; i++)
	{
		lod_bucket_list[i].chunk_index_list = GOSNEW(localAllocator, gos::FastArray<u32>)(localAllocator, 512);
	}


	const f32 lod_distance[MAX_LOD] = {
		(chunk_border_len__m * 1.5f) * 1.8f,
		chunk_border_len__m * 1.6f,
		chunk_border_len__m * 1.6f,
		chunk_border_len__m * 1.6f,
		chunk_border_len__m,
		chunk_border_len__m,
		chunk_border_len__m,
		chunk_border_len__m
	};

	lod_info_list[0].max_distance_sq__m = lod_distance[0];
	for (u8 i=1; i<MAX_LOD; i++)
		lod_info_list[i].max_distance_sq__m = lod_info_list[i-1].max_distance_sq__m + lod_distance[i];

	for (u8 i=0; i<MAX_LOD; i++)
		lod_info_list[i].max_distance_sq__m *= lod_info_list[i].max_distance_sq__m;


}

//********************************
void Renderer::priv__create_block_geometry (u32 num_vtx_per_lato_max_LOD, f32 scala_XZ)
{
	assert (NULL != gpu);
	assert (handle_vb.isInvalid());
	assert (handle_ib.isInvalid());


	struct Vertex
	{
		vec2f pos;
		vec2f tutv;
	};

	gpu::StageHelper stageHelper;
	const u32 NUM_TOT_VTX = num_vtx_per_lato_max_LOD * num_vtx_per_lato_max_LOD;
	const u32 NUM_TOT_IDX = lod_info_list[num_lod-1].starting_index + lod_info_list[num_lod-1].num_indices;
	const u32 SIZEOF_VB = NUM_TOT_VTX * sizeof(Vertex);
	const u32 SIZEOF_IB = NUM_TOT_IDX * sizeof(u16);
	stageHelper.setup (gpu, GOSMAX(SIZEOF_VB, SIZEOF_IB));

	//creazione VB
	gpu->vertexBuffer_create (SIZEOF_VB, eMemAccessMode::onGPU, &handle_vb);
	{
		Vertex *vb = GOSALLOCT(Vertex*, gos::getScrapAllocator(), SIZEOF_VB);
		u32 ct = 0;
		u32 zz = num_vtx_per_lato_max_LOD;
		const f32 tuvInc = 1.0f / (f32)num_vtx_per_lato_max_LOD;
		f32 tv = 0;
		for (u32 z=0; z<num_vtx_per_lato_max_LOD; z++)
		{
			zz--;

			f32 tu = 0;
			for (u32 x=0; x<num_vtx_per_lato_max_LOD; x++)
			{
				vb[ct].pos.set ( (f32)x * scala_XZ, (f32)zz * scala_XZ);
				vb[ct].tutv.set (tu, tv);
				tu += tuvInc;
				ct++;
			}

			tv += tuvInc;
		}

		stageHelper.begin()
			.mem_to_buffer (vb, SIZEOF_VB, handle_vb, 0)
			.submit();

		GOSFREE(gos::getScrapAllocator(), vb);
	}
	


	//creo un IB che contiene tutti i lod
	gpu->indexBuffer_create (SIZEOF_IB, eMemAccessMode::onGPU, &handle_ib);
	{
		u16 *ib = GOSALLOCT(u16*, gos::getScrapAllocator(), SIZEOF_IB);

		u32 idx_num = 0;
		u32 inc = 1;
		for (u32 i=0; i<num_lod; i++)
		{
			for (u32 z=0; z<num_vtx_per_lato_max_LOD-inc; z+=inc)
			{
				const u32 one_row = num_vtx_per_lato_max_LOD * inc;
				u32 v = z * num_vtx_per_lato_max_LOD;
				for (u32 x=0; x<num_vtx_per_lato_max_LOD-inc; x+=inc)
				{
					ib[idx_num++] = v;
					ib[idx_num++] = v+inc;
					ib[idx_num++] = v+inc + one_row;

					ib[idx_num++] = v+inc + one_row;
					ib[idx_num++] = v     + one_row;
					ib[idx_num++] = v;

					v+=inc;
				}
			}

			inc*=2;
		}
		assert (idx_num == NUM_TOT_IDX);

		stageHelper.begin()
			.mem_to_buffer (ib, SIZEOF_IB, handle_ib, 0)
			.submit();

		GOSFREE(gos::getScrapAllocator(), ib);
	}

}

//********************************
void Renderer::begin()
{
	assert (NULL != map);

	num_block_to_render = 0;
	for (u32 i=0; i<num_lod; i++)
	{
		lod_bucket_list[i].chunk_index_list->reset();
	}
}

//********************************
void Renderer::add (const gos::FastArray<ChunkCoord> &list)
{
	//smisto i chunk nei vari LOD
	for (u32 i=0; i<list.getNElem(); i++)
	{
		u8 lod = 0;
		while (lod<num_lod)
		{
			if (list(i).distance2_from_pov < lod_info_list[lod].max_distance_sq__m)
			{
				lod_bucket_list[lod].chunk_index_list->append (i);
				break;
			}
			lod++;
		}

		if (lod == num_lod)
		{
			lod_bucket_list[num_lod-1].chunk_index_list->append (i);
		}
	}


	u32 ct = 0;
	u32 ct_chunkData = 0;
	SBO_chunk_data::Elem *pChunkData = reinterpret_cast<SBO_chunk_data::Elem*>( sbo_chunk_data.mapped_buffer.host_pt );
	SBO_instance_data::Elem *pInstanceData = reinterpret_cast<SBO_instance_data::Elem*>( sbo_instance_data.mapped_buffer.host_pt );
	for (u8 lod=0; lod<num_lod; lod++)
	{
		const u32 n = lod_bucket_list[lod].chunk_index_list->getNElem();
		for (u32 i=0; i<n; i++)
		{
			const u32 index = lod_bucket_list[lod].chunk_index_list->queryElem(i);
			pInstanceData[ct].chunk_originXZ.x = list(index).originWC.x;
			pInstanceData[ct].chunk_originXZ.y = list(index).originWC.z;
			pInstanceData[ct].chunk_data_offset = 0;
			pInstanceData[ct].pad0 = lod;


			//altezze
			const Map::ChunkData *chunk_data = map->chunk__get(list(index).chunk_x, list(index).chunk_y);
			assert (NULL != chunk_data);
			{
				pInstanceData[ct].chunk_data_offset = ct_chunkData;

				u32 ct_map = 0;
				const u32 N = map->chunk__get_num_vtx_per_lato();
				for (u32 y=0; y<N; y++)
				{
					for (u32 x=0; x<N; x++)
					{
						const u16 h = chunk_data[ct_map++].height;
						pChunkData[ct_chunkData].height_and_pad = (u32)h;
						ct_chunkData++;
					}
				}				
			}


			//next chunk
			ct++;
			num_block_to_render++;
		}
	}


    //aggiornamento sbo_chunk_data
	{
		u32 size = ct_chunkData * sizeof(SBO_chunk_data::Elem);
		const u32 r = size % gpu->limits_get_nonCoherentAtomSize();
		if (r)
			size += gpu->limits_get_nonCoherentAtomSize() - r;
		
		assert (size <= sbo_chunk_data.sizeof_buffer);
		gpu->buffer_manualSync_cpuWrite (sbo_chunk_data.mapped_buffer, 0, size);	
	}	
}

//********************************
void Renderer::end()
{
    //aggiornamento sbo_block_data
	{
		u32 size = num_block_to_render * sizeof(SBO_instance_data::Elem);
		const u32 r = size % gpu->limits_get_nonCoherentAtomSize();
		if (r)
			size += gpu->limits_get_nonCoherentAtomSize() - r;
		
		assert (size <= sbo_instance_data.sizeof_buffer);
		gpu->buffer_manualSync_cpuWrite (sbo_instance_data.mapped_buffer, 0, size);
	}


}

//********************************
void Renderer::on__render (const gos::engine::RenderPipe::Context &ctx, gos::gpu::RenderCtx &rctx)
{
    const res::Pipeline *res_pipeline;
    if (!ctx.engine->get (handle_pipeline, &res_pipeline))
    {
        return;
    }

	//command
    rctx.bindPipeline (res_pipeline->pipeHandle)
        .bindDescriptorSet (ctx.handle_descrSet0, 0)
        .bindDescriptorSet (ctx.handle_descrSet1, 1)
        .bindDescriptorSet (handle_descrSet2, 2);


	u32 first_instance_index = 0;
	rctx.bindVtxIdxBuffer (handle_vb, 0, handle_ib, 0);
	for (u8 lod=0; lod<num_lod; lod++)
	{
		const u32 n = lod_bucket_list[lod].chunk_index_list->getNElem();
		if (n)
		{
			rctx.drawIndexed (lod_info_list[lod].num_indices, n, lod_info_list[lod].starting_index, 0, first_instance_index);
			first_instance_index += n;
		}
	}
}



