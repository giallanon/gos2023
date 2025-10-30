#include "test1.h"
#include "gosShapePrefabs.h"


using namespace gos;


//***************************************
Test1::Test1()
{
	allocator = gos::getSysHeapAllocator();

	entRegistry.setup();
	entRegistry.addComponentHandler<ent::CompPos>();
	entRegistry.addComponentHandler<ent::CompModelInstance>();

	entList.setup (gos::getSysHeapAllocator(), 32);

	skeleton = NULL;
	model = NULL;
	renderer = NULL;

	obj0_roty = 0;
}


//***************************************
Test1::~Test1()
{
	//free delle modelInstance di ogni entity
	entList.forEach ( [lambdaAllocator = this->allocator, lambdaEntReg = &entRegistry](u32 index, gos::Entity ent) 
	{
		ent::CompModelInstance *comp = lambdaEntReg->get<ent::CompModelInstance>(ent);
		if (NULL != comp)
		{
			GOSDELETE(lambdaAllocator, comp->modelInstance);
		}
		return true;
	});



	entList.unsetup();
	entRegistry.unsetup();

	GOSDELETE(allocator, model);
	GOSDELETE(allocator, skeleton);
	GOSDELETE(allocator, renderer);
}

//***************************************
bool Test1::priv_shape_create (gos::Engine *engine, gos::ENGShape *out)
{
	//creo una shape
	gos::Shape shape;
	shape.reset();
	{
		gos::VtxLayout vtxLayout;
		{
			shape::VtxLayoutWriter vtxLayoutW;

			vtxLayoutW.setup (&vtxLayout);
			vtxLayoutW.begin()
				.addPos3(0)
				.addNorm3(12)
				.addTexCoord(24)
				.end();
		}

		if (!shape::buildCube24 (vec3f(0, 0, 0), vec3f(1, 1, 1), vtxLayout, allocator, &shape))
			return false;
	}

	//creo una engine::shape
	gos::ENGShape shapeHandle;
	if (!engine->shape_create (&shape, &shapeHandle))
		return false;

	//ora devo copiare vtx/idx nel VB/IB
	{
		//copio idx/vtx nello stage buffer
		GPUStgBufferHandle stgBufferHandle;
		engine->gpu->stagingBuffer_create (1024, &stgBufferHandle);
		
		const u32 SIZE_OF_IDX = shape.numIdx * sizeof(u16);
		engine->gpu->stagingBuffer_memcpy (stgBufferHandle, 0, shape.idxBuffer, SIZE_OF_IDX);

		const u32 SIZE_OF_VTX = shape::calcSizeOfAVertex(shape.vtxLayout) * shape.numVtx;
		engine->gpu->stagingBuffer_memcpy (stgBufferHandle, SIZE_OF_IDX, shape.vtxBuffer, SIZE_OF_VTX);

		//free della shape
		shape::shapeFree (allocator, &shape);


		//creo un job per pushare lo stage buffer in VB/IB
		GPUCmdBufferHandle cmdBufferHandle;
		if (!engine->gpu->cmdBuffer_create (eGPUQueueFamily::transfer, &cmdBufferHandle))
			return false;
		const engine::Shape *shapeInfo = engine->shape_getInfo (shapeHandle);

		gos::gpu::pipe2::CmdBufferWriter2 cw;
		cw.begin (engine->gpu, cmdBufferHandle)
			.copyBuffer (stgBufferHandle, shapeInfo->ibHandle, 0, 0, SIZE_OF_IDX)
			.copyBuffer (stgBufferHandle, shapeInfo->vbHandle, SIZE_OF_IDX, 0, SIZE_OF_VTX)
			.end();


		gpu::TransferJob job;
		job.setup (engine->gpu);
		job.submit(cmdBufferHandle);

		while (!job.hasFinished())
		{
		}

		engine->gpu->deleteResource(cmdBufferHandle);
		engine->gpu->deleteResource(stgBufferHandle);

	}

	*out = shapeHandle;
	return true;
}

//***************************************
void Test1::priv_model_setup(gos::ENGShape shapeHandle)
{
	skeleton = GOSNEW(allocator, gos::Skeleton)();
	{
		gos::SkeletonBuilder builder;

		gos::Bone *bone;
		const u32 iRoot = builder.begin ("root");
		builder.addChildTo (iRoot, "left-arm", &bone);
			bone->matrix.buildTranslation(vec3f(-4,0,0));
		builder.addChildTo (iRoot, "right-arm", &bone);
			bone->matrix.buildTranslation(vec3f( 4,0,0));
		builder.end (gos::getSysHeapAllocator(), skeleton);
	}

	model = GOSNEW(allocator, model::Model)();
	{
		model->setSkeleton(skeleton);
		model->addShape (shapeHandle);
		model->linkShapeToBone (shapeHandle, "root");
		model->linkShapeToBone (shapeHandle, "left-arm");
		model->linkShapeToBone (shapeHandle, "right-arm");
	}
 }

//***************************************
void Test1::run (gos::Engine *engineIN)
{
	engine = engineIN;
	gpu = engine->gpu;

	//setup camera
    cam.setPerspectiveFovLH(gpu->swapChain_calcAspectRatio(),  math::gradToRad(45), 0.1f, 50.0f);
    cam.pos.identity();
    cam.pos.warp (0, 0, -10);
    cam.pos.lookAt (vec3f(0,0,0));
    cam.markUpdated();

	//e movement
    movement.bind (&cam.pos);

	//priv_run1();
	//priv_run2();
	//priv_run3();
	priv_run4();
}

//**********************************
void Test1::doCPUStuff ()
{
	Engine::InputEvent ev;
	while (engine->inputEvent_getNext(&ev))
	{
		switch (ev.actionID)
		{
		case COMPILE_TIME_STR_CRC32("move_forward"):           movement.moveForward ((ev.value == 1)); break;
		case COMPILE_TIME_STR_CRC32("move_backward"):          movement.moveBackward ((ev.value == 1));    break;
		case COMPILE_TIME_STR_CRC32("strafe_left"):            movement.strafeLeft ((ev.value == 1));    break;
		case COMPILE_TIME_STR_CRC32("strafe_right"):           movement.strafeRight ((ev.value == 1));    break;
		case COMPILE_TIME_STR_CRC32("strafe_up"):              movement.strafeUp ((ev.value == 1));    break;
		case COMPILE_TIME_STR_CRC32("strafe_down"):            movement.strafeDown ((ev.value == 1));    break;
		case COMPILE_TIME_STR_CRC32("rotateY"):                movement.rotateY ((ev.value < 0)); break;
		case COMPILE_TIME_STR_CRC32("rotateX"):                movement.rotateX ((ev.value < 0)); break;
		}
	}

    //gestione del movimento
    const u64 timeNow_msec = gos::getTimeSinceStart_msec();
    movement.update(timeNow_msec);
    cam.markUpdated();
}

//***************************************
void Test1::priv_run1 ()
{
	
	gos::ENGShape shapeHandle;
	if (!priv_shape_create (engine, &shapeHandle))
	{
		DBGBREAK;
		return;
	}


	//model 	
	priv_model_setup(shapeHandle);

	//3 ent, con posizione e modelInstance
	for (u32 i=0; i<3; i++)
	{
		Entity ent = entRegistry.newEntity();
		{
			auto pos = entRegistry.addComponent<ent::CompPos>(ent);
			pos->matrix.buildTranslation ( vec3f(0, 0, (f32)(10 * i)) );

			auto mi = entRegistry.addComponent<ent::CompModelInstance>(ent);
			mi->modelInstance = GOSNEW(allocator, model::ModelInstance)(model);
			mi->modelInstance->applyTransform (pos->matrix);
		}

		entList.append(ent);
	}


	//creo il renderer
	renderer = GOSNEW(allocator, gos::engine::Renderer1)();
	renderer->setup (allocator, engine);


	//loop
    gpu::MainLoop2 mainLoop;
    mainLoop.setup (engine->gpu);


    //command buffer 
    GPUCmdBufferHandle  cmdBufferHandle;
    engine->gpu->cmdBuffer_create (eGPUQueueFamily::gfx, &cmdBufferHandle);


	bool bQuit = false;
	while (false == bQuit)
	{
		if (!engine->update())
			bQuit = true;

//		gos::gpu::pipe2::CmdBufferWriter2 cw;
		
	}

	engine->gpu->deleteResource (cmdBufferHandle);
	engine->shape_release(shapeHandle);
}

//***************************************
bool Test1::priv_run2 ()
{
	gos::ENGShape shapeHandle;
	if (!priv_shape_create (engine, &shapeHandle))
	{
		DBGBREAK;
		return false;
	}
	const engine::Shape *info_shape = engine->shape_getInfo(shapeHandle);


	mat4x4f	matrixList[4];
	matrixList[0].identity();
	matrixList[1].buildTranslation (0, 0, 10);
	matrixList[2].buildTranslation (0, 3, 0);
	matrixList[3].buildTranslation (0, -3, 0);



    //load degli assets
	asset::Handle assHandle_pipe;
    if (!engine->assetHub->getHandle ("pipe1", &assHandle_pipe, true))
        return false;


    //risorse di rendering
    GPUZBufferHandle            handle_zbuffer;
    GPURenderTargetHandle       handle_rt0;
    GPUDescrPoolHandle          handle_descrPool;
    {
        //rt0
        if (!gpu->renderTarget_create ("0-", "0-", eImageFormat::U8_RGBA, &handle_rt0))
            return false;

        //zbuffer
        if (!gpu->zbuffer_create ("0-", "0-", eImageFormat::_DEPTH_BEST, &handle_zbuffer))
        {
            gos::logger::err ("Renderer::setup() => GPU::zbuffer_create\n");
            return false;
        }

        //creo un descriptor pool
        gpu->descrPool_createNew (&handle_descrPool)
            .setMaxNumDescriptorSet(4)
            .addPool_uniformBuffer(1)
            .addPool_storageBuffer(1)
            .addPool_sampler(2)
            .addPool_texture(1)
            .end();
        if (handle_descrPool.isInvalid())
        {
            gos::logger::err ("Renderer::setup() => can't create descriptor pool\n");
            return false;
        }
    }

    //attendo che la pipe sia stata caricata perche' mi servono le definizioni dei descrittori
	GPUDescrSetInstanceHandle   handle_descrSet0;
	GPUUniformBufferHandle      handle_ubo_scene;
	struct sLAYOUT_SCENE_DATA
	{
		mat4x4f	matVP;
		vec4f	lightDir;
	} scene;

    const asset::Asset_pipe *pipe;
    engine->assetHub->getAssetWithTimeout (assHandle_pipe, 5000, &pipe);
	{
        //alloco una istanza dei descriptor-set
        gos::gpu::DescrSetInstanceWriter dsw;

        //descriptor set 0
        if (!gpu->descrSetInstance_create (handle_descrPool, pipe->handle_pipe, 0, &handle_descrSet0))
        {
            gos::logger::err ("Renderer::setup() => can't create an instance of descriptorSet_0\n");
            return false;
        }
        gpu->uniformBuffer_create (sizeof(sLAYOUT_SCENE_DATA) * 16, eMemAccessMode::shared_cpuW_autoSync, &handle_ubo_scene);
        dsw.begin (gpu, handle_descrSet0)
            .bindUniformBuffer (0, handle_ubo_scene, 0)
            .end();
	}


	struct sPushConstantData
	{
		mat4x4f matW;
	} pushConstData;

	//renderizzo
    gpu::MainLoop2 mainLoop;
    mainLoop.setup (gpu);

    GPUCmdBufferHandle  cmdBufferHandle;
    gpu->cmdBuffer_create (eGPUQueueFamily::gfx, &cmdBufferHandle);
	
	bool bQuit = false;
	while (false == bQuit)
	{
		if (!engine->update())
		{
			bQuit = true;
			continue;
		}

        mainLoop.stat_onCPUFrameBegin();
        engine->assetHub->update (gos::getTimeSinceStart_msec());
		doCPUStuff();
		mainLoop.stat_onCPUFrameEnd();

        mainLoop.run();

        if (gpu->swapChain_wasRecreated())
            cam.changeAspectRatioPerspectiveFovLH (gpu->swapChain_calcAspectRatio());


        //se il job precedente e' stato presentato, posso schedularne uno nuovo
        gpu::SwapchainImg swapchainImg;
        if (mainLoop.gfxJob_canSubmit(&swapchainImg))
        {
			const asset::Asset_pipe *pipe;
			if (engine->assetHub->getAsset(assHandle_pipe, &pipe))
			{
				//aggiorno UBO
				scene.matVP = cam.getMatVP();
				scene.lightDir = vec4f (cam.pos.getAsseZ(), 0);
				scene.lightDir.normalize();
				gpu->writeAndSync (handle_ubo_scene, 0, &scene, sizeof(scene));

				gos::gpu::pipe2::CmdBufferWriter2 cw;
				cw
					.begin (gpu, cmdBufferHandle)
					.setViewport (gpu->viewport_getDefault())
					.imageTransition (handle_rt0, eImageLayout::undefined, eImageLayout::color_attachment_optimal)
					.imageTransition (handle_zbuffer, eImageLayout::undefined, eImageLayout::depth_attachment_optimal);

				auto &r = cw.beginRender();
					   r.withRenderArea (handle_rt0)
						.withRT (handle_rt0, eAttachmentLoadOp::clear, eAttachmentStoreOp::dont_care, gos::ColorHDR(0, 0.0f, 0.1f))
						.withZB (handle_zbuffer, eAttachmentLoadOp::clear, eAttachmentStoreOp::dont_care)
						.bindPipeline (pipe->handle_pipe)
						.bindDescriptorSet (handle_descrSet0, 0)
						.bindVtxBuffer(info_shape->vbHandle)
						.bindIdxBufferU16(info_shape->ibHandle);
						for (u32 i = 0; i < 4; i++)
						{
							r.pushConstant(0, &matrixList[i], sizeof(sPushConstantData));
							r.drawIndexed (info_shape->numIndices, 1, info_shape->indexStart, info_shape->vtxStart, 0);
						}
					r.endRender();

					cw.imageTransition (handle_rt0, eImageLayout::color_attachment_optimal, eImageLayout::transfer_src)
					.imageTransition (swapchainImg.image, eImageLayout::undefined, eImageLayout::transfer_dst)
					.copyImageToImage (handle_rt0, swapchainImg.image, gpu->swapChain_getImageExten2D(), gpu->swapChain_getImageExten2D())
					.imageTransition (swapchainImg.image, eImageLayout::transfer_dst, eImageLayout::presentation)
					.end();

				mainLoop.gfxJob_submitAndPresent (cmdBufferHandle, swapchainImg);
			}
        }		
	}
	
	gpu->waitIdle();
	mainLoop.unsetup();
	
	engine->shape_release(shapeHandle);
	gpu->deleteResource (cmdBufferHandle);
	return true;
}

//***************************************
bool Test1::priv_run3 ()
{
	gos::ENGShape shapeHandle;
	if (!priv_shape_create (engine, &shapeHandle))
	{
		DBGBREAK;
		return false;
	}
	const engine::Shape *info_shape = engine->shape_getInfo(shapeHandle);

    //load degli assets
	asset::Handle assHandle_pipe;
	asset::Handle assHandle_texBianca;
	asset::Handle assHandle_texChecker;
    if (!engine->assetHub->getHandle ("pipe2", &assHandle_pipe, true))
        return false;
    if (!engine->assetHub->getHandle ("tex_bianca", &assHandle_texBianca, true))
        return false;
    if (!engine->assetHub->getHandle ("tex_checker", &assHandle_texChecker, true))
        return false;


    //risorse di rendering
    GPUZBufferHandle            handle_zbuffer;
    GPURenderTargetHandle       handle_rt0;
    GPUDescrPoolHandle          handle_descrPool;
    {
        //rt0
        if (!gpu->renderTarget_create ("0-", "0-", eImageFormat::U8_RGBA, &handle_rt0))
            return false;

        //zbuffer
        if (!gpu->zbuffer_create ("0-", "0-", eImageFormat::_DEPTH_BEST, &handle_zbuffer))
        {
            gos::logger::err ("Renderer::setup() => GPU::zbuffer_create\n");
            return false;
        }

        //creo un descriptor pool
        gpu->descrPool_createNew (&handle_descrPool)
            .setMaxNumDescriptorSet(4)
            .addPool_uniformBuffer(1)
            .addPool_storageBuffer(2)
            .addPool_sampler(2)
            .addPool_texture(1024)
            .end();
        if (handle_descrPool.isInvalid())
        {
            gos::logger::err ("Renderer::setup() => can't create descriptor pool\n");
            return false;
        }
    }

    //attendo che la pipe sia stata caricata perche' mi servono le definizioni dei descrittori
	GPUDescrSetInstanceHandle   handle_descrSet0;
	GPUDescrSetInstanceHandle   handle_descrSet1;
	GPUDescrSetInstanceHandle   handle_descrSet2;
	GPUSamplerHandle			handle_samplers[2];
	GPUUniformBufferHandle      handle_ubo_scene;
	GPUStorageBufferHandle      handle_sbo_matrixList;
	GPUStorageBufferHandle      handle_sbo_materiaList;
	
	struct sLAYOUT_SCENE_DATA
	{
		mat4x4f	matVP;
		vec4f	lightDir;
	} scene;

	struct sMaterial
	{
		vec3f	diffuse_col;
		u32		texture_index;
	};	

    const asset::Asset_pipe *pipe;
    engine->assetHub->getAssetWithTimeout (assHandle_pipe, 5000, &pipe);
	{
		const asset::Asset_tex2D *texBianca;
		const asset::Asset_tex2D *texChecker;
		engine->assetHub->getAssetWithTimeout (assHandle_texBianca, 5000, &texBianca);
		engine->assetHub->getAssetWithTimeout (assHandle_texChecker, 5000, &texChecker);



        //alloco una istanza dei descriptor-set
        gos::gpu::DescrSetInstanceWriter dsw;

        //descriptor set 0
        if (!gpu->descrSetInstance_create (handle_descrPool, pipe->handle_pipe, 0, &handle_descrSet0))
        {
            gos::logger::err ("Renderer::setup() => can't create an instance of descriptorSet_0\n");
            return false;
        }
		else
		{
			//2 samplers
			gpu::SamplerDesc desc;

			//sampler2d: bilinear filtering
			desc.reset();
			gpu->sampler_create (desc, &handle_samplers[0]);

			//sampler2d: point filtering
			desc.reset();
			desc.minFilter = desc.magFilter = eSamplerFilter::point;
			desc.mipFilter = eSamplerMipFilter::nearest;
			desc.bAnisotropic = false;
			gpu->sampler_create (desc, &handle_samplers[1]);		



			dsw.begin (gpu, handle_descrSet0)
				.bindSamplerInArray (0, handle_samplers[0], 0)
				.bindSamplerInArray (0, handle_samplers[1], 1)
				.bindTextureInArray (1, texBianca->handle_texture, 0)
				.bindTextureInArray (1, texChecker->handle_texture, 1)
				.bindTextureInArray (1, texBianca->handle_texture, 2)
				.bindTextureInArray (1, texChecker->handle_texture, 3)
				.end();
		}


        //descriptor set 1
        if (!gpu->descrSetInstance_create (handle_descrPool, pipe->handle_pipe, 1, &handle_descrSet1))
        {
            gos::logger::err ("Renderer::setup() => can't create an instance of descriptorSet_1\n");
            return false;
        }
		else
		{
			//UBO scene
			gpu->uniformBuffer_create (sizeof(sLAYOUT_SCENE_DATA), eMemAccessMode::shared_cpuW_autoSync, &handle_ubo_scene);
       
			dsw.begin (gpu, handle_descrSet1)
				.bindUniformBuffer (0, handle_ubo_scene, 0)
				.end();
		}

        //descriptor set 2
        if (!gpu->descrSetInstance_create (handle_descrPool, pipe->handle_pipe, 2, &handle_descrSet2))
        {
            gos::logger::err ("Renderer::setup() => can't create an instance of descriptorSet_2\n");
            return false;
        }
		else
		{
			//SBO matrici
        	gpu->storageBuffer_create (sizeof(mat4x4f) * 16, eMemAccessMode::shared_cpuW_autoSync, &handle_sbo_matrixList);

			//SBO materialList
        	gpu->storageBuffer_create (sizeof(sMaterial) * 16, eMemAccessMode::shared_cpuW_autoSync, &handle_sbo_materiaList);

			dsw.begin (gpu, handle_descrSet2)
				.bindStorageBuffer (0, handle_sbo_matrixList, 0)
				.bindStorageBuffer (1, handle_sbo_materiaList, 0)
				.end();

			//fillo SBO con 4 matrixi
			mat4x4f	matrixList[4];
			matrixList[0].identity();
			matrixList[1].buildTranslation (0, 0, 10);
			matrixList[2].buildTranslation (0, 3, 0);
			matrixList[3].buildTranslation (0, -3, 0);
			gpu->writeAndSync (handle_sbo_matrixList, 0, matrixList, sizeof(matrixList));

			//fillo SBO con 4 materiali
			sMaterial materialList[4];
			materialList[0].diffuse_col.set (1,0,0);	materialList[0].texture_index = 0;
			materialList[1].diffuse_col.set (0,1,0);	materialList[1].texture_index = 1;
			materialList[2].diffuse_col.set (0,0,1);	materialList[2].texture_index = 0;
			materialList[3].diffuse_col.set (1,1,1);	materialList[3].texture_index = 1;
			gpu->writeAndSync (handle_sbo_materiaList, 0, materialList, sizeof(materialList));
		}		
	}

	/*struct sPushConstantData
	{
		u32 matrixIndex;
		u32 materialIndex;
	} pushConstData;*/





	//renderizzo
    gpu::MainLoop2 mainLoop;
    mainLoop.setup (gpu);

    GPUCmdBufferHandle  cmdBufferHandle;
    gpu->cmdBuffer_create (eGPUQueueFamily::gfx, &cmdBufferHandle);
	
	u64 nextTimeUpdate_msec = 0;

	bool bQuit = false;
	while (false == bQuit)
	{
		if (!engine->update())
		{
			bQuit = true;
			continue;
		}

        mainLoop.stat_onCPUFrameBegin();
        engine->assetHub->update (gos::getTimeSinceStart_msec());
		doCPUStuff();
		{
			if (gos::getTimeSinceStart_msec() > nextTimeUpdate_msec)
			{
				nextTimeUpdate_msec = gos::getTimeSinceStart_msec() + 30;
				obj0_roty += 1.0f;
				if (obj0_roty > 360)
					obj0_roty -= 360;

				mat4x4f mat1;
				mat4x4f mat2;
				mat1.buildRotationAboutY (gos::math::gradToRad(obj0_roty));
				gpu->writeAndSync (handle_sbo_matrixList, 0, &mat1, sizeof(mat4x4f));


				mat1.buildTranslation (0, 3, 0);
				mat2.buildRotationAboutZ (gos::math::gradToRad(obj0_roty));
				mat2 = mat1 * mat2;
				gpu->writeAndSync (handle_sbo_matrixList, sizeof(mat4x4f)*2, &mat2, sizeof(mat4x4f));
			}			
		}
		mainLoop.stat_onCPUFrameEnd();

        mainLoop.run();

        if (gpu->swapChain_wasRecreated())
            cam.changeAspectRatioPerspectiveFovLH (gpu->swapChain_calcAspectRatio());


        //se il job precedente e' stato presentato, posso schedularne uno nuovo
        gpu::SwapchainImg swapchainImg;
        if (mainLoop.gfxJob_canSubmit(&swapchainImg))
        {
			const asset::Asset_pipe *pipe;
			if (engine->assetHub->getAsset(assHandle_pipe, &pipe))
			{
				//aggiorno UBO scene
				scene.matVP = cam.getMatVP();
				scene.lightDir = vec4f (cam.pos.getAsseZ(), 0);
				scene.lightDir.normalize();
				gpu->writeAndSync (handle_ubo_scene, 0, &scene, sizeof(scene));




				gos::gpu::pipe2::CmdBufferWriter2 cw;
				cw
					.begin (gpu, cmdBufferHandle)
					.setViewport (gpu->viewport_getDefault())
					.imageTransition (handle_rt0, eImageLayout::undefined, eImageLayout::color_attachment_optimal)
					.imageTransition (handle_zbuffer, eImageLayout::undefined, eImageLayout::depth_attachment_optimal);

				auto &r = cw.beginRender();
					   r.withRenderArea (handle_rt0)
						.withRT (handle_rt0, eAttachmentLoadOp::clear, eAttachmentStoreOp::dont_care, gos::ColorHDR(0, 0.0f, 0.1f))
						.withZB (handle_zbuffer, eAttachmentLoadOp::clear, eAttachmentStoreOp::dont_care)
						.bindPipeline (pipe->handle_pipe)
						.bindDescriptorSet (handle_descrSet0, 0)
						.bindDescriptorSet (handle_descrSet1, 1)
						.bindDescriptorSet (handle_descrSet2, 2)
						.bindVtxBuffer(info_shape->vbHandle)
						.bindIdxBufferU16(info_shape->ibHandle);
						for (u32 i = 0; i < 4; i++)
						{
							r.pushConstant(0, &i, sizeof(u32));	//matrix index
							r.pushConstant(1, &i, sizeof(u32));	//material index
							r.drawIndexed (info_shape->numIndices, 1, info_shape->indexStart, info_shape->vtxStart, 0);
						}
					r.endRender();

					cw.imageTransition (handle_rt0, eImageLayout::color_attachment_optimal, eImageLayout::transfer_src)
					.imageTransition (swapchainImg.image, eImageLayout::undefined, eImageLayout::transfer_dst)
					.copyImageToImage (handle_rt0, swapchainImg.image, gpu->swapChain_getImageExten2D(), gpu->swapChain_getImageExten2D())
					.imageTransition (swapchainImg.image, eImageLayout::transfer_dst, eImageLayout::presentation)
					.end();

				mainLoop.gfxJob_submitAndPresent (cmdBufferHandle, swapchainImg);
			}
        }		
	}


	//free
	gpu->waitIdle();
	mainLoop.unsetup();
	
	engine->assetHub->unload (assHandle_pipe);
	engine->assetHub->unload (assHandle_texBianca);
	engine->assetHub->unload (assHandle_texChecker);
	engine->shape_release(shapeHandle);


    gpu->deleteResource (handle_zbuffer);
    gpu->deleteResource (handle_rt0);
    gpu->deleteResource (handle_descrPool);

    gpu->deleteResource (handle_descrSet0);
	gpu->deleteResource (handle_descrSet1);
	gpu->deleteResource (handle_descrSet2);
	gpu->deleteResource (handle_ubo_scene);
	gpu->deleteResource (handle_sbo_matrixList);
	gpu->deleteResource (handle_sbo_materiaList);

	gpu->deleteResource (cmdBufferHandle);
	return true;
}

//***************************************
bool Test1::priv_run4 ()
{
	gos::ENGShape shapeHandle;
	if (!priv_shape_create (engine, &shapeHandle))
	{
		DBGBREAK;
		return false;
	}
	const engine::Shape *info_shape = engine->shape_getInfo(shapeHandle);


	//creo il renderer
	renderer = GOSNEW(allocator, gos::engine::Renderer1)();
	renderer->setup (allocator, engine);



    //load degli assets
	asset::Handle assHandle_texBianca;
	asset::Handle assHandle_texChecker;
    if (!engine->assetHub->getHandle ("tex_bianca", &assHandle_texBianca, true))
        return false;
    if (!engine->assetHub->getHandle ("tex_checker", &assHandle_texChecker, true))
        return false;


    
	//renderizzo
    gpu::MainLoop2 mainLoop;
    mainLoop.setup (gpu);

    GPUCmdBufferHandle  cmdBufferHandle;
    gpu->cmdBuffer_create (eGPUQueueFamily::gfx, &cmdBufferHandle);
	
	u64 nextTimeUpdate_msec = 0;

	bool bQuit = false;
	while (false == bQuit)
	{
		if (!engine->update())
		{
			bQuit = true;
			continue;
		}

        mainLoop.stat_onCPUFrameBegin();
        engine->assetHub->update (gos::getTimeSinceStart_msec());
		doCPUStuff();
		{
			/*if (gos::getTimeSinceStart_msec() > nextTimeUpdate_msec)
			{
				nextTimeUpdate_msec = gos::getTimeSinceStart_msec() + 30;
				obj0_roty += 1.0f;
				if (obj0_roty > 360)
					obj0_roty -= 360;

				mat4x4f mat1;
				mat4x4f mat2;
				mat1.buildRotationAboutY (gos::math::gradToRad(obj0_roty));
				gpu->writeAndSync (handle_sbo_matrixList, 0, &mat1, sizeof(mat4x4f));


				mat1.buildTranslation (0, 3, 0);
				mat2.buildRotationAboutZ (gos::math::gradToRad(obj0_roty));
				mat2 = mat1 * mat2;
				gpu->writeAndSync (handle_sbo_matrixList, sizeof(mat4x4f)*2, &mat2, sizeof(mat4x4f));
			}*/			
		}
		mainLoop.stat_onCPUFrameEnd();

        mainLoop.run();

        if (gpu->swapChain_wasRecreated())
            cam.changeAspectRatioPerspectiveFovLH (gpu->swapChain_calcAspectRatio());


        //se il job precedente e' stato presentato, posso schedularne uno nuovo
        gpu::SwapchainImg swapchainImg;
        if (mainLoop.gfxJob_canSubmit(&swapchainImg))
        {
			gos::gpu::pipe2::CmdBufferWriter2 cw;
			cw	.begin (gpu, cmdBufferHandle)
				.setViewport (gpu->viewport_getDefault());

				renderer->begin(&cam);
					//renderer->add()
				renderer->end (cw);

			cw	.imageTransition (renderer->getHandle_rt0(), eImageLayout::color_attachment_optimal, eImageLayout::transfer_src)
				.imageTransition (swapchainImg.image, eImageLayout::undefined, eImageLayout::transfer_dst)
				.copyImageToImage (renderer->getHandle_rt0(), swapchainImg.image, gpu->swapChain_getImageExten2D(), gpu->swapChain_getImageExten2D())
				.imageTransition (swapchainImg.image, eImageLayout::transfer_dst, eImageLayout::presentation)
				.end();

			mainLoop.gfxJob_submitAndPresent (cmdBufferHandle, swapchainImg);
        }		
	}


	//free
	gpu->waitIdle();
	mainLoop.unsetup();
	
	engine->assetHub->unload (assHandle_texBianca);
	engine->assetHub->unload (assHandle_texChecker);
	engine->shape_release(shapeHandle);
	GOSDELETE(allocator, renderer);
	renderer = NULL;


	gpu->deleteResource (cmdBufferHandle);
	return true;
}

