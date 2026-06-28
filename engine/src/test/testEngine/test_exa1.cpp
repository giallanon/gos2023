#include "test_exa1.h"
#include "gosGeomUtils.h"

using namespace gos;


//***************************************
Test_exa1::Test_exa1()
{
	allocator = gos::getSysHeapAllocator();
	renderer_line3d = NULL;
}

//***************************************
Test_exa1::~Test_exa1()
{
	gpu->deleteResource (handle_rt0);
	gpu->deleteResource (handle_zb);

}

//***************************************
void Test_exa1::run (gos::Engine *engineIN)
{
	engine = engineIN;
	gpu = engine->gpu;

	//input
	engine->inputCtx->
		action_add ("select_edge_to_remove")
		.action_add ("try_remove_edge")
		.action_add ("simplify_90")
		.action_add ("subdivide")
		.action_add ("restart")
		.action_add ("relax")
		.action_add ("fast_create");
		// .action_add ("mouse-LB")
		// .action_add ("toggle_cam_mode");

	//engine->inputCtx->action_bindToAxleREL ("mouse-wheel", input::eOrigin::mouse, input::eAxle::z, input::eAxleDirection::both);
	// engine->inputCtx->action_bindToBtn ("mouse-LB", input::eOrigin::mouse, 0, input::eButtonStatus::pressed);
	engine->inputCtx->action_bindToBtn ("select_edge_to_remove", input::eOrigin::keyboard, GLFW_KEY_SPACE, input::eButtonStatus::pressed);
	engine->inputCtx->action_bindToBtn ("try_remove_edge", input::eOrigin::keyboard, GLFW_KEY_ENTER, input::eButtonStatus::pressed);
	engine->inputCtx->action_bindToBtn ("simplify_90", input::eOrigin::keyboard, GLFW_KEY_F1, input::eButtonStatus::pressed);
	engine->inputCtx->action_bindToBtn ("subdivide", input::eOrigin::keyboard, GLFW_KEY_F2, input::eButtonStatus::pressed);
	engine->inputCtx->action_bindToBtn ("restart", input::eOrigin::keyboard, GLFW_KEY_F3, input::eButtonStatus::pressed);
	engine->inputCtx->action_bindToBtn ("relax", input::eOrigin::keyboard, GLFW_KEY_F4, input::eButtonStatus::pressed);
	engine->inputCtx->action_bindToBtn ("fast_create", input::eOrigin::keyboard, GLFW_KEY_F5, input::eButtonStatus::pressed);
	

	//render target & zbuffer
	gpu->renderTarget_create ("100%", "100%", eImageFormat::U8_RGBA, &handle_rt0);
	gpu->zbuffer_create ("0-", "0-", eImageFormat::_DEPTH_BEST, &handle_zb);


	//renderer
	renderer_line3d = engine->renderPipe.add_renderer<engine::Renderer_line3d>();
		line_ctx1 = renderer_line3d->ctx__crete_new ("ctx1", 512);
		line_ctx2 = renderer_line3d->ctx__crete_new ("ctx2", 16);
		line_ctx3 = renderer_line3d->ctx__crete_new ("ctx3", 16);



	//setup camera
    cam.setPerspectiveFovLH(gpu->swapChain_calcAspectRatio(),  math::gradToRad(45), 0.1f, 250.0f);
    cam.pos.identity();
    cam.pos.warp (0, 0, -10);
	cam.pos.lookAt (vec3f(0,0,0));
	cam.markUpdated();

	//movement
    movement.bind (&cam.pos);

    priv_loop();
}

//**********************************
void Test_exa1::doCPUStuff ()
{
    const u64 timeNow_msec = gos::getTimeSinceStart_msec();

	Engine::InputEvent ev;
	while (engine->inputEvent_getNext(&ev))
	{
		switch (ev.actionID)
		{
		case COMPILE_TIME_STR_CRC32("move_forward"):
            movement.moveForward ((ev.value == 1));
			break;

		case COMPILE_TIME_STR_CRC32("move_backward"):
            movement.moveBackward ((ev.value == 1));
			break;

		case COMPILE_TIME_STR_CRC32("strafe_left"):
			movement.strafeLeft ((ev.value == 1));  
			break;

		case COMPILE_TIME_STR_CRC32("strafe_right"):
			movement.strafeRight ((ev.value == 1));
			break;

		case COMPILE_TIME_STR_CRC32("rotateY"):
			movement.rotateY ((ev.value < 0));
			break;

		case COMPILE_TIME_STR_CRC32("rotateX"):
			movement.rotateX ((ev.value < 0));
			break;

		case COMPILE_TIME_STR_CRC32("strafe_up"):
			movement.strafeUp ((ev.value == 1));
			break;

		case COMPILE_TIME_STR_CRC32("strafe_down"):
			movement.strafeDown ((ev.value == 1));
			break;

		case COMPILE_TIME_STR_CRC32("select_edge_to_remove"):
			{
				exagen.select_edge_to_remove(&edge_to_remove);

				line_ctx2->clear();
				line_ctx2->set_color_ARGB (0xFF0000FF);
				line_ctx2->set_line_width(4);
				line_ctx2->line (exagen.vtxList(edge_to_remove.edge_vtx0), exagen.vtxList(edge_to_remove.edge_vtx1));


				const vec3f c = calc_tris_center(edge_to_remove.tris_index);
				const vec3f cix = vec3f(0.1f, 0, 0);
				const vec3f ciy = vec3f(0, 0.1f, 0);

				line_ctx2->set_line_width(2);
				line_ctx2->line (c, c + cix);
				line_ctx2->line (c, c - cix);
				line_ctx2->line (c, c + ciy);
				line_ctx2->line (c, c - ciy);
			}
			break;

		case COMPILE_TIME_STR_CRC32("try_remove_edge"):
			{
				exagen.try_remove_edge(edge_to_remove);
				build_line_ctx();
			}
			break;

		case COMPILE_TIME_STR_CRC32("simplify_90"):
			exagen.simplify_90();
			build_line_ctx();
			break;

		case COMPILE_TIME_STR_CRC32("subdivide"):
			line_ctx2->clear();
			line_ctx3->clear();
			exagen.subdivide();
			build_line_ctx();
			break;

		case COMPILE_TIME_STR_CRC32("restart"):
			exagen.create_default_exa();
			line_ctx1->clear();
			line_ctx2->clear();
			line_ctx3->clear();
			build_line_ctx();
			break;			

		case COMPILE_TIME_STR_CRC32("relax"):
			for (u32 i=0;i<10; i++)
			{
				exagen.relax();
				build_line_ctx();
			}
			break;

		case COMPILE_TIME_STR_CRC32("fast_create"):
			exagen.build();
			build_line_ctx();
			break;

		}
	}

	movement.update(timeNow_msec);
    cam.markUpdated();
}

//***************************************
vec3f Test_exa1::calc_tris_center(u32 tris_index) const
{
	const vec3f v0 = exagen.vtxList(exagen.trisList(tris_index).vtx_idx0);
	const vec3f v1 = exagen.vtxList(exagen.trisList(tris_index).vtx_idx1);
	const vec3f v2 = exagen.vtxList(exagen.trisList(tris_index).vtx_idx2);

	vec3f ret = (v0 + v1 + v2) / 3.0f;
	return ret;
}


//***************************************
void Test_exa1::build_line_ctx ()
{
	line_ctx1->clear();

	exagen.vtxList.forEach( [&line_ctx1=this->line_ctx1](u32 index, const vec3f &v){
		line_ctx1->vtx_add(v);
		return true;
	});

	
	line_ctx1->set_line_width(2);
	exagen.trisList.forEach ( [&line_ctx1=this->line_ctx1, &vtxList=this->exagen.vtxList](u32 index, const ExaGenerator::sTris &tris){
		line_ctx1->line_begin();
		line_ctx1->line_add_vtx (tris.vtx_idx0);
		line_ctx1->line_add_vtx (tris.vtx_idx1);
		line_ctx1->line_add_vtx (tris.vtx_idx2);
		line_ctx1->line_add_vtx (tris.vtx_idx0);
		line_ctx1->line_end();
		return true;
	});

	line_ctx1->set_line_width(3);
	line_ctx1->set_color_ARGB (0xFF00FF00);
	exagen.quadList.forEach ( [&line_ctx1=this->line_ctx1, &vtxList=this->exagen.vtxList](u32 index, const ExaGenerator::sQuad &quad){
		line_ctx1->line_begin();
		line_ctx1->line_add_vtx (quad.vtx_idx0);
		line_ctx1->line_add_vtx (quad.vtx_idx1);
		line_ctx1->line_add_vtx (quad.vtx_idx2);
		line_ctx1->line_add_vtx (quad.vtx_idx3);
		line_ctx1->line_add_vtx (quad.vtx_idx0);
		line_ctx1->line_end();
		return true;
	});


	line_ctx1->point_set_radius(10);
	line_ctx1->set_color_ARGB (0xFFFFFF00);
	exagen.listOfBorderVtxIndex.forEach( [&line_ctx1=this->line_ctx1, &vtxList=this->exagen.vtxList](u32 index, const u32 vtx_index)
	{
		line_ctx1->point (vtx_index);
		return true;
	});

	


	logger::log (eTextColor::white, "cur graph: vtx=%d, tris=%d, quad=%d\n", exagen.vtxList.getNElem(), exagen.trisList.getNElem(), exagen.quadList.getNElem());
}

//***************************************
void Test_exa1::priv_loop ()
{
	exagen.create_default_exa();
	build_line_ctx();


	logger::log (eTextColor::magenta, "num vtx=%d, num tris=%d\n", exagen.vtxList.getNElem(), exagen.trisList.getNElem());


    //loop
    gpu::MainLoop2 mainLoop;
    mainLoop.setup (gpu);
	mainLoop.stat_setPrintReportEvery(u32MAX);

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

		mainLoop.run();

        //CPU jobs
		mainLoop.stat_onCPUFrameBegin();
		{
			doCPUStuff();
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
				engine->renderPipe.render (swapchainImg, cmdBufferHandle, &cam);
			}
			mainLoop.stat_onCommandBufferEnd();
			mainLoop.gfxJob_submitAndPresent (cmdBufferHandle, swapchainImg);
        }		
	}

	//free
	gpu->waitIdle();
	mainLoop.unsetup();

	gpu->deleteResource (cmdBufferHandle);
}

