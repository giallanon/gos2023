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
	num_block_to_render = 0;
	map = NULL;
}

//********************************
void Renderer::on__detach (const gos::engine::RenderPipe::Context &ctx)
{
	gpu->buffer_unmap (sbo_instance_data.mapped_buffer);
	gpu->deleteResource(sbo_instance_data.handle_sbo);

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

	priv__create_block_geometry(NUM_VTX_PER_LATO);

	//SBO instance data
	assert (sbo_instance_data.handle_sbo.isInvalid());
	{
    	sbo_instance_data.sizeof_buffer = NUM_MAX_CHUNK * sizeof(SBO_instance_data::Elem);
	    gpu->storageBuffer_create (sbo_instance_data.sizeof_buffer, eMemAccessMode::shared_cpuW_manualSync, &sbo_instance_data.handle_sbo);
    	gpu->map (sbo_instance_data.handle_sbo, 0, u32MAX, &sbo_instance_data.mapped_buffer);
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
			return false;
		}
		else
		{
			dsw.begin (gpu, handle_descrSet2)
				.bindStorageBuffer (0, sbo_instance_data.handle_sbo)
				//.bindStorageBuffer (1, sbo_chunk_data.handle_sbo)
				//.bindStorageBuffer (2, sbo_material_data.handle_sbo)
				//.bindStorageBuffer (3, sbo_meshInstanceData.handle_sbo)
				.end();
		}
	}	
	return true;
}

//********************************
void Renderer::bind_map (land::Map *mapIN)
{
	map = mapIN;
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

	gpu::StageHelper stageHelper;
	const u32 NUM_TOT_VTX = num_vtx_per_lato * num_vtx_per_lato;
	const u32 NUM_TOT_IDX = (num_vtx_per_lato-1) * (num_vtx_per_lato-1) * 6;
	const u32 SIZEOF_VB = NUM_TOT_VTX * sizeof(Vertex);
	const u32 SIZEOF_IB = NUM_TOT_IDX * sizeof(u16);
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
}

//********************************
void Renderer::add (const MapQTree *mapQTree, const land::ChunkCoordList &list)
{
	SBO_instance_data::Elem *pInstanceData = reinterpret_cast<SBO_instance_data::Elem*>( sbo_instance_data.mapped_buffer.host_pt );
	const u32 N = list.getNElem();
	for (u32 i=0; i<N; i++)
	{
		const land::ChunkCoord cc = list(i);

		geom::AABB3 aabb;
		mapQTree->aabb_from_chunkCoord (cc, &aabb);

		const u8 lod = cc.get_lod();
		const f32 scaleXZ = aabb.vmax.x - aabb.vmin.x;
		const vec2f tutv_offset (0.25f *(f32)(lod % 4), 0.25f *(f32)(lod / 4));

		
		pInstanceData[num_block_to_render].chunk_originXZ.set (aabb.vmin.x, aabb.vmax.z);
		pInstanceData[num_block_to_render].scale_XZ.set (scaleXZ, scaleXZ);
		pInstanceData[num_block_to_render].tutv_offset = tutv_offset;
		pInstanceData[num_block_to_render].tutv_scale.set (0.25f, 0.25f);
		num_block_to_render++;
	}
}

//********************************
void Renderer::end()
{
	if (0 == num_block_to_render)
		return;

    //aggiornamento SBO_instance_data
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

	constexpr u32 NUM_TOT_IDX = (NUM_VTX_PER_LATO-1) * (NUM_VTX_PER_LATO-1) * 6;
	//command
    rctx.bindPipeline (res_pipeline->pipeHandle)
        .bindDescriptorSet (ctx.handle_descrSet0, 0)
        .bindDescriptorSet (ctx.handle_descrSet1, 1)
        .bindDescriptorSet (handle_descrSet2, 2);


	rctx.bindVtxIdxBuffer (handle_vb, 0, handle_ib, 0);
	rctx.drawIndexed (NUM_TOT_IDX, num_block_to_render, 0, 0, 0);
}



