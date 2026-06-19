#include "Land1.h"

using namespace gos;



//***************************************
Land1::Land1()
{
	localAllocator = NULL;
	engine = NULL;
	gpu = NULL;
}

//***************************************
void Land1::unsetup()
{
	if (NULL == engine)
		return;

	common.unsetup();

	gpu->buffer_unmap (exaVtxList.mapped_buffer);
	gpu->buffer_unmap (packedInstanceData.mapped_buffer);

    gpu->deleteResource(handle_ubo_scene);
    gpu->deleteResource(handle_descrSet1);
    gpu->deleteResource(handle_descrSet2);
	gpu->deleteResource(exaVtxList.handle_sbo);
	gpu->deleteResource(packedInstanceData.handle_sbo);

	engine->release(handle__model_tile1);


	localAllocator = NULL;
	engine = NULL;
	gpu = NULL;
}

//***************************************
bool Land1::setup (gos::Allocator *allocatorIN, gos::Engine *engineIN)
{
	//load pipe
	if (!common.setup(allocatorIN, engineIN, "land1_pipe"))
		return false;

	localAllocator = allocatorIN;
	engine = engineIN;
	gpu = engine->gpu;


    //UBO "scene"
    gpu->uniformBuffer_create (sizeof(SceneData), eMemAccessMode::shared_cpuW_autoSync, &handle_ubo_scene);

    //SBO hexaVtxList
	{
    	exaVtxList.sizeof_buffer = NUM_MAX_EXA * HEXA__NUM_VTX * sizeof(vec4f);
	    gpu->storageBuffer_create (exaVtxList.sizeof_buffer, eMemAccessMode::shared_cpuW_manualSync, &exaVtxList.handle_sbo);
    	gpu->map (exaVtxList.handle_sbo, 0, u32MAX, &exaVtxList.mapped_buffer);
	}

    //SBO instance data
	{
    	packedInstanceData.sizeof_buffer = NUM_MAX_EXA * HEXA__AVG_NUM_QUAD * sizeof(u64);
    	gpu->storageBuffer_create (packedInstanceData.sizeof_buffer, eMemAccessMode::shared_cpuW_manualSync, &packedInstanceData.handle_sbo);
    	gpu->map (packedInstanceData.handle_sbo, 0, u32MAX, &packedInstanceData.mapped_buffer);
	}


	//mi serve che la pipe sia loaded
    const res::Pipeline *res_pipeline;
    if (engine->get (common.handle_pipeline, &res_pipeline, 5000))
    {
		//creo i descriptor set
		gos::gpu::DescrSetInstanceWriter dsw;

		//descriptor set 1
		if (!gpu->descrSetInstance_create (common.handle_descrPool, res_pipeline->pipeHandle, 1, &handle_descrSet1))
		{
			gos::logger::err ("Land1::setup() => can't create an instance of descriptorSet_1\n");
			return false;
		}
		else
		{
			dsw.begin (gpu, handle_descrSet1)
				.bindUniformBuffer (0, handle_ubo_scene, 0)
				.end();
		}


		//descriptor set 2        
		if (!gpu->descrSetInstance_create (common.handle_descrPool, res_pipeline->pipeHandle, 2, &handle_descrSet2))
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




	const f32 RADIUS = 5.0f;

	exagen.setup (localAllocator);
	exagen.build (RADIUS, vec3f(0,0,0));

	exagen2.setup (localAllocator);
	exagen2.build (RADIUS, vec3f(RADIUS*2,0,0));


	return true;	
}

//***************************************
void Land1::begin (gos::geom::Camera3 *cam)
{
	num_vtx = 0;
	num_quad = 0;

    //aggiorno UBO descrittore scena
	scene.matVP = cam->getMatVP();
	scene.lightDir = vec4f (cam->pos.getAsseZ(), 0);
	scene.lightDir.set (-0.3f, -1.0f, 0.3f, 0);
	scene.lightDir.normalize();
	gpu->writeAndSync (handle_ubo_scene, 0, &scene, sizeof(scene));
}

//***************************************
void Land1::end (gos::gpu::CmdBufferWriter2 &cw)
{
	cw  .imageTransition (common.handle_rt0, eImageLayout::undefined, eImageLayout::color_attachment_optimal)
		.imageTransition (common.handle_zbuffer, eImageLayout::undefined, eImageLayout::depth_attachment_optimal);

    gpu::RenderCtx rctx;
    cw  .renderCtx_define_begin(&rctx)
            .withRenderArea (common.handle_rt0)
            .withRT (common.handle_rt0, eAttachmentLoadOp::clear, eAttachmentStoreOp::store, gos::ColorHDR(0, 0.0f, 0.1f))
            .withZB (common.handle_zbuffer, eAttachmentLoadOp::clear, eAttachmentStoreOp::store)
        .define_end();

    priv_do_render (rctx);
    rctx.end_render_ctx();
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
void Land1::priv_add_quad (u16 idx1, u16 idx2, u16 idx3, u16 idx4)
{
	if (num_quad >= NUM_MAX_EXA * HEXA__AVG_NUM_QUAD)
	{
		DBGBREAK;
		return;
	}

	//idealmente:  idx1 | idx2 | idx3 | idx4  ma nello shader u32 LSB e u32 MSB sono inveriti
	//quindi:  idx3 | idx4 | idx1 | idx2
	u64 packed = ((u64)idx3) << 48;
	packed |= ((u64)idx4) << 32;
	packed |= ((u64)idx1) << 16;
	packed |= ((u64)idx2);


	u64 *p = reinterpret_cast<u64*>( packedInstanceData.mapped_buffer.host_pt );
	p[num_quad++] = packed;
}

//***************************************
void Land1::add__test1()
{
	priv_add_vtx ( vec3f(0.0f, 0, 5.0f) );
	priv_add_vtx ( vec3f(4.0f, 0, 5.0f) );
	priv_add_vtx ( vec3f(4.0f, 0, 0.0f) );
	priv_add_vtx ( vec3f(0.0f, 0, 0.0f) );

	priv_add_quad (0,1,2,3);

	priv_add_vtx ( vec3f(4.2f, 0, 5.0f) );
	priv_add_vtx ( vec3f(8.2f, 0, 5.0f) );
	priv_add_vtx ( vec3f(8.2f, 0, 0.0f) );
	priv_add_vtx ( vec3f(4.2f, 0, 0.0f) );

	priv_add_quad (4,5,6,7);
}

//***************************************
void Land1::add__exa (ExaGenerator &exa)
{
	u32 starting_vtx = num_vtx;
	for (u32 i=0; i<exa.vtxList.getNElem(); i++)
		priv_add_vtx ( exa.vtxList(i) );

	for (u32 i=0; i<exa.quadList.getNElem(); i++)
		priv_add_quad ( starting_vtx + exa.quadList(i).vtx_idx0,
						starting_vtx + exa.quadList(i).vtx_idx1,
						starting_vtx + exa.quadList(i).vtx_idx2,
						starting_vtx + exa.quadList(i).vtx_idx3);
}

//***************************************
void Land1::priv_do_render (gpu::RenderCtx &rctx)
{
    if (0 == num_quad)
        return;

    const res::Pipeline *res_pipeline;
    if (!engine->get (common.handle_pipeline, &res_pipeline))
    {
        return;
    }

    //aggiornamento exaVtx
    {
		u32 size = num_vtx * sizeof(vec4f);
		const u32 r = size % gpu->limits_get_nonCoherentAtomSize();
		if (r)
			size += gpu->limits_get_nonCoherentAtomSize() - r;
		
        gpu->buffer_manualSync_cpuWrite (exaVtxList.mapped_buffer, 0, size);
    }

    //aggiornamento quad
    {
		u32 size = num_quad * sizeof(u64);
		const u32 r = size % gpu->limits_get_nonCoherentAtomSize();
		if (r)
			size += gpu->limits_get_nonCoherentAtomSize() - r;

		gpu->buffer_manualSync_cpuWrite (packedInstanceData.mapped_buffer, 0, size);
    }    

    
    //command
    rctx.bindPipeline (res_pipeline->pipeHandle)
        .bindDescriptorSet (common.handle_descrSet0, 0)
        .bindDescriptorSet (handle_descrSet1, 1)
        .bindDescriptorSet (handle_descrSet2, 2);

    //render delle shape
	const u32 numInstances = num_quad;
	u32 first_instance_index = 0;

	const res::GPUShape *cur_shape_info;
	if (engine->get (shape_list[0], &cur_shape_info))
	{
		rctx.bindVtxIdxBuffer (cur_shape_info->vbHandle, 0, cur_shape_info->ibHandle, 0)
			.drawIndexed (cur_shape_info->numIndices, numInstances, cur_shape_info->indexStart, cur_shape_info->vtxStart, first_instance_index);
	}
	if (engine->get (shape_list[1], &cur_shape_info))
	{
		rctx.bindVtxIdxBuffer (cur_shape_info->vbHandle, 0, cur_shape_info->ibHandle, 0)
			.drawIndexed (cur_shape_info->numIndices, numInstances, cur_shape_info->indexStart, cur_shape_info->vtxStart, first_instance_index);
	}

	first_instance_index += numInstances;
}


//***************************************
void Land1::render (gos::gpu::SwapchainImg swapchainImg, GPUCmdBufferHandle cmdBufferHandle, gos::geom::Camera3 *cam)
{
	gos::gpu::CmdBufferWriter2 cw;
	cw	.begin (gpu, cmdBufferHandle)
		.setViewport (gpu->viewport_getDefault());

	begin (cam);
	//add__test1();
	add__exa (exagen);
	add__exa (exagen2);

	end(cw);	

	//present
	cw	.imageTransition (common.handle_rt0, eImageLayout::color_attachment_optimal, eImageLayout::transfer_src)
		.imageTransition (swapchainImg.image, eImageLayout::undefined, eImageLayout::transfer_dst)
		.copyImageToImage (common.handle_rt0, swapchainImg.image, gpu->swapChain_getImageExten2D(), gpu->swapChain_getImageExten2D())
		.imageTransition (swapchainImg.image, eImageLayout::transfer_dst, eImageLayout::presentation)
		.end();	
}