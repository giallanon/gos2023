#include "Land1.h"
#include "../PerlinNoise.hpp"

using namespace gos;



//***************************************
Land1::Land1()
{
	localAllocator = NULL;
	engine = NULL;
	gpu = NULL;
}

//***************************************
void Land1::priv_unsetup()
{
	if (NULL == engine)
		return;

	for (u32 i=0; i<exagenList.getNElem(); i++)
	{
		GOSDELETE(localAllocator, exagenList[i]);
	}

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
bool Land1::on__attach (const RPIPE::Context &ctx)
{
	//load pipe
	if (!ctx.engine->pipeline_createFromAsset ("land1_pipe", &handle_pipeline, res::eLoadMode::asap))
	{
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
			gos::logger::err ("Land1::setup() => can't create an instance of descriptorSet_2\n");
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

	priv_generate_terrain();
	return true;
}

//***************************************
void Land1::priv_generate_terrain()
{
	const vec3f CENTER(0,0,0);
	const u32 NUM_RINGS = 3;

	siv::PerlinNoise perlin {1234}; //gos::randomU32(u32MAX)};

	gos::HexMap hexmap;
	HexMap::Coord coord_center(0,0);
	HexMap::Coord coordList[64];
	hexmap.world__set_information (vec3f(0,0,0), HEX_RADIUS);

	u32 ct = 0;
	exagenList.setup (localAllocator, 128);

	exagenList[ct] = GOSNEW(localAllocator, ExaGenerator); exagenList[ct]->setup(localAllocator);
	exagenList[ct]->build (HEX_RADIUS, hexmap.hex_coord_to_world (coord_center));
	ct++;

	for (u32 ring=0; ring<NUM_RINGS; ring++)
	{
		const u32 radius = ring+1;
		u32 n = hexmap.coord_ring (coord_center, radius, coordList, 64);
		for (u32 i=0; i<n; i++)
		{
			exagenList[ct] = GOSNEW(localAllocator, ExaGenerator); exagenList[ct]->setup(localAllocator);
			exagenList[ct]->build (HEX_RADIUS, hexmap.hex_coord_to_world (coordList[i]));
			ct++;
		}
	}

	for (u32 nn=0; nn<exagenList.getNElem(); nn++)
	{
		ExaGenerator *exa = exagenList[nn];
		for (u32 i=0; i<exa->quadList.getNElem(); i++)
		{
			vec3f c = exa->quad_center(i);
			c /= (HEX_RADIUS);

			f32 h = perlin.octave2D_01( c.x, c.z, 2);
			//h*= 4.0f;
			if (h > 0.8)	h = 4.0f;
			else if (h > 0.5)	h = 2.0f;
			else h = 0;

			h=0;
			exa->quadList[i].height = h;
			exa->quadList[i].material_index = gos::randomU32(2);
		}
	}
}

//***************************************
void Land1::on__detach (const RPIPE::Context &ctx)
{
	priv_unsetup();
}

//***************************************
void Land1::priv_add_vtx (const gos::vec3f &v)
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
void Land1::priv_add_quad (u32 idx1, u32 idx2, u32 idx3, u32 idx4, f32 height, u32 material_index)
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
void Land1::add__exa (const ExaGenerator *exa)
{
	u32 starting_vtx = num_vtx;
	for (u32 i=0; i<exa->vtxList.getNElem(); i++)
		priv_add_vtx ( exa->vtxList(i) );

	for (u32 i=0; i<exa->quadList.getNElem(); i++)
		priv_add_quad ( starting_vtx + exa->quadList(i).vtx_idx0,
						starting_vtx + exa->quadList(i).vtx_idx1,
						starting_vtx + exa->quadList(i).vtx_idx2,
						starting_vtx + exa->quadList(i).vtx_idx3,
						exa->quadList(i).height,
						exa->quadList(i).material_index);
}

//***************************************
void Land1::priv_do_render (const RPIPE::Context &ctx, gpu::RenderCtx &rctx)
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
	if (engine->get (shape_list[0], &cur_shape_info))
	{
		u32 is_basetta = 1;
		rctx.bindVtxIdxBuffer (cur_shape_info->vbHandle, 0, cur_shape_info->ibHandle, 0)
			.pushConstant (0, &is_basetta, sizeof(is_basetta))
			.drawIndexed (cur_shape_info->numIndices, numInstances, cur_shape_info->indexStart, cur_shape_info->vtxStart, first_instance_index);
	}

	first_instance_index += numInstances;
}


//***************************************
void Land1::on__render (const RPIPE::Context &ctx, gpu::RenderCtx &rctx)
{
	num_vtx = 0;
	num_quad = 0;
	for (u32 nn=0; nn<exagenList.getNElem(); nn++)
	{
		add__exa (exagenList(nn));
	}	

	priv_do_render (ctx, rctx);
}
