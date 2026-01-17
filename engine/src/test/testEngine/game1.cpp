#include "game1.h"
#include "gosShapePrefabs.h"
#include "gosGeomUtils.h"

using namespace gos;


struct CompMissile
{
	u64			timeToDie_msec;
	gos::Entity	*me;
};

//***************************************
Game1::Game1()
{
	allocator = gos::getSysHeapAllocator();

	entRegistry.setup();
	entRegistry.addComponentHandler<ent::CompTransform3>();
	entRegistry.addComponentHandler<ent::CompPos>(true);
	entRegistry.addComponentHandler<ent::CompModelInstance>();
	entRegistry.addComponentHandler<ent::CompScriptable>();
	entRegistry.addComponentHandler<CompMissile>();

	renderer = NULL;
	rend_line3d = NULL;
    skeleton1 = NULL;
    skeleton2 = NULL;
    model_player = NULL;
    model_pavimento = NULL;

	num_missile_alive = 0;
	for (u8 i=0; i<NUM_MAX_MISSILE; i++)
		ent_missile[i].setInvalid();

	cameraMode = eCameraMode::third_person;

}

//***************************************
Game1::~Game1()
{
	entRegistry.unsetup();
	GOSDELETE(allocator, renderer);
	GOSDELETE(allocator, rend_line3d);
    GOSDELETE(allocator, skeleton1);
    GOSDELETE(allocator, skeleton2);
    GOSDELETE(allocator, model_player);
    GOSDELETE(allocator, model_pavimento);
}

//***************************************
void Game1::run (gos::Engine *engineIN)
{
	engine = engineIN;
	gpu = engine->gpu;

	//input
	engine->inputCtx->
		action_add ("mouse-wheel")
		.action_add ("mouse-LB")
		.action_add ("toggle_cam_mode");

	engine->inputCtx->action_bindToAxleREL ("mouse-wheel", input::eOrigin::mouse, input::eAxle::z, input::eAxleDirection::both);
	engine->inputCtx->action_bindToBtn ("mouse-LB", input::eOrigin::mouse, 0, input::eButtonStatus::pressed);
	engine->inputCtx->action_bindToBtn ("toggle_cam_mode", input::eOrigin::keyboard, GLFW_KEY_TAB, input::eButtonStatus::pressed, input::sButtonModifier(input::eButtonModifier::LSHIFT));

	//renderer
	renderer = GOSNEW(allocator, gos::engine::Renderer1)();
	renderer->setup (allocator, engine);


	rend_line3d = GOSNEW(allocator, gos::engine::Rend_line3d)();
	rend_line3d->setup (allocator, engine);

	//setup camera
    cam.setPerspectiveFovLH(gpu->swapChain_calcAspectRatio(),  math::gradToRad(45), 0.1f, 250.0f);
    cam.pos.identity();
    cam.pos.warp (0, 15.0f, -10);
	cam.pos.lookAt (vec3f(0,0,0));
	cam.markUpdated();

	//e movement
    movement.bind (&cam.pos);
	
    if (!priv_loadAssets())
    {
        gos::logger::err ("load asset failed\n");
        return;
    }

    priv_createShapes();
    priv_createModel_mainPlayer();
    priv_createModel_pavimento();
    priv_loop();
}


//**********************************
void Game1::doCPUStuff ()
{
    const u64 timeNow_msec = gos::getTimeSinceStart_msec();

	Engine::InputEvent ev;
	while (engine->inputEvent_getNext(&ev))
	{
		switch (ev.actionID)
		{
		case COMPILE_TIME_STR_CRC32("toggle_cam_mode"):
			if (eCameraMode::third_person == cameraMode)
				cameraMode = eCameraMode::free_cam;
			else
				cameraMode = eCameraMode::third_person;
			break;



		case COMPILE_TIME_STR_CRC32("mouse-wheel"):
			charCtrl.camera_adjust_distance ( (ev.value<0)? true: false);
			break;

		case COMPILE_TIME_STR_CRC32("mouse-LB"):
		{
			const ent::CompPos *cpos = entRegistry.query<ent::CompPos>(ent_mainPlayer);
			
			vec3f ax,ay,az;
			cpos->quat.toAxis (&ax, &ay, &az);
			priv_spawnMissile (cpos->pos, az);
		}
		break;

		case COMPILE_TIME_STR_CRC32("move_forward"):
			if (eCameraMode::free_cam == cameraMode)
				movement.moveForward ((ev.value == 1));
			else
				charCtrl.moveForward ((ev.value == 1));
			break;

		case COMPILE_TIME_STR_CRC32("move_backward"):
			if (eCameraMode::free_cam == cameraMode)
				movement.moveBackward ((ev.value == 1));
			else
				charCtrl.moveBackward ((ev.value == 1));
			break;

		case COMPILE_TIME_STR_CRC32("strafe_left"):
			if (eCameraMode::free_cam == cameraMode)
				movement.strafeLeft ((ev.value == 1));  
			else
				charCtrl.strafeLeft ((ev.value == 1));  
			break;

		case COMPILE_TIME_STR_CRC32("strafe_right"):
			if (eCameraMode::free_cam == cameraMode)
				movement.strafeRight ((ev.value == 1));
			else
				charCtrl.strafeRight ((ev.value == 1));
			break;

		case COMPILE_TIME_STR_CRC32("rotateY"):
			if (eCameraMode::free_cam == cameraMode)
				movement.rotateY ((ev.value < 0));
			else
				charCtrl.camera_rotate_aboutY ((ev.value > 0));
			break;

		case COMPILE_TIME_STR_CRC32("rotateX"):
			if (eCameraMode::free_cam == cameraMode)
				movement.rotateX ((ev.value < 0));
			else
				charCtrl.camera_rotate_aboutX ((ev.value < 0));
			break;

		case COMPILE_TIME_STR_CRC32("strafe_up"):
			if (eCameraMode::free_cam == cameraMode)
				movement.strafeUp ((ev.value == 1));
			break;

		case COMPILE_TIME_STR_CRC32("strafe_down"):
			if (eCameraMode::free_cam == cameraMode)
				movement.strafeDown ((ev.value == 1));
			break;

		}
	}

	//charCtrl

    //gestione del movimento
	if (eCameraMode::free_cam == cameraMode)
		movement.update(timeNow_msec);
	else
		charCtrl.update(entRegistry, timeNow_msec);
		


    cam.markUpdated();
}


//***************************************
bool Game1::priv_loadAssets()
{
	engine->texture2D_createFromAsset ("tex_bianca", &handle_texBianca);
	engine->texture2D_createFromAsset ("tex_checker", &handle_texChecker, engine::eLoadMode::asap);


	//binding di materiali al renderer
	{
		const gos::engine::ResTexture *tex;
		u32	texture_index__texBianca = u32MAX;
		u32	texture_index__texChecker = u32MAX;

		
		if (!engine->get (handle_texBianca, &tex, 5000))
		{
			DBGBREAK;
			return false;
		}
		texture_index__texBianca = renderer->texture_addIfNotExitst(tex->data.texHandle);

		if (!engine->get (handle_texChecker, &tex, 5000))
		{
			DBGBREAK;
			return false;
		}
		texture_index__texChecker = renderer->texture_addIfNotExitst(tex->data.texHandle);

		material_indices[0] = renderer->material_create (texture_index__texBianca, vec3f(1.0f, 1.0f, 1.0f));
		material_indices[1] = renderer->material_create (texture_index__texBianca, vec3f(1.0f, 0.0f, 0.0f));
		material_indices[2] = renderer->material_create (texture_index__texBianca, vec3f(0.0f, 1.0f, 0.0f));
		material_indices[3] = renderer->material_create (texture_index__texChecker, vec3f(1.0f, 1.0f, 1.0f));
	}

    return true;
}

//***************************************
gos::ENGGPUShape Game1::priv_create_engineShape (GPUStgBufferHandle stgBufferHandle, GPUCmdBufferHandle cmdBufferHandle, const gos::Shape *shapeSRC)
{
	gos::ENGGPUShape handle_shape;
	handle_shape.setInvalid();

	if (engine->GPUShape_create (shapeSRC, &handle_shape))
	{
		const u32 SIZE_OF_IDX = shapeSRC->numIdx * sizeof(u16);
		engine->gpu->stagingBuffer_memcpy (stgBufferHandle, 0, shapeSRC->idxBuffer, SIZE_OF_IDX);

		const u32 SIZE_OF_VTX = shape::calcSizeOfAVertex(shapeSRC->vtxLayout) * shapeSRC->numVtx;
		engine->gpu->stagingBuffer_memcpy (stgBufferHandle, SIZE_OF_IDX, shapeSRC->vtxBuffer, SIZE_OF_VTX);

		//creo un job per pushare lo stage buffer in VB/IB
		const engine::ResGPUShape *shapeInfo;
		if (!engine->get (handle_shape, &shapeInfo))
			DBGBREAK;


		gos::gpu::CmdBufferWriter2 cw;
		cw.begin (engine->gpu, cmdBufferHandle)
			.copyBuffer (stgBufferHandle, shapeInfo->ibHandle, 0, shapeInfo->alloc_idxbuf_offset, SIZE_OF_IDX)
			.copyBuffer (stgBufferHandle, shapeInfo->vbHandle, SIZE_OF_IDX, shapeInfo->alloc_vtxbuf_offset, SIZE_OF_VTX)
			.end();

		gpu::TransferJob job;
		job.setup (engine->gpu);
		job.submit(cmdBufferHandle);

		while (!job.hasFinished())
		{
		}
	}

	return handle_shape;
}

//***************************************
bool Game1::priv_createShapes()
{
	//creo una shape
	gos::Shape shape_cube;
	gos::Shape shape_cylinder;
	
	shape_cube.reset();
	shape_cylinder.reset();
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

		if (!shape::buildCube24 (vec3f(0, 0, 0), vec3f(1, 1, 1), vtxLayout, allocator, &shape_cube))
			return false;

        const f32 CYL_HEIGHT = 1.8f;
		if (!shape::buildCylinder (vec3f(0, 0, 0), 0.6f, CYL_HEIGHT, 32, 4, true, true, vtxLayout, allocator, &shape_cylinder))
			return false;
	}



	engine->utils__quick_and_dirty__create_GPUSHape_and_stageIt_to_VB_IB (&shape_cube, &handle_gpushape_cube); 
	shape::shapeFree (allocator, &shape_cube);
	engine->utils__quick_and_dirty__create_GPUSHape_and_stageIt_to_VB_IB (&shape_cylinder, &handle_gpushape_cyl); 
	shape::shapeFree (allocator, &shape_cylinder);

	////staging buffer per la copia di VB/IB in GPU
	//GPUStgBufferHandle stgBufferHandle;
	//engine->gpu->stagingBuffer_create (8192, &stgBufferHandle);

	//GPUCmdBufferHandle cmdBufferHandle;
	//if (!engine->gpu->cmdBuffer_create (eGPUQueueFamily::transfer, &cmdBufferHandle))
	//	return false;


	////creo una engine::shape
	//handle_gpushape_cube = priv_create_engineShape(stgBufferHandle, cmdBufferHandle, &shape_cube);
	//shape::shapeFree (allocator, &shape_cube);


	//handle_gpushape_cyl = priv_create_engineShape (stgBufferHandle, cmdBufferHandle, &shape_cylinder);
	//shape::shapeFree (allocator, &shape_cylinder);


	//engine->gpu->deleteResource(cmdBufferHandle);
	//engine->gpu->deleteResource(stgBufferHandle);
	return true;
}

//***************************************
void Game1::priv_createModel_mainPlayer()
{
	//skeleton1
	{
		gos::SkeletonBuilder builder;

        gos::mat4x4f mT, mS;

		gos::Bone *bone;
		const u32 iRoot = builder.begin ("piedi", &bone);

		builder.addChildTo (iRoot, "occhi", &bone);
            mT.buildTranslation (vec3f(0, 1.6f, 0.5f));
            mS.buildScale  (vec3f(1.1f, 0.3f, 0.4f));
			bone->matrix = mT * mS;

		builder.addChildTo (iRoot, "coso-rotante", &bone);
            mT.buildTranslation (vec3f(0.8f, 0.05f, 0));
            mS.buildScale  (vec3f(0.1f, 0.1f, 0.1f));
			bone->matrix = mT * mS;
		skeleton1 = builder.end (allocator);
	}    

	//model_player
	{
		model::Builder builder;
		builder.begin(skeleton1);
		builder.addMeshToBone (handle_gpushape_cyl, material_indices[0], "piedi");
		builder.addMeshToBone (handle_gpushape_cube, material_indices[1], "occhi");
		builder.addMeshToBone (handle_gpushape_cube, material_indices[2], "coso-rotante");
		model_player = builder.end (allocator);
	}    
}

//***************************************
void Game1::priv_createModel_pavimento()
{
	//skeleton2
	{
        gos::SkeletonBuilder builder;
		builder.begin ("piedi");
		skeleton2 = builder.end (allocator);
	}    

	//model_pavimento = GOSNEW(allocator, model::Model)();
	{
		model::Builder builder;
		builder.begin(skeleton2);
		builder.addMeshToBone (handle_gpushape_cube, material_indices[3], "piedi");
		model_pavimento = builder.end (allocator);
	}    
}

//***************************************
void Game1__entity_script_mainPlayer (Entity ent, ent::Registry *registry)
{
}

//***************************************
void Game1__entity_script_missile (Entity ent, ent::Registry *registry)
{
	auto cmiss = registry->get<CompMissile>(ent);
	if (gos::getTimeSinceStart_msec() >= cmiss->timeToDie_msec)
	{
		registry->removeComponent<ent::CompScriptable>(ent);
		cmiss->me->setInvalid();
		return;
	}


	auto cpos = registry->get<ent::CompPos>(ent, true);

	vec3f ax, ay, az;
	cpos->quat.toAxis (&ax, &ay, &az);

	cpos->pos += az * 2.0f;
}


//***************************************
void Game1::priv_spawnMissile (const gos::vec3f &o, const gos::vec3f dir)
{
	u32 index = u32MAX;
	for (u8 i = 0; i < NUM_MAX_MISSILE; i++)
	{
		if (ent_missile[i].isInvalid())
		{
			index = i;
			break;
		}
	}
	if (u32MAX == index)
		return;

	gos::Entity ent = ent_missile[index] = entRegistry.newEntity();

	auto cmiss = entRegistry.addComponent<CompMissile>(ent);
		cmiss->timeToDie_msec = gos::getTimeSinceStart_msec() + 2000;
		cmiss->me = &ent_missile[index];


	//pos
	entRegistry.addComponent<ent::CompTransform3>(ent);
	auto cpos = entRegistry.addComponent<ent::CompPos>(ent);
	cpos->reset();

	geom::Pos3 p3;
	p3.identity();
	p3.lookAt (dir);
	cpos->quat.buildFromMatrix3x3 (p3.rot);
	cpos->pos = o;

	//shape
	auto cModelInstance = entRegistry.addComponent<ent::CompModelInstance>(ent);
	cModelInstance->model_instance.setup (model_pavimento);

	//script
    auto cScript = entRegistry.addComponent<ent::CompScriptable>(ent);
    cScript->callback = Game1__entity_script_missile;
}

//***************************************
void Game1::priv_loop ()
{
    ent_mainPlayer = entRegistry.newEntity();
    {
		entRegistry.addComponent<ent::CompTransform3>(ent_mainPlayer);
        auto cpos = entRegistry.addComponent<ent::CompPos>(ent_mainPlayer);
        cpos->reset();

        //shape
        auto cModelInstance = entRegistry.addComponent<ent::CompModelInstance>(ent_mainPlayer);
        cModelInstance->model_instance.setup (model_player);

        auto cScript = entRegistry.addComponent<ent::CompScriptable>(ent_mainPlayer);
        cScript->callback = Game1__entity_script_mainPlayer;
    }

    Entity ent_pavimento = entRegistry.newEntity();
    {
		gos::mat4x4f mTR;
		mTR.buildTranslation (vec3f(0, -0.1f, 0));
		gos::mat4x4f mSC;
		mSC.buildScale(100.0f, 0.1f, 100.0f);
		gos::mat4x4f mFinal =  mTR * mSC;

        //shape
        auto cModelInstance = entRegistry.addComponent<ent::CompModelInstance>(ent_pavimento);
        cModelInstance->model_instance.setup (model_pavimento);
		cModelInstance->model_instance.applyTransform (mFinal);
    }


	//char controller
	charCtrl.bind (ent_mainPlayer, &cam);


	//line3d
	gos::engine::Rend_line3d::Ctx line_ctx1;
	line_ctx1.setup (allocator, 32);

	line_ctx1.clear();
	line_ctx1.vtx_add (0,0,0);
	line_ctx1.vtx_add (10,0,0);
	line_ctx1.vtx_add (0,10,0);
	line_ctx1.vtx_add (0,0,10);
	
	line_ctx1.set_color_ARGB (0xFFFF0000); 	line_ctx1.line (0, 1);
	line_ctx1.set_color_ARGB (0xFF00FF00); 	line_ctx1.line (0, 2);
	line_ctx1.set_color_ARGB (0xFF0000FF); 	line_ctx1.line (0, 3);

	gos::engine::Rend_line3d::Ctx line_ctx2;
	line_ctx2.setup (allocator, 32);
	{
		FastArray<vec3f> vtxList (gos::getScrapAllocator(), 64);

		static constexpr u8 NUM_POINT = 6;
		geom::circle (&vtxList, vec3f(0,0,0), 4.0f, NUM_POINT, -90.0f);
		line_ctx2.set_color_ARGB (0xFFFF00FF);
		line_ctx2.enable_depth_test(true);
		line_ctx2.closed_line (vtxList, NUM_POINT);

		vtxList.reset();
		geom::circle (&vtxList, vec3f(0.3f + 8 * cosf(math::gradToRad(30)),0,0), 4.0f, NUM_POINT, -90.0f);
		line_ctx2.set_color_ARGB (0xFF00FFFF); 
		line_ctx2.enable_depth_test(false);
		line_ctx2.closed_line (vtxList, NUM_POINT);

	}

	// gos::engine::Rend_line3d::Ctx line_ctx1;
	// line_ctx1.setup (allocator, 32);
	// line_ctx1.line (vec3f(0,0,0), vec3f(10,0,0));

	// gos::engine::Rend_line3d::Ctx line_ctx2;
	// line_ctx2.setup (allocator, 32);
	// line_ctx2.line (vec3f(1,1,1), vec3f(5,5,5));


    //loop
    u64 nextTimeUpdate_msec = 0;
    gpu::MainLoop2 mainLoop;
    mainLoop.setup (gpu);

    GPUCmdBufferHandle  cmdBufferHandle;
    gpu->cmdBuffer_create (eGPUQueueFamily::gfx, &cmdBufferHandle);
	
	ent::UniqueList	ent_uniqueList;
	ent_uniqueList.setup (allocator, 1024);

	bool bQuit = false;
	while (false == bQuit)
	{
		if (!engine->update())
		{
			bQuit = true;
			continue;
		}

		mainLoop.run();

        //CPU jobs
		mainLoop.stat_onCPUFrameBegin();
		{
			doCPUStuff();


            //itero tutte le ent che hanno il componente <scriptable>
            if (gos::getTimeSinceStart_msec() > nextTimeUpdate_msec)
            {
                nextTimeUpdate_msec = gos::getTimeSinceStart_msec() + 30;

                entRegistry.getAllEntitiesWith<ent::CompScriptable>()->forEach ([&entRegistry = entRegistry](ent::CompScriptable *cScript, gos::Entity ent){
                    cScript->callback (ent, &entRegistry);
                });
            }
            mainLoop.run(); //questo lo chiamo per aggiornare il timer gfxJob in modo che il tempo di "GPU" sia printato con + accuratezza
            
                

            //aggiornamento posizione delle entities
            //itero tutte le ent che hanno modificato il proprio componente <position>
            {
                auto list = entRegistry.getUpdatedEntityList<ent::CompPos>();
                list->forEach ( [&entRegistry = entRegistry](u32 index, Entity ent) {
                    auto cpos = entRegistry.get<ent::CompPos>(ent, false);
					auto ctransf = entRegistry.get<ent::CompTransform3>(ent, false);
                    cpos->buildMatrix(&ctransf->matrix);

                    //se queste hanno il componente modelInstance, aggiorno pure quello
                    auto cModelInstance = entRegistry.get<ent::CompModelInstance>(ent, false);
                    if (NULL != cModelInstance)
                    {
                        cModelInstance->model_instance.applyTransform (ctransf->matrix);
                    }


                    return true;
                });
                list->reset();
            }
        }
		mainLoop.stat_onCPUFrameEnd();		


		//rendering
        if (gpu->swapChain_wasRecreated())
            cam.changeAspectRatioPerspectiveFovLH (gpu->swapChain_calcAspectRatio());

        //se il job precedente e' stato presentato, posso schedularne uno nuovo
        gpu::SwapchainImg swapchainImg;
        if (mainLoop.gfxJob_canSubmit(&swapchainImg))
        {
			gos::gpu::CmdBufferWriter2 cw;
			cw	.begin (gpu, cmdBufferHandle)
				.setViewport (gpu->viewport_getDefault());

			mainLoop.stat_onCommandBufferBegin();
			{
				renderer->begin(&cam);
				{
                    renderer->add ( entRegistry.query<ent::CompModelInstance>(ent_mainPlayer) );
                    renderer->add ( entRegistry.query<ent::CompModelInstance>(ent_pavimento) );

					for (u8 i = 0; i < NUM_MAX_MISSILE; i++)
					{
						if (ent_missile[i].isValid())
							renderer->add ( entRegistry.query<ent::CompModelInstance>(ent_missile[i]) );
					}
		
				}
				renderer->end (cw);
			}
			mainLoop.stat_onCommandBufferEnd();


			//line3d
			{
				gpu::RenderCtx rctx;
				cw	.renderCtx_define_begin(&rctx)
					.withRenderArea (renderer->getHandle_rt0())
					.withRT (renderer->getHandle_rt0(), eAttachmentLoadOp::load, eAttachmentStoreOp::dont_care, gos::ColorHDR(0, 0.1f, 0.1f))
					.withZB (renderer->getHandle_zbuffer(), eAttachmentLoadOp::load, eAttachmentStoreOp::dont_care)
					.define_end();
			
					rend_line3d->begin (&cam, &rctx);
						rend_line3d->appendToCommandBuffer (line_ctx2);
						rend_line3d->appendToCommandBuffer (line_ctx1);
						
						
						
					rend_line3d->end();

				rctx.end_render_ctx();
			}

			//present
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

	//free delle modelInstance
	entRegistry.getAllEntitiesWith<ent::CompModelInstance>()->forEach ([](ent::CompModelInstance *comp, gos::Entity ent){
		comp->model_instance.unsetup();
	});    

	//free asset
    engine->release(handle_gpushape_cube);
    engine->release(handle_gpushape_cyl);
	engine->release (handle_texBianca);
	engine->release (handle_texChecker);
	

	gpu->deleteResource (cmdBufferHandle);
}

