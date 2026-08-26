#include "gosDefaultApp.h"

using namespace gos;
using namespace gos::engine;


//***************************************
DefaultApp::DefaultApp()
{
	allocator = gos::getSysHeapAllocator();
	engine = NULL;
	gpu = NULL;
	bShowCamPos = 0;
	bEnableAssetMonitor = false;
	ctrl_action.zero();

	for (u8 i=0; i<CAMERA__NUM_MAX; i++)
		cam_list[i].flag.zero();

	memset (&nav, 0, sizeof(nav));
	navigation__create_mode(NAV_MODE__DEFAULT_CAMERA);
}

//***************************************
DefaultApp::~DefaultApp()
{
}

//***************************************
geom::Camera3* DefaultApp::camera__create (u32 index, f32 fov_grad, f32 near_plane, f32 far_plane)
{
	assert (index < CAMERA__NUM_MAX);
	
	cam_list[index].flag.set (CameraInfo::FLAG_CREATED);

	cam_list[index].cam.set_perspective_FOV_LH(gpu->swapChain_calcAspectRatio(), fov_grad, near_plane, far_plane);
    cam_list[index].cam.pos.identity();
	cam_list[index].cam.mark_updated();

	return &cam_list[index].cam;
}

//***************************************
geom::Camera3* DefaultApp::camera__get (u32 index)
{
	assert (index < CAMERA__NUM_MAX);
	assert (cam_list[index].flag.isBitSet (CameraInfo::FLAG_CREATED));
	return &cam_list[index].cam;
}

//***************************************
void DefaultApp::navigation__create_mode (u8 mode_uid)
{
	const u8 n = nav.num++;
	assert (nav.num <= NAVIGATION__NUM_MAX_MODE);
	
	nav.uid[n] = mode_uid;
}

//***************************************
void DefaultApp::navigation__set_mode (u8 mode_uid)
{
	for (u8 i=0; i<nav.num; i++)
	{
		if (nav.uid[i] == mode_uid)
		{
			nav.current_index = i;

			logger::log ("navigation_mode: %d\n", mode_uid);
			on__navigation_mode_changed(mode_uid);

			if (NAV_MODE__DEFAULT_CAMERA == mode_uid)
			{
				camera__set_render_camera (0);
				ctrl_default_cam.set_linear_speed__m_sec (10);
				ctrl_default_cam.bind (&render_cam->pos);
				render_cam->mark_updated();
			}

			return;
		}
	}
	DBGBREAK;
}

//***************************************
void DefaultApp::run (gos::Engine *engineIN)
{
	engine = engineIN;
	gpu = engine->gpu;

	//input
	engine->inputCtx->
		action_add ("toggle_ctrl_mode")
		.action_add ("toggle_show_cam_pos");

	engine->inputCtx->action_bindToBtn ("toggle_ctrl_mode", input::eOrigin::keyboard, GLFW_KEY_TAB, input::eButtonStatus::pressed, input::sButtonModifier(input::eButtonModifier::LSHIFT));
	engine->inputCtx->action_bindToBtn ("toggle_show_cam_pos", input::eOrigin::keyboard, GLFW_KEY_C, input::eButtonStatus::pressed, input::sButtonModifier(input::eButtonModifier::LALT));

	//setup camera default
	camera__create (0, math::gradToRad(45), 0.1f, 250.0f);
	navigation__set_mode(0);

    priv_loop();
}

//**********************************
void DefaultApp::default_handle_input ()
{
	Engine::InputEvent ev;
	while (engine->inputEvent_getNext(&ev))
	{
		switch (ev.actionID)
		{
		default:
			on__handle_input(ev);
			break;

		case COMPILE_TIME_STR_CRC32("toggle_ctrl_mode"):
			nav.current_index++;
			if (nav.current_index >= nav.num)
				nav.current_index = 0;
			navigation__set_mode ( nav.uid[nav.current_index] );
			break;

		case COMPILE_TIME_STR_CRC32("toggle_show_cam_pos"):
			bShowCamPos = 1 - bShowCamPos;
			break;

		case COMPILE_TIME_STR_CRC32("move_forward"):
			if (gos::input::eButtonStatus::pressed == engine->inputEvent_getBtnStatus())
				ctrl_action.set (eCtrlAction::forward);
			else
				ctrl_action.clear (eCtrlAction::forward);
			break;

		case COMPILE_TIME_STR_CRC32("move_backward"):
			if (gos::input::eButtonStatus::pressed == engine->inputEvent_getBtnStatus())
				ctrl_action.set (eCtrlAction::backward);
			else
				ctrl_action.clear (eCtrlAction::backward);
			break;

		case COMPILE_TIME_STR_CRC32("strafe_left"):
			if (gos::input::eButtonStatus::pressed == engine->inputEvent_getBtnStatus())
				ctrl_action.set (eCtrlAction::strafe_left);
			else
				ctrl_action.clear (eCtrlAction::strafe_left);
			break;

		case COMPILE_TIME_STR_CRC32("strafe_right"):
			if (gos::input::eButtonStatus::pressed == engine->inputEvent_getBtnStatus())
				ctrl_action.set (eCtrlAction::strafe_right);
			else
				ctrl_action.clear (eCtrlAction::strafe_right);
			break;


		case COMPILE_TIME_STR_CRC32("rotateY"):
			if (ev.value < 0)
				ctrl_action.set (eCtrlAction::rot_y_clock, true);
			else
				ctrl_action.set (eCtrlAction::rot_y_counterclock, true);
			break;

		case COMPILE_TIME_STR_CRC32("rotateX"):
			if (ev.value < 0)
				ctrl_action.set (eCtrlAction::rot_x_clock, true);
			else
				ctrl_action.set (eCtrlAction::rot_x_counterclock, true);
			break;

		case COMPILE_TIME_STR_CRC32("strafe_up"):
			if (gos::input::eButtonStatus::pressed == engine->inputEvent_getBtnStatus())
				ctrl_action.set (eCtrlAction::strafe_up);
			else
				ctrl_action.clear (eCtrlAction::strafe_up);
			break;

		case COMPILE_TIME_STR_CRC32("strafe_down"):
			if (gos::input::eButtonStatus::pressed == engine->inputEvent_getBtnStatus())
				ctrl_action.set (eCtrlAction::strafe_down);
			else
				ctrl_action.clear (eCtrlAction::strafe_down);
			break;
			
		case COMPILE_TIME_STR_CRC32("zoom_in"):
			if (ev.value > 0)	ctrl_action.set (eCtrlAction::zoom_in, true);
			else ctrl_action.set (eCtrlAction::zoom_out, true);
			break;
		}
	}
}

//***************************************
void DefaultApp::priv_loop ()
{
	on__setup();

	//init dell'asset monitor se richiesto
	u64 next_time_check_assetMon__msec = 0;
	asset2::MonitorClient assetMon;
	if (bEnableAssetMonitor)
	{
		if (!assetMon.connect())
		{
			logger::err ("unable to connecto to asset monitor\n");
		}
	}
	else
		next_time_check_assetMon__msec = u64MAX;

    gpu::MainLoop2 mainLoop;
    mainLoop.setup (gpu);
	mainLoop.stat_setPrintReportEvery (5000);

    GPUCmdBufferHandle  cmdBufferHandle;
    gpu->cmdBuffer_create (eGPUQueueFamily::gfx, &cmdBufferHandle);
	
	bool bQuit = false;
	vec3f last_cam_pos;
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
			const u64 timenow_msec = gos::getTimeSinceStart_msec();
			
			//ogni tot verifico se ho ricevuto notifiche dall'asset monitor
			if (timenow_msec >= next_time_check_assetMon__msec)
			{
				asset2::UID uid;
				while ( assetMon.read(&uid) )
				{
					engine->asset_hotreload (uid);
				}

				next_time_check_assetMon__msec = timenow_msec + 500;
			}

			default_handle_input();

			//gestione del movimento della camera di default
			if (0 == navigation__get_mode())
			{
				ctrl_default_cam.update (gos::getTimeSinceStart_msec(), ctrl_action);
				render_cam->mark_updated();
			}			

			//virtual callback
			on__update(timenow_msec);

			if (last_cam_pos != render_cam->pos.o)
			{
				last_cam_pos = render_cam->pos.o;
				if (bShowCamPos)
					logger::log (eTextColor::white, "CAM: %.2f, %.2f, %.2f\n", last_cam_pos.x, last_cam_pos.y, last_cam_pos.z);
			}

			
        }
		mainLoop.stat_onCPUFrameEnd();		


		//rendering
        if (gpu->swapChain_wasRecreated())
		{
			for (u8 i=0; i<CAMERA__NUM_MAX; i++)
			{
				if (cam_list[i].flag.isBitSet(CameraInfo::FLAG_CREATED))
					cam_list[i].cam.change_aspectRatio_perspective_FOV_LH (gpu->swapChain_calcAspectRatio());
			}					
		}

        //se il job precedente e' stato presentato, posso schedularne uno nuovo
        gpu::SwapchainImg swapchainImg;
        if (mainLoop.gfxJob_canSubmit(&swapchainImg))
        {
			mainLoop.stat_onCommandBufferBegin();
			{
				on__render();
				engine->renderPipe.render (swapchainImg, cmdBufferHandle, render_cam);
			}
			mainLoop.stat_onCommandBufferEnd();
			mainLoop.gfxJob_submitAndPresent (cmdBufferHandle, swapchainImg);
        }
	}

	if (bEnableAssetMonitor)
		assetMon.disconnect();

	//free
	gpu->waitIdle();
	mainLoop.unsetup();
	gpu->deleteResource (cmdBufferHandle);

	on__unsetup();
}

