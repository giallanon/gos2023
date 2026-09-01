#include "renderer.h"
#include "land.h"

using namespace gos;
using namespace land;


//********************************
Renderer::Renderer()
{
	localAllocator = gos::getSysHeapAllocator();
	eng = NULL;
	gpu = NULL;
	num_instance_to_render = 0;
	map = NULL;
	pointData = NULL;
	sizeof_pointData = 0;
	
	chunk_data_elem_buffer = NULL;
	num_vtx_per_lato = 0;
}

//********************************
void Renderer::on__detach (const gos::engine::RenderPipe::Context &ctx)
{
	if (NULL != pointData)	GOSFREE_AND_NULL(localAllocator, pointData);
	
	if (NULL != chunk_data_elem_buffer)	GOSFREE_AND_NULL(localAllocator, chunk_data_elem_buffer);

	gpu->deleteResource(sbo_instance_data.handle_sbo);
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
	if (!ctx.engine->pipeline_createFromAsset ("land20260829", &handle_pipeline, res::eLoadMode::asap))
	{
		logger::err ("Renderer::on__attach() => can't load pipeline\n");
        return false; 
	}	

	//load texture LOD
	eng->texture2D_createFromAsset ("tex_lod", &handle_texture_lod, res::eLoadMode::asap);

	//SBO instance data
	assert (sbo_instance_data.handle_sbo.isInvalid());
	{
    	const u32 size = NUM_MAX_CHUNK_INSTANCE * sizeof(SBO_instance_data::Elem);
	    gpu->storageBuffer_create (size, eMemAccessMode::shared_cpuW, &sbo_instance_data.handle_sbo);
	}

	//mi serve che la pipe sia loaded
    const res::Pipeline *res_pipeline;
    GOS_DEBUG_ASSERT( eng->get (handle_pipeline, &res_pipeline, 5000) );
    {
		//creo i descriptor set
		
		//descriptor set 2        
		if (!gpu->descrSetInstance_create (handle_descrPool, res_pipeline->pipeHandle, 2, &handle_descrSet2))
		{
			gos::logger::err ("Renderer::setup() => can't create an instance of descriptorSet_2\n");
			return false;
		}
	}	
	return true;
}

//********************************
void Renderer::bind_map (land::Map *mapIN)
{
	map = mapIN;
	num_vtx_per_lato = map->qtree__get_num_vtx_per_chunk_side();
	priv__create_block_geometry (num_vtx_per_lato);

	sizeof_pointData = sizeof(land::PointData) * num_vtx_per_lato * num_vtx_per_lato;
	pointData = GOSALLOCT(land::PointData*, localAllocator, sizeof_pointData);
	
	const u32 sizeof_chunkData = sizeof(SBO_chunk_data::Elem) * num_vtx_per_lato * num_vtx_per_lato;
	chunk_data_elem_buffer = GOSALLOCT(SBO_chunk_data::Elem*, localAllocator, sizeof_chunkData);

	cached_chunk_data_list.setup (localAllocator, NUM_MAX_CHUNK_INSTANCE, num_vtx_per_lato * num_vtx_per_lato);

	//SBO chunk data
	assert (sbo_chunk_data.handle_sbo.isInvalid());
	{
    	const u32 size = NUM_MAX_CHUNK_INSTANCE * sizeof(SBO_chunk_data::Elem) * num_vtx_per_lato * num_vtx_per_lato;
	    gpu->storageBuffer_create (size, eMemAccessMode::shared_cpuW, &sbo_chunk_data.handle_sbo);
	}

	gos::gpu::DescrSetInstanceWriter dsw;
	dsw.begin (gpu, handle_descrSet2)
		.bindStorageBuffer (0, sbo_instance_data.handle_sbo)
		.bindStorageBuffer (1, sbo_chunk_data.handle_sbo)
		//.bindStorageBuffer (2, sbo_material_data.handle_sbo)
		//.bindStorageBuffer (3, sbo_meshInstanceData.handle_sbo)
		.end();

}

//********************************
void Renderer::priv__create_block_geometry (u32 num_vtx_per_lato)
{
	assert (NULL != gpu);
	assert (handle_vb.isInvalid());
	assert (handle_ib.isInvalid());

	struct Vertex
	{
		vec2f pos;
		vec2f tutv;
	};

	this->num_tot_idx = (num_vtx_per_lato-1) * (num_vtx_per_lato-1) * 6;

	gpu::StageHelper stageHelper;
	const u32 NUM_TOT_VTX = num_vtx_per_lato * num_vtx_per_lato;
	const u32 SIZEOF_VB = NUM_TOT_VTX * sizeof(Vertex);
	const u32 SIZEOF_IB = num_tot_idx * sizeof(u16);
	stageHelper.setup (gpu, GOSMAX(SIZEOF_VB, SIZEOF_IB));

	//creazione VB
	gpu->vertexBuffer_create (SIZEOF_VB, eMemAccessMode::onGPU, &handle_vb);
	{
		Vertex *vb = GOSALLOCT(Vertex*, gos::getScrapAllocator(), SIZEOF_VB);
		u32 ct = 0;
		const f32 tuvInc = 1.0f / (f32)(num_vtx_per_lato-1);
		f32 tv = 0;
		f32 zz = 0;
		for (u32 z=0; z<num_vtx_per_lato; z++)
		{
			f32 tu = 0;
			for (u32 x=0; x<num_vtx_per_lato; x++)
			{
				vb[ct].pos.set ( tu, zz);
				vb[ct].tutv.set (tu, tv);
				tu += tuvInc;
				ct++;
			}

			tv += tuvInc;
			zz -= tuvInc;
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
		for (u32 z=0; z<num_vtx_per_lato-1; z++)
		{
			const u32 one_row = num_vtx_per_lato;
			u32 v = z * one_row;
			for (u32 x=0; x<num_vtx_per_lato-1; x++)
			{
				ib[idx_num++] = v;
				ib[idx_num++] = v+1;
				ib[idx_num++] = v+1 + one_row;

				ib[idx_num++] = v+1 + one_row;
				ib[idx_num++] = v   + one_row;
				ib[idx_num++] = v;

				v++;
			}
		}
		assert (idx_num == num_tot_idx);

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
	num_instance_to_render = 0;

	gpu->begin_write (sbo_instance_data.handle_sbo, &sbo_instance_data.mapped);
	gpu->begin_write (sbo_chunk_data.handle_sbo, &sbo_chunk_data.mapped);
}

//********************************
void Renderer::add (const land::QTreeCoordList &list)
{
	const u32 N = list.getNElem();
	if (N > NUM_MAX_CHUNK_INSTANCE)
	{
		DBGBREAK;
		return;
	}

	const u32 timenow_msec = (u32)gos::getTimeSinceStart_msec();
	for (u32 i=0; i<N; i++)
	{
		const land::QTreeCoord cc = list(i);

		u32 cached_offset = 0;
		if (!cached_chunk_data_list.get_from_cache (timenow_msec, cc, &cached_offset))
		{
			//recupero i PointData di questo chunk
			if (map->map__get_data (cc, pointData, sizeof_pointData))
			{
				cached_offset = cached_chunk_data_list.get_a_slot (timenow_msec, cc);

				//altezze, normali e via dicendo
				const u32 NN = num_vtx_per_lato * num_vtx_per_lato;
				for (u32 iPoint=0; iPoint<NN; iPoint++)
				{
					u16 height_and_stuff = pointData[iPoint].height._encoded;
					height_and_stuff |= (u32)pointData[iPoint].materialID << 16;
					height_and_stuff |= (u32)pointData[iPoint].ao << 24;
					

					chunk_data_elem_buffer[iPoint].encoded_norm = pointData[iPoint].norm._encoded;
					chunk_data_elem_buffer[iPoint].height_and_stuff = height_and_stuff;
				}

				const u32 sizeNN = sizeof(SBO_chunk_data::Elem) * NN;
				sbo_chunk_data.mapped.write (chunk_data_elem_buffer, sizeNN, cached_offset * sizeof(SBO_chunk_data::Elem) );
			}
		}


		//parametri per l'istanza
		{
			geom::AABB3 aabb;
			map->qtree__aabb_from_coord (cc, &aabb);

			const u8 lod = cc.get_lod();
			const f32 scaleXZ = aabb.vmax.x - aabb.vmin.x;
			const vec2f tutv_offset (0.25f *(f32)(lod % 4), 0.25f *(f32)(lod / 4));

			SBO_instance_data::Elem elem;
				elem.chunk_originXZ.set (aabb.vmin.x, aabb.vmax.z);
				elem.scale_XZ = scaleXZ;
				elem.tutv_offset = tutv_offset;
				elem.chunk_data_offset = cached_offset;
			
			sbo_instance_data.mapped.writeT (elem, num_instance_to_render * sizeof(elem));
		}

		//recupero i PointData di questo chunk
		// GOS_DEBUG_ASSERT( map->map__get_data (cc, pointData, sizeof_pointData) );
		// 	SBO_chunk_data::Elem *cdeb = chunk_data_elem_buffer[iChunkDataElemBuffer++];
		// 	if (iChunkDataElemBuffer >= NUM_CHUNK_DATA_ELEM_BUFFER)
		// 		iChunkDataElemBuffer = 0;

		// 	//altezze, normali e via dicendo
		// 	const u32 NN = num_vtx_per_lato * num_vtx_per_lato;
		// 	for (u32 iPoint=0; iPoint<NN; iPoint++)
		// 	{
		// 		cdeb[iPoint].encoded_norm = pointData[iPoint].norm._encoded;
		// 		cdeb[iPoint].height_and_stuff = pointData[iPoint].height._encoded;
		// 		cdeb[iPoint].height_and_stuff |= (u32)pointData[iPoint].materialID << 16;
		// 		cdeb[iPoint].height_and_stuff |= (u32)pointData[iPoint].ao << 24;
		// 	}
		//  	const u32 sizeNN = sizeof(SBO_chunk_data::Elem) * NN;
		//  	sbo_chunk_data.mapped.write (cdeb, sizeNN, cached_offset);
		// 	cached_offset += sizeNN;

		// SBO_chunk_data::Elem *pp = reinterpret_cast<SBO_chunk_data::Elem*>(	sbo_chunk_data.mapped.buffer->mapped_host_pt );
		// GOS_DEBUG_ASSERT( map->map__get_data (cc, pointData, sizeof_pointData) );
		// 	//altezze, normali e via dicendo
		// 	const u32 NN = num_vtx_per_lato * num_vtx_per_lato;
		// 	u32 iPoint = 0;
		// 	for (u32 yy=0; yy<num_vtx_per_lato; yy++)
		// 	{
		// 		for (u32 xx=0; xx<num_vtx_per_lato; xx++)
		// 		{
		// 			u16 height_and_stuff = pointData[iPoint].height._encoded;

		// 			if (xx==1 || yy==1) height_and_stuff = 300;

		// 			height_and_stuff |= (u32)pointData[iPoint].materialID << 16;
		// 			height_and_stuff |= (u32)pointData[iPoint].ao << 24;


		// 			pp[cached_offset].encoded_norm = pointData[iPoint].norm._encoded;
		// 			pp[cached_offset].height_and_stuff = height_and_stuff;
		// 			cached_offset++;
		// 		}
		// 	}


		// GOS_DEBUG_ASSERT( map->map__get_data (cc, pointData, sizeof_pointData) );
		// 	//altezze, normali e via dicendo
		// 	const u32 NN = num_vtx_per_lato * num_vtx_per_lato;
		// 	for (u32 iPoint=0; iPoint<NN; iPoint++)
		// 	{
		// 		u16 height_and_stuff = pointData[iPoint].height._encoded;
		// 		height_and_stuff |= (u32)pointData[iPoint].materialID << 16;
		// 		height_and_stuff |= (u32)pointData[iPoint].ao << 24;

		// 		chunk_data_elem_buffer[iPoint].encoded_norm = pointData[iPoint].norm._encoded;
		// 		chunk_data_elem_buffer[iPoint].height_and_stuff = height_and_stuff;
		// 	}
		//  	const u32 sizeNN = sizeof(SBO_chunk_data::Elem) * NN;
		// 	sbo_chunk_data.mapped.write (chunk_data_elem_buffer, sizeNN, cached_offset);
		// 	cached_offset += sizeNN;

		num_instance_to_render++;
		
	}

	
}

//********************************
void Renderer::end()
{
	sbo_instance_data.mapped.end();
	sbo_chunk_data.mapped.end();
}

//********************************
void Renderer::on__render (const gos::engine::RenderPipe::Context &ctx, gos::gpu::RenderCtx &rctx)
{
	if (0 == num_instance_to_render)
		return;

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


	rctx.bindVtxIdxBuffer (handle_vb, 0, handle_ib, 0);
	rctx.drawIndexed (num_tot_idx, num_instance_to_render, 0, 0, 0);
}



