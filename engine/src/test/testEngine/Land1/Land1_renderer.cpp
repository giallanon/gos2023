#include "Land1_renderer.h"
#include "Land1_exaGenerator.h"

using namespace gos;
using namespace Land1;


//***************************************
Renderer::Renderer()
{
	localAllocator = NULL;
	engine = NULL;
	gpu = NULL;
	num_vtx = 0;
	num_quad = 0;
	map = NULL;
}

//***************************************
void Renderer::priv_unsetup()
{
	if (NULL == engine)
		return;

	engine->release(handle_pipeline);

	gpu->buffer_unmap (sbo_exaVtxList.mapped_buffer);
	gpu->buffer_unmap (sbo_exaVtxInfo.mapped_buffer);
	gpu->buffer_unmap (sbo_packedInstanceData.mapped_buffer);
	gpu->buffer_unmap (sbo_meshInstanceData.mapped_buffer);

    gpu->deleteResource(handle_descrSet2);
	gpu->deleteResource(sbo_exaVtxList.handle_sbo);
	gpu->deleteResource(sbo_exaVtxInfo.handle_sbo);
	gpu->deleteResource(sbo_packedInstanceData.handle_sbo);
	gpu->deleteResource(sbo_meshInstanceData.handle_sbo);

	engine->release(handle__model_tile1);

	const u8 N = (u8)Land1::eMeshType::_COUNT;
	for (u8 i=0; i<N; i++)
	{
		GOSFREE(localAllocator, mesh_instance_data[i].quad_index_list);
	}


	exaR_map.forEach ([localAllocator=this->localAllocator](u32 key, ExaR *exar) {
		Land1::ExaR::free (localAllocator, exar);
		return true;
	});
	exaR_map.reset();

	localAllocator = NULL;
	engine = NULL;
	gpu = NULL;
}

//***************************************
void Renderer::map_attach (Map2 *mapIN)
{
	map = mapIN;
}

//***************************************
bool Renderer::on__attach (const RPIPE::Context &ctx, u8 renderer_UID)
{
	//load pipe
	if (!ctx.engine->pipeline_createFromAsset ("land1_pipe", &handle_pipeline, res::eLoadMode::asap))
	{
		logger::err ("Land1_renderer::on__attach() => can't load pipeline\n");
        return false; 
	}	


	localAllocator = ctx.allocator;
	engine = ctx.engine;
	gpu = engine->gpu;


    //SBO hexaVtxList
	{
    	sbo_exaVtxList.sizeof_buffer = NUM_MAX_EXA * HEXA__NUM_VTX * sizeof(vec2f);
	    gpu->storageBuffer_create (sbo_exaVtxList.sizeof_buffer, eMemAccessMode::shared_cpuW_manualSync, &sbo_exaVtxList.handle_sbo);
    	gpu->map (sbo_exaVtxList.handle_sbo, 0, u32MAX, &sbo_exaVtxList.mapped_buffer);
	}

    //SBO exaVtxInfo
	{
    	sbo_exaVtxInfo.sizeof_buffer = NUM_MAX_EXA * 256 * sizeof(sVtxInfo);
    	gpu->storageBuffer_create (sbo_exaVtxInfo.sizeof_buffer, eMemAccessMode::shared_cpuW_manualSync, &sbo_exaVtxInfo.handle_sbo);
    	gpu->map (sbo_exaVtxInfo.handle_sbo, 0, u32MAX, &sbo_exaVtxInfo.mapped_buffer);
	}

    //SBO instance data
	{
    	sbo_packedInstanceData.sizeof_buffer = NUM_MAX_EXA * HEXA__MAX_NUM_QUAD * sizeof(sInstanceData);
    	gpu->storageBuffer_create (sbo_packedInstanceData.sizeof_buffer, eMemAccessMode::shared_cpuW_manualSync, &sbo_packedInstanceData.handle_sbo);
    	gpu->map (sbo_packedInstanceData.handle_sbo, 0, u32MAX, &sbo_packedInstanceData.mapped_buffer);
	}

	//SBO MeshInstanceData
	{
    	sbo_meshInstanceData.sizeof_buffer = NUM_MAX_EXA * HEXA__NUM_VTX * sizeof(u32);
	    gpu->storageBuffer_create (sbo_meshInstanceData.sizeof_buffer, eMemAccessMode::shared_cpuW_manualSync, &sbo_meshInstanceData.handle_sbo);
    	gpu->map (sbo_meshInstanceData.handle_sbo, 0, u32MAX, &sbo_meshInstanceData.mapped_buffer);
	}

	//mi serve che la pipe sia loaded
    const res::Pipeline *res_pipeline;
    if (engine->get (handle_pipeline, &res_pipeline, 5000))
    {
		//creo i descriptor set
		gos::gpu::DescrSetInstanceWriter dsw;

		//descriptor set 2        
		if (!gpu->descrSetInstance_create (ctx.handle_descrPool, res_pipeline->pipeHandle, 2, &handle_descrSet2))
		{
			gos::logger::err ("Renderer::setup() => can't create an instance of descriptorSet_2\n");
			return false;
		}
		else
		{
			dsw.begin (gpu, handle_descrSet2)
				.bindStorageBuffer (0, sbo_exaVtxList.handle_sbo)
				.bindStorageBuffer (1, sbo_exaVtxInfo.handle_sbo)
				.bindStorageBuffer (2, sbo_packedInstanceData.handle_sbo)
				.bindStorageBuffer (3, sbo_meshInstanceData.handle_sbo)
				.end();
		}
	}
	

	const u8 N = (u8)Land1::eMeshType::_COUNT;
	for (u8 i=0; i<N; i++)
	{
		mesh_instance_data[i].num_quad = 0;
		mesh_instance_data[i].quad_index_list = GOSALLOCT(u32*, localAllocator, sizeof(u32) * NUM_MAX_INSTANCE_PER_MESH);

	}


	//load risorse
	engine->model_createFromAsset ("model_tile1", &handle__model_tile1, res::eLoadMode::asap);
	

	exaR_map.setup (localAllocator, 128);


	//aspetto che le risorse siano caricate
	const res::Model3d *res_model;
	engine->get (handle__model_tile1, &res_model, 4000);

	//il modello ha delle shape, voglio sapere quali
	//queste shape sono gia' bindata a VB/IB
	gos::model::Reader mr;
	mr.setup (&res_model->model);
	shape_list = mr.gpushape_get_pt_to_list();


	return true;
}

//***************************************
void Renderer::priv_add_vtx (const gos::vec2f &v)
{
	if (num_vtx >= NUM_MAX_EXA * HEXA__NUM_VTX)
	{
		DBGBREAK;
		return;
	}

	u32 ct = num_vtx*2;
	f32 *p = reinterpret_cast<f32*>( sbo_exaVtxList.mapped_buffer.host_pt );
	p[ct++] = v.x;
	p[ct++] = v.y;

	num_vtx++;
}

//***************************************
void Renderer::priv_add_vtxInfo (u32 height, u32 material_index)
{
	if (num_vtxInfo >= NUM_MAX_EXA * 256)
	{
		DBGBREAK;
		return;
	}

	sVtxInfo *p = reinterpret_cast<sVtxInfo*>( sbo_exaVtxInfo.mapped_buffer.host_pt );
	p[num_vtxInfo].height = height;
	p[num_vtxInfo].material_index = material_index;

	num_vtxInfo++;
}

//***************************************
void Renderer::priv_add_quad (Land1::eMeshType mesh_type, u32 reference_vtx_index, u32 idx1, u32 idx2, u32 idx3, u32 idx4)
{
	if (num_quad >= NUM_MAX_INSTANCE_PER_MESH)
	{
		DBGBREAK;
		return;
	}

	assert (idx1 < u16MAX);
	assert (idx2 < u16MAX);
	assert (idx3 < u16MAX);
	assert (idx4 < u16MAX);

	sInstanceData *p = reinterpret_cast<sInstanceData*>( sbo_packedInstanceData.mapped_buffer.host_pt );
	p[num_quad].quad_indices_0_1 = (((u32)idx1) << 16) | (u32)idx2;
	p[num_quad].quad_indices_2_3 = (((u32)idx3) << 16) | (u32)idx4;
	p[num_quad].reference_vtx_index = reference_vtx_index;
	
	
	const u8 N = (u8)mesh_type;
	if (mesh_instance_data[N].num_quad >= NUM_MAX_INSTANCE_PER_MESH)
	{
		DBGBREAK;
		return;
	}

	const u32 NQ = mesh_instance_data[N].num_quad++;
	mesh_instance_data[N].quad_index_list[NQ] = num_quad;
	
	
	num_quad++;
}

//***************************************
void Renderer::on__render (const RPIPE::Context &ctx, gpu::RenderCtx &rctx)
{
    if (0 == num_quad)
        return;

    const res::Pipeline *res_pipeline;
    if (!engine->get (handle_pipeline, &res_pipeline))
    {
        return;
    }

    //aggiornamento exaVtx
    {
		u32 size = num_vtx * sizeof(vec2f);
		const u32 r = size % gpu->limits_get_nonCoherentAtomSize();
		if (r)
			size += gpu->limits_get_nonCoherentAtomSize() - r;
		
		assert (size <= sbo_exaVtxList.sizeof_buffer);
        gpu->buffer_manualSync_cpuWrite (sbo_exaVtxList.mapped_buffer, 0, size);
    }

    //aggiornamento exaVtxInfo
    {
		u32 size = num_vtxInfo * sizeof(sVtxInfo);
		const u32 r = size % gpu->limits_get_nonCoherentAtomSize();
		if (r)
			size += gpu->limits_get_nonCoherentAtomSize() - r;
		
		assert (size <= sbo_exaVtxInfo.sizeof_buffer);
        gpu->buffer_manualSync_cpuWrite (sbo_exaVtxInfo.mapped_buffer, 0, size);
    }	

    //aggiornamento packedInstanceData
    {
		u32 size = num_quad * sizeof(sInstanceData);
		const u32 r = size % gpu->limits_get_nonCoherentAtomSize();
		if (r)
			size += gpu->limits_get_nonCoherentAtomSize() - r;

		assert (size <= sbo_packedInstanceData.sizeof_buffer);
		gpu->buffer_manualSync_cpuWrite (sbo_packedInstanceData.mapped_buffer, 0, size);
    }    

	//aggiornamento meshInstanceData
	{
		u32 total_size = 0;
		for (u8 i=0; i<(u8)Land1::eMeshType::_COUNT; i++)
		{
			const u32 NQ = mesh_instance_data[i].num_quad;
			if (0 == NQ)
				continue;

			u32 size = NQ * sizeof(u32);
			const u32 r = size % gpu->limits_get_nonCoherentAtomSize();
			if (r)
				size += gpu->limits_get_nonCoherentAtomSize() - r;

			u8 *p = reinterpret_cast<u8*>( sbo_meshInstanceData.mapped_buffer.host_pt );
			memcpy (&p[total_size], mesh_instance_data[i].quad_index_list, sizeof(u32) * NQ);

			mesh_instance_data[i].start_index_in_SBO = total_size / sizeof(u32);
			total_size += size;
		}	

		assert (total_size <= sbo_meshInstanceData.sizeof_buffer);
		gpu->buffer_manualSync_cpuWrite (sbo_meshInstanceData.mapped_buffer, 0, total_size);

	}	

    
    //command
    rctx.bindPipeline (res_pipeline->pipeHandle)
        .bindDescriptorSet (ctx.handle_descrSet0, 0)
        .bindDescriptorSet (ctx.handle_descrSet1, 1)
        .bindDescriptorSet (handle_descrSet2, 2);

    //render delle shape
	for (u8 i=0; i<(u8)Land1::eMeshType::_COUNT; i++)
	{
		if (0 == mesh_instance_data[i].num_quad)
			continue;

		u32 meshType;
		static constexpr u32 MESH_INDEX__ANGOLO = 1;
		static constexpr u32 MESH_INDEX__ANGOLO_INTERNO = 2;
		static constexpr u32 MESH_INDEX__BORDO_SINGOLO_SU = 3;
		static constexpr u32 MESH_INDEX__BORDO_SINGOLO_DX = 4;		
		static constexpr u32 MESH_INDEX__FULL = 5;
		static constexpr u32 MESH_INDEX__STRANO = 6;
		
		
		switch ((Land1::eMeshType)i)
		{
		default:
			DBGBREAK;
			continue;

		case Land1::eMeshType::angolo:	meshType = MESH_INDEX__ANGOLO; break;
		case Land1::eMeshType::full:		meshType = MESH_INDEX__FULL; break;
		case Land1::eMeshType::angolo_interno:		meshType = MESH_INDEX__ANGOLO_INTERNO; break;
		case Land1::eMeshType::bordo_singolo_dx:	meshType = MESH_INDEX__BORDO_SINGOLO_DX; break;
		case Land1::eMeshType::bordo_singolo_su:	meshType = MESH_INDEX__BORDO_SINGOLO_SU; break;
		case Land1::eMeshType::bordo_strano:		meshType = MESH_INDEX__STRANO; break;
			break;
		}
	
	
		const u32 numInstances = mesh_instance_data[i].num_quad;
		const u32 first_instance_index = mesh_instance_data[i].start_index_in_SBO;

		const res::GPUShape *cur_shape_info;
		if (engine->get (shape_list[meshType], &cur_shape_info))
		{
			rctx.bindVtxIdxBuffer (cur_shape_info->vbHandle, 0, cur_shape_info->ibHandle, 0)
				.drawIndexed (cur_shape_info->numIndices, numInstances, cur_shape_info->indexStart, cur_shape_info->vtxStart, first_instance_index);
		}

	}
}


//***************************************
void Renderer::begin()
{
	assert (NULL != map);
	num_vtx = 0;
	num_vtxInfo = 0;
	num_quad = 0;

	const u8 N = (u8)Land1::eMeshType::_COUNT;
	for (u8 i=0; i<N; i++)
	{
		mesh_instance_data[i].num_quad = 0;
		mesh_instance_data[i].start_index_in_SBO = 0;
	}	
}

//***************************************
void Renderer::end()
{
}

//***************************************
void Renderer::add_exa (const gos::examap::Coord exa_coord)
{
	assert (NULL != map);

	u16 last_time_updated;
	if (!map->get_exa_last_time_updated (exa_coord, &last_time_updated))
		return;

	const u32 key = exa_coord.pack_coord_u32();
	ExaR *exar;
	if (exaR_map.find (key, &exar))
	{
		if (exar->exaSRC_last_time_updated == last_time_updated)
		{
			priv_do_add_exa (exar);
			return;
		}

		//devo refreshare il mio exaR perche' la mappa e' cambiata
		ExaR::free (localAllocator, exar);
		exaR_map.remove (key);
	}

	exar = map->calc_exaR (localAllocator, exa_coord);
	if (NULL != exar)
	{
		exaR_map.insertIfNotExists (key, exar);
		priv_do_add_exa (exar);
	}
}


//***************************************
void Renderer::priv_do_add_exa (const Land1::ExaR *exa)
{
	const u32 STARTING_VTX = num_vtx;
	for (u32 i = 0; i < exa->num_vtx_tot; i++)
	{
		priv_add_vtx (exa->vtxList[i]);
	}

	const u32 STARTING_VTXINFO = num_vtxInfo;
	for (u32 iVtx = 0; iVtx < exa->num_vtxInfo; iVtx++)
	{
		const ExaR::VtxInfo *vi = &exa->vtxInfoList[iVtx];

		priv_add_vtxInfo (vi->height, vi->material_index);

		//if (vi->height == 0)	continue;

		for (u32 iQuad=0; iQuad < vi->num_quad; iQuad++)
		{
			const u32 ii = 1 + 2*iQuad;
			u32 ii2 = ii+2;
			if (iQuad +1 == vi->num_quad)
				ii2 = 1;

			priv_add_quad ( vi->mesh_type[iQuad], STARTING_VTXINFO + iVtx, 
				STARTING_VTX + vi->idx_list[0],
				STARTING_VTX + vi->idx_list[ii], 
				STARTING_VTX + vi->idx_list[ii+1], 
				STARTING_VTX + vi->idx_list[ii2]);
		}

	}
}
