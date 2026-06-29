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
}

//***************************************
void Renderer::priv_unsetup()
{
	if (NULL == engine)
		return;

	engine->release(handle_pipeline);

	gpu->buffer_unmap (exaVtxList.mapped_buffer);
	gpu->buffer_unmap (packedInstanceData.mapped_buffer);

    gpu->deleteResource(handle_descrSet2);
	gpu->deleteResource(exaVtxList.handle_sbo);
	gpu->deleteResource(packedInstanceData.handle_sbo);

	engine->release(handle__model_tile1);


	localAllocator = NULL;
	engine = NULL;
	gpu = NULL;
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
    	exaVtxList.sizeof_buffer = NUM_MAX_EXA * HEXA__NUM_VTX * sizeof(vec4f);
	    gpu->storageBuffer_create (exaVtxList.sizeof_buffer, eMemAccessMode::shared_cpuW_manualSync, &exaVtxList.handle_sbo);
    	gpu->map (exaVtxList.handle_sbo, 0, u32MAX, &exaVtxList.mapped_buffer);
	}

    //SBO instance data
	{
    	packedInstanceData.sizeof_buffer = NUM_MAX_EXA * HEXA__AVG_NUM_QUAD * sizeof(sInstanceData);
    	gpu->storageBuffer_create (packedInstanceData.sizeof_buffer, eMemAccessMode::shared_cpuW_manualSync, &packedInstanceData.handle_sbo);
    	gpu->map (packedInstanceData.handle_sbo, 0, u32MAX, &packedInstanceData.mapped_buffer);
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
				.bindStorageBuffer (0, exaVtxList.handle_sbo)
				.bindStorageBuffer (1, packedInstanceData.handle_sbo)
				.end();
		}
	}
	
	
	
	//load risorse
	engine->model_createFromAsset ("model_tile1", &handle__model_tile1, res::eLoadMode::asap);
	
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
void Renderer::priv_add_vtx (const gos::vec3f &v)
{
	if (num_vtx >= NUM_MAX_EXA * HEXA__NUM_VTX)
	{
		DBGBREAK;
		return;
	}

	u32 ct = num_vtx*4;
	f32 *p = reinterpret_cast<f32*>( exaVtxList.mapped_buffer.host_pt );
	p[ct++] = v.x;
	p[ct++] = v.y;
	p[ct++] = v.z;
	p[ct] = 1.0f;

	num_vtx++;
}

//***************************************
void Renderer::priv_add_quad (u32 idx1, u32 idx2, u32 idx3, u32 idx4, f32 height, u32 material_index)
{
	if (num_quad >= NUM_MAX_EXA * HEXA__AVG_NUM_QUAD)
	{
		DBGBREAK;
		return;
	}

	assert (idx1 < u16MAX);
	assert (idx2 < u16MAX);
	assert (idx3 < u16MAX);
	assert (idx4 < u16MAX);

	sInstanceData *p = reinterpret_cast<sInstanceData*>( packedInstanceData.mapped_buffer.host_pt );
	p[num_quad].quad_indices_0_1 = (((u32)idx1) << 16) | (u32)idx2;
	p[num_quad].quad_indices_2_3 = (((u32)idx3) << 16) | (u32)idx4;
	p[num_quad].height = height;
	p[num_quad].material_index = material_index;
	num_quad++;
}

//***************************************
void Renderer::priv_do_render (const RPIPE::Context &ctx, gpu::RenderCtx &rctx)
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
		u32 size = num_vtx * sizeof(vec4f);
		const u32 r = size % gpu->limits_get_nonCoherentAtomSize();
		if (r)
			size += gpu->limits_get_nonCoherentAtomSize() - r;
		
		assert (size <= exaVtxList.sizeof_buffer);
        gpu->buffer_manualSync_cpuWrite (exaVtxList.mapped_buffer, 0, size);
    }

    //aggiornamento quad
    {
		u32 size = num_quad * sizeof(u64);
		const u32 r = size % gpu->limits_get_nonCoherentAtomSize();
		if (r)
			size += gpu->limits_get_nonCoherentAtomSize() - r;

		assert (size <= packedInstanceData.sizeof_buffer);
		gpu->buffer_manualSync_cpuWrite (packedInstanceData.mapped_buffer, 0, size);
    }    

    
    //command
    rctx.bindPipeline (res_pipeline->pipeHandle)
        .bindDescriptorSet (ctx.handle_descrSet0, 0)
        .bindDescriptorSet (ctx.handle_descrSet1, 1)
        .bindDescriptorSet (handle_descrSet2, 2);

    //render delle shape
	const u32 numInstances = num_quad;
	u32 first_instance_index = 0;

	const res::GPUShape *cur_shape_info;

	//prato
	if (engine->get (shape_list[1], &cur_shape_info))
	{
		u32 is_basetta = 0;
		rctx.bindVtxIdxBuffer (cur_shape_info->vbHandle, 0, cur_shape_info->ibHandle, 0)
			.pushConstant (0, &is_basetta, sizeof(is_basetta))
			.drawIndexed (cur_shape_info->numIndices, numInstances, cur_shape_info->indexStart, cur_shape_info->vtxStart, first_instance_index);
	}

	//basetta
	//if (engine->get (shape_list[0], &cur_shape_info))
	//{
	//	u32 is_basetta = 1;
	//	rctx.bindVtxIdxBuffer (cur_shape_info->vbHandle, 0, cur_shape_info->ibHandle, 0)
	//		.pushConstant (0, &is_basetta, sizeof(is_basetta))
	//		.drawIndexed (cur_shape_info->numIndices, numInstances, cur_shape_info->indexStart, cur_shape_info->vtxStart, first_instance_index);
	//}

	first_instance_index += numInstances;
}

//***************************************
void Renderer::on__render (const RPIPE::Context &ctx, gpu::RenderCtx &rctx)
{
	if (0 == num_quad)
		return;
	priv_do_render (ctx, rctx);
}


//***************************************
void Renderer::begin()						{ priv_begin2(); }
void Renderer::add_exa (const Exa *exa)		{ priv_add_exa2 (exa); }
void Renderer::end()						{ priv_end2(); }




//***************************************
void Renderer::priv_begin1()
{
	num_vtx = 0;
	num_quad = 0;
}

//***************************************
void Renderer::priv_add_exa1 (const Land1::Exa *exa)
{
	u32 starting_vtx = num_vtx;
	for (u32 i = 0; i < exa->num_vtx; i++)
	{
		priv_add_vtx (vec3f(exa->vtxList[i].x, 0, exa->vtxList[i].y) );
	}

	for (u32 i=0; i<exa->num_quad; i++)
		priv_add_quad ( starting_vtx + exa->quadList[i].idx[0],
						starting_vtx + exa->quadList[i].idx[1],
						starting_vtx + exa->quadList[i].idx[2],
						starting_vtx + exa->quadList[i].idx[3],
						exa->quadList[i].height,
						exa->quadList[i].material_index);
}

//***************************************
void Renderer::priv_end1()
{
}




//***************************************
void Renderer::priv_begin2()
{
	num_vtx = 0;
	num_quad = 0;
}

//***************************************
void Renderer::priv_add_exa2 (const Land1::Exa *exa)
{
	u32 starting_vtx = num_vtx;
	for (u32 i = 0; i < exa->num_quad; i++)
	{
		priv_add_vtx (vec3f(exa->quadCenterList[i].x, 0, exa->quadCenterList[i].y) );
	}

	for (u32 i = 0; i < exa->num_vtx; i++)
	{
		if (0 == exa->vtxInfoList[i].material_index)
			continue;

		//recupero i quad che sharano il vtx i-esimo
		u32 quads[8];
		const u32 nquad = exa->get_quad_from_vtx (i, quads, 8);
		switch (nquad)
		{
		default:
			break;

		case 3:
			//aggiungo un quad composto dai quad-center dei 4 quad trovati
			priv_add_quad ( starting_vtx + quads[0], starting_vtx + quads[1], starting_vtx + quads[2], starting_vtx + quads[0], 0, exa->vtxInfoList[i].material_index);
			break;

		case 4:
			//aggiungo un quad composto dai quad-center dei 4 quad trovati
			priv_add_quad ( starting_vtx + quads[0], starting_vtx + quads[1], starting_vtx + quads[2], starting_vtx + quads[3], 0, exa->vtxInfoList[i].material_index);
			break;

		case 5:
			//aggiungo un quad composto dai quad-center dei 4 quad trovati
			priv_add_quad ( starting_vtx + quads[0], starting_vtx + quads[1], starting_vtx + quads[2], starting_vtx + quads[3], 0, exa->vtxInfoList[i].material_index);
			priv_add_quad ( starting_vtx + quads[3], starting_vtx + quads[4], starting_vtx + quads[0], starting_vtx + quads[3], 0, exa->vtxInfoList[i].material_index);
			break;

		case 6:
			//aggiungo un quad composto dai quad-center dei 4 quad trovati
			priv_add_quad ( starting_vtx + quads[0], starting_vtx + quads[1], starting_vtx + quads[2], starting_vtx + quads[3], 0, exa->vtxInfoList[i].material_index);
			priv_add_quad ( starting_vtx + quads[3], starting_vtx + quads[4], starting_vtx + quads[5], starting_vtx + quads[0], 0, exa->vtxInfoList[i].material_index);
			break;

		}

	}
}

//***************************************
void Renderer::priv_end2()
{
}


