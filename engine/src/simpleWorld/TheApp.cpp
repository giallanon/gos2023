#include "TheApp.h"

using namespace gos;


//********************************
TheApp::TheApp()
{
    gpu = NULL;
}

//********************************
TheApp::~TheApp()
{
}

//********************************
bool TheApp::setup (gos::GPU *gpuIN)
{
    gpu = gpuIN;

    //fs alias
    fs::addAlias ("@shader", "shader", eAliasPathMode::relativeToAppFolder);

    //input mapping
    inputCtx.action_add ("quit")
        .action_add ("toggleFullscreen")
        .action_add ("toggleVSync")
        .action_add ("show_all_actions")

        .action_add ("toggle_mouse_mode")
        .action_add ("move_forward")
        .action_add ("move_backward")
        .action_add ("strafe_left")
        .action_add ("strafe_right")
        .action_add ("strafe_up")
        .action_add ("strafe_down")
        .action_add ("rotateX")
        .action_add ("rotateY");


    inputCtx.action_bindToBtn ("quit", input::eOrigin::keyboard, GLFW_KEY_Q, input::eButtonStatus::pressed, input::sButtonModifier(input::eButtonModifier::LCTRL));
    inputCtx.action_bindToBtn ("quit", input::eOrigin::window, GOS_BUTTON_WINDOW_CLOSE, input::eButtonStatus::pressed);
    inputCtx.action_bindToBtn ("toggleFullscreen", input::eOrigin::keyboard, GLFW_KEY_ENTER, input::eButtonStatus::pressed, input::sButtonModifier(input::eButtonModifier::LALT));
    inputCtx.action_bindToBtn ("toggleVSync", input::eOrigin::keyboard, GLFW_KEY_BACKSPACE, input::eButtonStatus::pressed, input::sButtonModifier(input::eButtonModifier::LALT));
    inputCtx.action_bindToBtn ("show_all_actions", input::eOrigin::keyboard, GLFW_KEY_F1, input::eButtonStatus::pressed, input::sButtonModifier(input::eButtonModifier::LCTRL, input::eButtonModifier::LSHIFT));

    inputCtx.action_bindToBtn ("toggle_mouse_mode", input::eOrigin::keyboard, GLFW_KEY_TAB, input::eButtonStatus::pressed);
    inputCtx.action_bindToBtn ("move_forward", input::eOrigin::keyboard, GLFW_KEY_W, input::eButtonStatus::both);
    inputCtx.action_bindToBtn ("move_backward", input::eOrigin::keyboard, GLFW_KEY_S, input::eButtonStatus::both);
    inputCtx.action_bindToBtn ("strafe_left", input::eOrigin::keyboard, GLFW_KEY_A, input::eButtonStatus::both);
    inputCtx.action_bindToBtn ("strafe_right", input::eOrigin::keyboard, GLFW_KEY_D, input::eButtonStatus::both);
    inputCtx.action_bindToBtn ("strafe_up", input::eOrigin::keyboard, GLFW_KEY_Q, input::eButtonStatus::both);
    inputCtx.action_bindToBtn ("strafe_down", input::eOrigin::keyboard, GLFW_KEY_Z, input::eButtonStatus::both);

    inputCtx.action_bindToAxleREL ("rotateX",  input::eOrigin::mouse, input::eAxle::y, input::eAxleDirection::both);
    inputCtx.action_bindToAxleREL ("rotateY",  input::eOrigin::mouse, input::eAxle::x, input::eAxleDirection::both);

    if (!renderer.setup(gpu))
        return false;
    
    return true;
}


//************************************
void TheApp::priv_toggleVSync()
{ 
    if (gpu->vsync_isEnabled())
    {
        gpu->vsync_enable (false);
        gos::logger::log (eTextColor::yellow, "VSYNC: off\n");
    }
    else
    {
        gpu->vsync_enable (true);
        gos::logger::log (eTextColor::yellow, "VSYNC: on\n");
    }
}

//************************************
void TheApp::priv_handleInput()
{
    gos::input::pollEvents();

    input::ResolvedEvtList evtList;
    input::resolveEvents (gpu->getWindow(), &inputCtx, &evtList);

    i16 value;
    while (1)
    {
        const u32 actionID = evtList.nextActionID(&value);
        if (0 == actionID)
            break;
        switch (actionID)
        {
        default:
            break;

        case COMPILE_TIME_STR_CRC32("quit"):
            bQuitApp = true;
            break;

        case COMPILE_TIME_STR_CRC32("toggleFullscreen"):
            gpu->toggleFullscreen();
            break;

        case COMPILE_TIME_STR_CRC32("toggleVSync"):
            this->priv_toggleVSync();
            break;

        case COMPILE_TIME_STR_CRC32("show_all_actions"):
            inputCtx.logAllMappedInput();
            break;

        case COMPILE_TIME_STR_CRC32("toggle_mouse_mode"):
            input::window_toggleMouseMode(gpu->getWindow());
            break;

        case COMPILE_TIME_STR_CRC32("move_forward"):           movement.moveForward ((value == 1));break;
        case COMPILE_TIME_STR_CRC32("move_backward"):          movement.moveBackward ((value == 1));    break;
        case COMPILE_TIME_STR_CRC32("strafe_left"):            movement.strafeLeft ((value == 1));    break;
        case COMPILE_TIME_STR_CRC32("strafe_right"):           movement.strafeRight ((value == 1));    break;
        case COMPILE_TIME_STR_CRC32("strafe_up"):              movement.strafeUp ((value == 1));    break;
        case COMPILE_TIME_STR_CRC32("strafe_down"):            movement.strafeDown ((value == 1));    break;
        case COMPILE_TIME_STR_CRC32("rotateY"):                movement.rotateY ((value<0)); break;
        case COMPILE_TIME_STR_CRC32("rotateX"):                movement.rotateX ((value<0)); break;

        case COMPILE_TIME_STR_CRC32("mouse_move"):
        case COMPILE_TIME_STR_CRC32("LMB"):
        case COMPILE_TIME_STR_CRC32("RMB"):
            break;
        }
    }
}    

//**********************************
void TheApp::priv_doCPUStuff ()
{
    priv_handleInput();

    //gestione del movimento
    const u64 timeNow_msec = gos::getTimeSinceStart_msec();
    movement.update(timeNow_msec);
    cam.markUpdated();
}


//********************************
void TheApp::run()
{
    //posizione inziale della camera
    cam.setPerspectiveFovLH (gpu->swapChain_calcAspectRatio(),  math::gradToRad(45), 0.1f, 50.0f);
    //cam.pos.identity(); cam.pos.warp (0, 0, -19); cam.pos.lookAt (vec3f(0,0,0));
    cam.pos.identity(); cam.pos.warp (0, 30, 0); cam.pos.rotateMeAboutMyX (-math::PIMEZZI);
    cam.markUpdated();
    movement.bind (&cam.pos);



    //command buffer 
    GPUCmdBufferHandle  cmdBufferHandle;
    gpu->cmdBuffer_create (eGPUQueueType::gfx, &cmdBufferHandle);
    gos::gpu::CmdBufferWriter cw;



    //main loop
    gpu::MainLoop gpuLoop;
    gpuLoop.setup (gpu);
    bQuitApp = false;
    while (bQuitApp == false)
    {
        //cpu stuff & input handling
        gpuLoop.stat_onCPUFrameBegin();
        priv_doCPUStuff();
        gpuLoop.stat_onCPUFrameEnd();

        gpuLoop.stat_printReport();


        gpuLoop.run ();
        if (gpuLoop.swapchainRecreated())
            cam.changeAspectRatioPerspectiveFovLH (gpu->swapChain_calcAspectRatio());
        if (gpuLoop.canSubmitGFXJob())
        {
            cw.begin (gpu, cmdBufferHandle);
                renderer.recordCommandBuffer(cw, &cam);
            cw.end();
            gpuLoop.submitGFXJob (cmdBufferHandle);
        }

    }
    
    //aspetto che GPU abbia finito tutto cio' che ha in coda
    gpu->waitIdle();

    //free
    gpu->deleteResource (cmdBufferHandle);
    gpuLoop.unsetup();
}
