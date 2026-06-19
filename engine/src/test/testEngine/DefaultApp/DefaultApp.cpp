#include "DefaultApp.h"
#include "gosShapePrefabs.h"
#include "gosGeomUtils.h"

using namespace gos;



//***************************************
DefaultApp::DefaultApp()
{
	allocator = gos::getSysHeapAllocator();
	camera_mode = eCameraMode::move_free;
}

//***************************************
DefaultApp::~DefaultApp()
{
}

//***************************************
void DefaultApp::run (gos::Engine *engineIN)
{
	engine = engineIN;
	gpu = engine->gpu;

	//input
	engine->inputCtx->
		action_add ("toggle_cam_mode");

	engine->inputCtx->action_bindToBtn ("toggle_cam_mode", input::eOrigin::keyboard, GLFW_KEY_TAB, input::eButtonStatus::pressed, input::sButtonModifier(input::eButtonModifier::LSHIFT));

	//setup camera
    cam.setPerspectiveFovLH(gpu->swapChain_calcAspectRatio(),  math::gradToRad(45), 0.1f, 250.0f);
    cam.pos.identity();
    cam.pos.warp (0, 15.0f, -10);
	cam.pos.lookAt (vec3f(0,0,0));
	cam.markUpdated();

	//e movement
    move_fps.bind (&cam.pos);
	move_free.bind (&cam.pos);
	

	default_load_material();
    priv_loop();
}

//**********************************
const char* DefaultApp::enum_to_string (eCameraMode m) const
{
	switch (m)
	{
	default:						return "ERR, camMode::unknown";
	case eCameraMode::move_free:	return "free";
	case eCameraMode::move_fps:		return "FPS";
	}
}

//**********************************
bool DefaultApp::default_load_material()
{
    return true;
}

//**********************************
void DefaultApp::default_handle_input ()
{
    const u64 timeNow_msec = gos::getTimeSinceStart_msec();

	Engine::InputEvent ev;
	while (engine->inputEvent_getNext(&ev))
	{
		switch (ev.actionID)
		{
		case COMPILE_TIME_STR_CRC32("toggle_cam_mode"):
			if (eCameraMode::move_free == camera_mode)
				camera_mode = eCameraMode::move_fps;
			else
				camera_mode = eCameraMode::move_free;

			logger::log ("Cam mode: %s\n", enum_to_string(camera_mode));
			break;

		case COMPILE_TIME_STR_CRC32("move_forward"):
			if (eCameraMode::move_free == camera_mode)
				move_free.moveForward ((ev.value == 1));
			else
				move_fps.moveForward ((ev.value == 1));
			break;

		case COMPILE_TIME_STR_CRC32("move_backward"):
			if (eCameraMode::move_free == camera_mode)
				move_free.moveBackward ((ev.value == 1));
			else
				move_fps.moveBackward ((ev.value == 1));
			break;

		case COMPILE_TIME_STR_CRC32("strafe_left"):
			if (eCameraMode::move_free == camera_mode)
				move_free.strafeLeft ((ev.value == 1));  
			else
				move_fps.strafeLeft ((ev.value == 1));  
			break;

		case COMPILE_TIME_STR_CRC32("strafe_right"):
			if (eCameraMode::move_free == camera_mode)
				move_free.strafeRight ((ev.value == 1));
			else
				move_fps.strafeRight ((ev.value == 1));
			break;

		case COMPILE_TIME_STR_CRC32("rotateY"):
			if (eCameraMode::move_free == camera_mode)
				move_free.rotateY ((ev.value < 0));
			else
				move_fps.mouseRotateY (ev.value);
			break;

		case COMPILE_TIME_STR_CRC32("rotateX"):
			if (eCameraMode::move_free == camera_mode)
				move_free.rotateX ((ev.value < 0));
			else
				move_fps.mouseRotateX (ev.value);
			break;

		case COMPILE_TIME_STR_CRC32("strafe_up"):
			if (eCameraMode::move_free == camera_mode)
				move_free.strafeUp ((ev.value == 1));
			else
				move_fps.strafeUp ((ev.value == 1));
			break;

		case COMPILE_TIME_STR_CRC32("strafe_down"):
			if (eCameraMode::move_free == camera_mode)
				move_free.strafeDown ((ev.value == 1));
			else
				move_fps.strafeDown ((ev.value == 1));
			break;

		}
	}

    //gestione del movimento
	if (eCameraMode::move_free == camera_mode)
		move_free.update (timeNow_msec);
	else
		move_fps.update (timeNow_msec);
		

    cam.markUpdated();
}

//***************************************
void DefaultApp::priv_loop ()
{
	on__load_assets();

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

		mainLoop.run();

        //CPU jobs
		mainLoop.stat_onCPUFrameBegin();
		{
			default_handle_input();
			on__handle_input();
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
				on__render(swapchainImg, cmdBufferHandle, &cam);
			}
			mainLoop.stat_onCommandBufferEnd();
			mainLoop.gfxJob_submitAndPresent (cmdBufferHandle, swapchainImg);
        }
	}

	//free
	gpu->waitIdle();
	mainLoop.unsetup();
	gpu->deleteResource (cmdBufferHandle);

	engine->release (handle_texBianca);
	on__cleanup();
}

