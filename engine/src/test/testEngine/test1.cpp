#include "test1.h"
#include "gosShapePrefabs.h"


using namespace gos;


//***************************************
Test1::Test1()
{
	allocator = gos::getSysHeapAllocator();

	entRegistry.setup();
	entRegistry.addComponentHandler<ent::CompTransform3>();
	entRegistry.addComponentHandler<ent::CompPos>(true);
	entRegistry.addComponentHandler<ent::CompModelInstance>();
	entRegistry.addComponentHandler<ent::CompScriptable>();

	handle_skeleton.setInvalid();
	handle_model.setInvalid();
	renderer = NULL;

	nextTimeUpdate_msec = 0;
}


//***************************************
Test1::~Test1()
{
	entRegistry.unsetup();

	engine->release(handle_skeleton);
	engine->release(handle_model);
	GOSDELETE(allocator, renderer);
}

//***************************************
bool Test1::priv_shape_create (gos::Engine *engine, gos::ENGGPUShape *out_cube, gos::ENGGPUShape *out_cylinder)
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
		if (!shape::buildCylinder (vec3f(0, 0, 0), 0.7f, 1.0f, 32, 4, true, true, vtxLayout, allocator, &shape_cylinder))
			return false;
	}

	//staging buffer per la copia di VB/IB in GPU
	gpu::StageHelper stageHelper;
	stageHelper.setup (gpu, 8192);


	//creo una engine::shape
	engine->GPUShape_create (&shape_cube, stageHelper, out_cube);
	shape::shapeFree (allocator, &shape_cube);


	engine->GPUShape_create (&shape_cylinder, stageHelper, out_cylinder);
	shape::shapeFree (allocator, &shape_cylinder);

	return true;
}

//***************************************
void Test1::priv_model_setup(gos::ENGGPUShape shape_cube, gos::ENGGPUShape shape_cylinder)
{
	//skeleton
	{
		gos::Skeleton sk;
		sk.reset();
		gos::skeleton::Builder builder;

		gos::Bone *bone;
		const u32 iRoot = builder.begin ("root");
		builder.addChildTo (iRoot, "up-arm", &bone);
			bone->matrix.buildTranslation(vec3f(0,  1.3f, 0));
		builder.addChildTo (iRoot, "down-arm", &bone);
			bone->matrix.buildTranslation(vec3f(0, -1.3f, 0));
		if (!builder.end (allocator, &sk))
		{
			logger::err ("priv_model_setup() => error creating skeleton\n");
		}

		if (!engine->skeleton_create (sk, &handle_skeleton))
		{
			logger::err ("priv_model_setup() => error creating skeleton(2)\n");
		}

		skeleton::free (sk);
	}


	const res::Skeleton *res_skeleton;
	if (engine->get (handle_skeleton, &res_skeleton))
	{
		// model::Builder builder;
		// builder.begin(skeleton);
		// builder.addMeshToBone (shape_cylinder, 0, "root");
		// builder.addMeshToBone (shape_cube, 1, "up-arm");
		// builder.addMeshToBone (shape_cube, 2, "down-arm");
		// model = builder.end(allocator);

		skeleton::Reader skr(&res_skeleton->skeleton);
		const u32 bone_index__root = skr.bone_get_index_by_name("root");
		const u32 bone_index__up_arm = skr.bone_get_index_by_name("up-arm");
		const u32 bone_index__down_arm = skr.bone_get_index_by_name("down-arm");

		const u32 material_0 = 0;
		const u32 material_1 = 1;
		const u32 material_2 = 2;
		

		const u32 NUM_SHAPES = 2;
		const u32 NUM_MATERIAL = 3;
		const u32 NUM_MESHES = 3;
		gos::Model *model = engine->model_create (handle_skeleton, NUM_SHAPES, NUM_MATERIAL, NUM_MESHES, &handle_model);
		assert (NULL != model);

		model::set_gpushape (*model, 0, shape_cylinder);
		model::set_gpushape (*model, 1, shape_cube);

		model::set_mesh (*model, 0, 0, bone_index__root, material_0);
		model::set_mesh (*model, 1, 1, bone_index__up_arm, material_1);
		model::set_mesh (*model, 2, 1, bone_index__down_arm, material_2);
		return;
	}

	//lo skeleton non era 'ready'
	DBGBREAK;

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
void Test1__entity_script_callback_0 (Entity ent, ent::Registry *registry)
{
	auto cpos = registry->get<ent::CompPos>(ent);
	cpos->quat.rotateMeAbout (vec3f(1,0,0), math::gradToRad(1));
}
void Test1__entity_script_callback_1 (Entity ent, ent::Registry *registry)
{
	auto cpos = registry->get<ent::CompPos>(ent);
	cpos->quat.rotateMeAbout (vec3f(0,1,0), math::gradToRad(1));
}
void Test1__entity_script_callback_2 (Entity ent, ent::Registry *registry)
{
	auto cpos = registry->get<ent::CompPos>(ent);
	cpos->quat.rotateMeAbout (vec3f(0,0,1), math::gradToRad(1));
}
void Test1__entity_script_callback_3 (Entity ent, ent::Registry *registry)
{
	auto cpos = registry->get<ent::CompPos>(ent);

	cpos->quat.rotateMeAbout (vec3f(1,0,0), math::gradToRad(1));
	vec3f ax, ay, az;
	cpos->quat.toAxis (&ax, &ay, &az);
	//cpos->quat.rotateMeAbout (ax, math::gradToRad(1));
	cpos->quat.rotateMeAbout (ay, math::gradToRad(4));
	
}

//***************************************
bool Test1::priv_run4 ()
{
	//shape
	gos::ENGGPUShape handle_shape_list[4];
	if (!priv_shape_create (engine, &handle_shape_list[0], &handle_shape_list[1]))
	{
		DBGBREAK;
		return false;
	}
	priv_shape_create (engine, &handle_shape_list[2], &handle_shape_list[3]);


	//model
	priv_model_setup (handle_shape_list[0], handle_shape_list[1]);



	//creo il renderer
	renderer = GOSNEW(allocator, gos::engine::Renderer1)();
	renderer->setup (allocator, engine);


    //load degli assets
	gos::ENGTexture	handle_texBianca;
	gos::ENGTexture	handle_texChecker;
	engine->texture2D_createFromAsset ("tex_bianca", &handle_texBianca);
	engine->texture2D_createFromAsset ("tex_checker", &handle_texChecker, res::eLoadMode::asap);

	//binding di materiali al renderer
	{
		const gos::res::Texture2d *tex;
		u32	texture_index__texBianca = u32MAX;
		u32	texture_index__texChecker = u32MAX;

		if (!engine->get (handle_texBianca, &tex, 5000))
			return false;
		texture_index__texBianca = renderer->texture_addIfNotExitst(tex->texHandle);

		if (!engine->get (handle_texChecker, &tex, 5000))
			return false;
		texture_index__texChecker = renderer->texture_addIfNotExitst(tex->texHandle);

		renderer->material_create (texture_index__texBianca, vec3f(1.0f, 1.0f, 1.0f));
		renderer->material_create (texture_index__texChecker, vec3f(1.0f, 1.0f, 1.0f));
		renderer->material_create (texture_index__texBianca, vec3f(1.0f, 0.2f, 0.2f));
		renderer->material_create (texture_index__texChecker, vec3f(0.2f, 1.0f, 0.2f));
	}


	//creo una scena
	engine::Scene scene;
	scene.setup (allocator);
	scene.begin();
	{
		const f32 SCALE = 0.1f;

#ifdef _DEBUG
		const u32 NUM_ENTITIES_X = 100;
		const u32 NUM_ENTITIES_Z = 100;
#else
		const u32 NUM_ENTITIES_X = 1000;
		const u32 NUM_ENTITIES_Z = 100;
#endif		
		const f32 ENTITY_GRID_X = 2.5f * SCALE;
		const f32 ENTITY_GRID_Z = 2.5f * SCALE;

		const f32 x_min = - ((f32)NUM_ENTITIES_X / 2.0f) * ENTITY_GRID_X;
		const f32 z_min = - ((f32)NUM_ENTITIES_Z / 2.0f) * ENTITY_GRID_Z;

		f32 zz = z_min;
		for (u32 z=0; z<NUM_ENTITIES_Z; z++)
		{
			f32 xx = x_min;
			for (u32 x=0; x<NUM_ENTITIES_X; x++)
			{
				Entity ent = entRegistry.newEntity();
				
				//transform
				entRegistry.addComponent<ent::CompTransform3>(ent);

				//posizione
				auto cpos = entRegistry.addComponent<ent::CompPos>(ent);
				cpos->reset();
				cpos->pos.set (xx, 0, zz);
				cpos->scale.set (SCALE, SCALE, SCALE);

				xx += ENTITY_GRID_X;

				//aggiungo uno script alla entity
				auto scriptable = entRegistry.addComponent<ent::CompScriptable>(ent);

				const u32 random_0_3 = gos::randomU32(3);
				switch (random_0_3 )
				{
				default:
				case 0:	scriptable->callback = Test1__entity_script_callback_0; break;
				case 1:	scriptable->callback = Test1__entity_script_callback_1; break;
				case 2:	scriptable->callback = Test1__entity_script_callback_2; break;
				case 3:	scriptable->callback = Test1__entity_script_callback_3; break;
				}

				//model-instance
				auto cModelInstance = entRegistry.addComponent<ent::CompModelInstance>(ent);
				engine->modelinst_create (handle_model, &cModelInstance->handle_mi);

				scene.add(ent);
			}

			zz += ENTITY_GRID_Z;
		}
	}
	scene.end();





	





    
	//renderizzo
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
        	mainLoop.run(); //questo lo chiamo per aggiornare il timer gfxJob in modo che il tempo di "GPU" sia printato con + accuratezza
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
				gos::gpu::CmdBufferWriter2 cw;
				cw	.begin (gpu, cmdBufferHandle)
					.setViewport (gpu->viewport_getDefault());

				renderer->begin(&cam);
				{
					scene.query (cam, &ent_uniqueList, true);

					ent_uniqueList.forEach ( [&entRegistry = entRegistry, &renderer = renderer](u32 index, gos::Entity ent) {
						auto cModelInstance = entRegistry.query<ent::CompModelInstance>(ent);
						renderer->add (cModelInstance);
						return true;
					});
				}
				renderer->end (cw);


				cw	.imageTransition (renderer->getHandle_rt0(), eImageLayout::color_attachment_optimal, eImageLayout::transfer_src)
					.imageTransition (swapchainImg.image, eImageLayout::undefined, eImageLayout::transfer_dst)
					.copyImageToImage (renderer->getHandle_rt0(), swapchainImg.image, gpu->swapChain_getImageExten2D(), gpu->swapChain_getImageExten2D())
					.imageTransition (swapchainImg.image, eImageLayout::transfer_dst, eImageLayout::presentation)
					.end();
			}
			mainLoop.stat_onCommandBufferEnd();


			mainLoop.gfxJob_submitAndPresent (cmdBufferHandle, swapchainImg);
        }		
	}


	//free
	gpu->waitIdle();
	mainLoop.unsetup();

	//free delle modelInstance
	entRegistry.getAllEntitiesWith<ent::CompModelInstance>()->forEach ([engine=this->engine](ent::CompModelInstance *comp, gos::Entity ent){
		engine->release (comp->handle_mi);
	});



	scene.unsetup();
	engine->release (handle_texBianca);
	engine->release (handle_texChecker);

	
	for (u8 i=0; i<4; i++)
		engine->release(handle_shape_list[i]);
	
	GOSDELETE(allocator, renderer);
	renderer = NULL;


	gpu->deleteResource (cmdBufferHandle);
	return true;
}

