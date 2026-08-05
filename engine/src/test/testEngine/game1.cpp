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

	renderer_line3d = NULL;
	renderer_PIPE3 = NULL;
    handle_skeleton1.setInvalid();
    handle_skeleton2.setInvalid();
    handle_model_player.setInvalid();
    handle_model_pavimento.setInvalid();

	num_missile_alive = 0;
	for (u8 i=0; i<NUM_MAX_MISSILE; i++)
		ent_missile[i].setInvalid();

	cameraMode = eCameraMode::third_person;

}

//***************************************
Game1::~Game1()
{
	entRegistry.unsetup();
    engine->release(handle_skeleton1);
    engine->release(handle_skeleton2);
	engine->release(handle_model_player);
	engine->release(handle_model_pavimento);
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
	renderer_PIPE3 = engine->renderPipe.add_renderer<engine::Renderer_PIPE3>();
	renderer_line3d =engine->renderPipe.add_renderer<engine::Renderer_line3d>();
		


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
				charCtrl.camera_rotate_aboutY ((ev.value < 0));
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
	engine->texture2D_createFromAsset ("tex_checker", &handle_texChecker, res::eLoadMode::asap);


	//binding di materiali al renderer
	{
		const gos::res::Texture2d *tex;
		u32	texture_index__texBianca = u32MAX;
		u32	texture_index__texChecker = u32MAX;

		
		if (!engine->get_texture_bianca (&tex))
		{
			DBGBREAK;
			return false;
		}
		texture_index__texBianca = tex->index;

		if (!engine->get (handle_texChecker, &tex, 5000))
		{
			DBGBREAK;
			return false;
		}
		texture_index__texChecker = tex->index;

		material_indices[0] =renderer_PIPE3->material_create (texture_index__texBianca, vec3f(1.0f, 1.0f, 1.0f));
		material_indices[1] =renderer_PIPE3->material_create (texture_index__texBianca, vec3f(1.0f, 0.0f, 0.0f));
		material_indices[2] =renderer_PIPE3->material_create (texture_index__texBianca, vec3f(0.0f, 1.0f, 0.0f));
		material_indices[3] =renderer_PIPE3->material_create (texture_index__texChecker, vec3f(1.0f, 1.0f, 1.0f));
	}

    return true;
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

	gpu::StageHelper stageHelper;
	stageHelper.setup (gpu, 8192);

	engine->GPUShape_create (&shape_cube, stageHelper, &handle_gpushape_cube); 
	shape::shapeFree (allocator, &shape_cube);

	engine->GPUShape_create (&shape_cylinder, stageHelper, &handle_gpushape_cyl); 
	shape::shapeFree (allocator, &shape_cylinder);

	return true;
}

//***************************************
void Game1::priv_createModel_mainPlayer()
{
	//skeleton1
	{
		gos::Skeleton skeleton1;
		skeleton1.reset();
		gos::skeleton::Builder builder;

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
		builder.end (allocator, &skeleton1);

		engine->skeleton_create (skeleton1, &handle_skeleton1);
		skeleton::free (skeleton1);

	}    

	//model_player
	const res::Skeleton *res_skeleton;
	if (engine->get (handle_skeleton1, &res_skeleton))
	{
		// model::Builder builder;
		// builder.begin(skeleton1);
		// builder.addMeshToBone (handle_gpushape_cyl, material_indices[0], "piedi");
		// builder.addMeshToBone (handle_gpushape_cube, material_indices[1], "occhi");
		// builder.addMeshToBone (handle_gpushape_cube, material_indices[2], "coso-rotante");
		// model_player = builder.end (allocator);

		skeleton::Reader skr(&res_skeleton->skeleton);
		const u32 bone_index__piedi = skr.bone_get_index_by_name("piedi");
		const u32 bone_index__occhi = skr.bone_get_index_by_name("occhi");
		const u32 bone_index__coso_rotante = skr.bone_get_index_by_name("coso-rotante");

		const u32 material_0 = material_indices[0];
		const u32 material_1 = material_indices[1];
		const u32 material_2 = material_indices[2];
		

		const u32 NUM_SHAPES = 2;
		const u32 NUM_MATERIAL = 3;
		const u32 NUM_MESHES = 3;
		gos::Model *model = engine->model_create (handle_skeleton1, NUM_SHAPES, NUM_MATERIAL, NUM_MESHES, &handle_model_player);
		assert (NULL != model);

		model::set_gpushape (*model, 0, handle_gpushape_cyl);
		model::set_gpushape (*model, 1, handle_gpushape_cube);

		model::set_mesh (*model, 0, 0, bone_index__piedi, material_0);
		model::set_mesh (*model, 1, 1, bone_index__occhi, material_1);
		model::set_mesh (*model, 2, 1, bone_index__coso_rotante, material_2);		
	}    
}

//***************************************
void Game1::priv_createModel_pavimento()
{
	//skeleton2
	{
		gos::Skeleton skeleton2;
		skeleton2.reset();
        gos::skeleton::Builder builder;
		
		builder.begin ("piedi");
		builder.end (allocator, &skeleton2);

		engine->skeleton_create (skeleton2, &handle_skeleton2);
		skeleton::free (skeleton2);
	}    

	//model_player
	const res::Skeleton *res_skeleton;
	if (engine->get (handle_skeleton2, &res_skeleton))
	{
		// model::Builder builder;
		// builder.begin(skeleton2);
		// builder.addMeshToBone (handle_gpushape_cube, material_indices[3], "piedi");
		// model_pavimento = builder.end (allocator);

		skeleton::Reader skr(&res_skeleton->skeleton);
		const u32 bone_index__piedi = skr.bone_get_index_by_name("piedi");

		const u32 material_0 = material_indices[3];
		

		const u32 NUM_SHAPES = 1;
		const u32 NUM_MATERIAL = 1;
		const u32 NUM_MESHES = 1;
		gos::Model *model = engine->model_create (handle_skeleton2, NUM_SHAPES, NUM_MATERIAL, NUM_MESHES, &handle_model_pavimento);
		assert (NULL != model);

		model::set_gpushape (*model, 0, handle_gpushape_cube);

		model::set_mesh (*model, 0, 0, bone_index__piedi, material_0);
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
	cpos->scale.set (0.5f, 0.5f, 0.5f);

	geom::Pos3 p3;
	p3.identity();
	p3.lookAt (dir);
	cpos->quat.buildFromMatrix3x3 (p3.rot);
	cpos->pos = o;

	vec3f aax, aay, aaz;
	cpos->quat.toAxis (&aax, &aay, &aaz);

	//shape
	auto cModelInstance = entRegistry.addComponent<ent::CompModelInstance>(ent);
	//engine->modelinst_create (handle_model_pavimento, &cModelInstance->handle_mi);
	engine->modelinst_create (handle_model_albero, &cModelInstance->handle_mi);

	//script
    auto cScript = entRegistry.addComponent<ent::CompScriptable>(ent);
    cScript->callback = Game1__entity_script_missile;
}

//***************************************
void Game1::priv_loop ()
{
	//engine->model_createFromAsset ("model_albero", &handle_model_albero, res::eLoadMode::asap);
	engine->model_createFromAsset ("model_omino", &handle_model_albero, res::eLoadMode::asap);

	const res::Model3d *res_model_albero;
	engine->get (handle_model_albero, &res_model_albero, 4000);


	for (u32 i=0; i<3; i++)
	{
		ent_cubi[i] = entRegistry.newEntity();

		entRegistry.addComponent<ent::CompTransform3>(ent_cubi[i]);
        auto cpos = entRegistry.addComponent<ent::CompPos>(ent_cubi[i]);
        cpos->reset();
		cpos->scale.set (1, 0.95f, 1);
		cpos->pos.set (-5.0, 0.5f + (f32)i, 0);
		

        auto cModelInstance = entRegistry.addComponent<ent::CompModelInstance>(ent_cubi[i]);
		engine->modelinst_create (handle_model_pavimento, &cModelInstance->handle_mi);
	}




    ent_mainPlayer = entRegistry.newEntity();
    {
		entRegistry.addComponent<ent::CompTransform3>(ent_mainPlayer);
        auto cpos = entRegistry.addComponent<ent::CompPos>(ent_mainPlayer);
        cpos->reset();

        //shape
        auto cModelInstance = entRegistry.addComponent<ent::CompModelInstance>(ent_mainPlayer);
		engine->modelinst_create (handle_model_player, &cModelInstance->handle_mi);
		//engine->modelinst_create (handle_model_albero, &cModelInstance->handle_mi);

        auto cScript = entRegistry.addComponent<ent::CompScriptable>(ent_mainPlayer);
        cScript->callback = Game1__entity_script_mainPlayer;
    }

    Entity ent_pavimento = entRegistry.newEntity();
    {
		gos::mat4x4f mTR;
		mTR.buildTranslation (vec3f(0, 0, 0));
		gos::mat4x4f mSC;
		mSC.buildScale(100.0f, 0.1f, 100.0f);
		gos::mat4x4f mFinal =  mTR * mSC;

        //shape
        auto cModelInstance = entRegistry.addComponent<ent::CompModelInstance>(ent_pavimento);
		engine->modelinst_create (handle_model_pavimento, &cModelInstance->handle_mi);
		engine->modelinst_applyTransform (cModelInstance->handle_mi, mFinal);
    }


	//char controller
	charCtrl.bind (ent_mainPlayer, &cam);


	//line3d
	gos::engine::Renderer_line3d::Ctx *line_ctx1 = renderer_line3d->ctx__create_new("ctx1", 32);
		line_ctx1->clear();
		line_ctx1->vtx_add (0,0,0);
		line_ctx1->vtx_add (10,0,0);
		line_ctx1->vtx_add (0,10,0);
		line_ctx1->vtx_add (0,0,10);
		
		line_ctx1->set_color_ARGB (0xFFFF0000); 	line_ctx1->line (0, 1);
		line_ctx1->set_color_ARGB (0xFF00FF00); 	line_ctx1->line (0, 2);
		line_ctx1->set_color_ARGB (0xFF0000FF); 	line_ctx1->line (0, 3);

	gos::engine::Renderer_line3d::Ctx *line_ctx2 = renderer_line3d->ctx__create_new("ctx2", 32);
	{
		FastArray<vec3f> vtxList (gos::getScrapAllocator(), 64);

		static constexpr u8 NUM_POINT = 6;
		geom::circle (&vtxList, vec3f(0,0,0), 4.0f, NUM_POINT, -90.0f);
		line_ctx2->set_color_ARGB (0xFFFF00FF);
		line_ctx2->enable_depth_test(true);
		line_ctx2->closed_line (vtxList, NUM_POINT);

		vtxList.reset();
		geom::circle (&vtxList, vec3f(0.3f + 8 * cosf(math::gradToRad(30)),0,0), 4.0f, NUM_POINT, -90.0f);
		line_ctx2->set_color_ARGB (0xFF00FFFF); 
		line_ctx2->enable_depth_test(false);
		line_ctx2->closed_line (vtxList, NUM_POINT);

	}

	// gos::engine::Rend_line3d::Ctx line_ctx1;
	// line_ctx1->setup (allocator, 32);
	// line_ctx1->line (vec3f(0,0,0), vec3f(10,0,0));

	// gos::engine::Rend_line3d::Ctx line_ctx2;
	// line_ctx2->setup (allocator, 32);
	// line_ctx2->line (vec3f(1,1,1), vec3f(5,5,5));


	
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
                list->forEach ( [&entRegistry = entRegistry, engine=this->engine](u32 index, Entity ent) {
                    auto cpos = entRegistry.get<ent::CompPos>(ent, false);
					auto ctransf = entRegistry.get<ent::CompTransform3>(ent, false);
                    cpos->buildMatrix(&ctransf->matrix);

                    //se queste hanno il componente modelInstance, aggiorno pure quello
                    auto cModelInstance = entRegistry.get<ent::CompModelInstance>(ent, false);
                    if (NULL != cModelInstance)
                    {
						engine->modelinst_applyTransform (cModelInstance->handle_mi, ctransf->matrix);
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
			mainLoop.stat_onCommandBufferBegin();
			{
				renderer_PIPE3->begin();
				{
					renderer_PIPE3->add ( entRegistry.query<ent::CompModelInstance>(ent_mainPlayer) );
					renderer_PIPE3->add ( entRegistry.query<ent::CompModelInstance>(ent_pavimento) );
					renderer_PIPE3->add ( entRegistry.query<ent::CompModelInstance>(ent_cubi[0]) );
					renderer_PIPE3->add ( entRegistry.query<ent::CompModelInstance>(ent_cubi[1]) );
					renderer_PIPE3->add ( entRegistry.query<ent::CompModelInstance>(ent_cubi[2]) );
						

					for (u8 i = 0; i < NUM_MAX_MISSILE; i++)
					{
						if (ent_missile[i].isValid())
							renderer_PIPE3->add ( entRegistry.query<ent::CompModelInstance>(ent_missile[i]) );
					}
		
				}
				renderer_PIPE3->end ();
			}
			engine->renderPipe.render (swapchainImg, cmdBufferHandle, &cam);
			mainLoop.stat_onCommandBufferEnd();
			mainLoop.gfxJob_submitAndPresent (cmdBufferHandle, swapchainImg);
        }
	}
	engine->release(handle_model_albero);

	//free
	gpu->waitIdle();
	mainLoop.unsetup();

	//free delle modelInstance
	entRegistry.getAllEntitiesWith<ent::CompModelInstance>()->forEach ([engine=this->engine](ent::CompModelInstance *comp, gos::Entity ent){
		engine->release (comp->handle_mi);
	});    

	//free asset
    engine->release(handle_gpushape_cube);
    engine->release(handle_gpushape_cyl);
	engine->release (handle_texChecker);
	

	gpu->deleteResource (cmdBufferHandle);
}

