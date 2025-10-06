#include "gosEngine.h"

using namespace gos;

//******************************** 
Engine::Engine()
{
    gpu = NULL;
    inputCtx = NULL;
    assetHub = NULL;
    bQuitEngine = false;
}

//******************************** 
Engine::~Engine()
{
    if (NULL == gpu)
        return;

    GOSWinHandle mainWin = gpu->getWindow();
    
    GOSDELETE(gos::getSysHeapAllocator(), assetHub);
    assetHub = NULL;

    gpu->deinit();
    GOSDELETE(gos::getSysHeapAllocator(), gpu);
    gpu = NULL;

    GOSDELETE(gos::getSysHeapAllocator(), inputCtx);
    inputCtx = NULL;

    gos::input::window_destroy (mainWin);
    gos::input::deinit();
}

//******************************** 
bool Engine::setup (u32 mainWin_w, u32 mainWin_h, const char *mainWin_title)
{
    if (!gos::input::init())
    {
        logger::err ("Engine::setup() => input::init failed\n");
        return false;
    }

    //main win
    GOSWinHandle mainWin;
    if (!gos::input::window_create (1024, 768, gos::getAppName(), &mainWin))
    {
        logger::err ("Engine::setup() => input::window_create failed\n");
        return false;
    }
    gos::input::window_setTitle (mainWin, mainWin_title);

    //GPU
    gpu = GOSNEW(gos::getSysHeapAllocator(), gos::GPU)();
    if (!gpu->init (mainWin, false))
    {
        logger::err ("Engine::setup() => gpu->init() failed\n");
        return false;
    }

    //assetHub
    assetHub = GOSNEW(gos::getSysHeapAllocator(), asset::Hub)();
    assetHub->setup ("@w/data", gpu);

    //creo l'input context di default
    inputCtx = GOSNEW(gos::getSysHeapAllocator(), input::Context)("global");
    {
        
        inputCtx->action_add ("app_terminate")
            .action_add ("toggle_fullscreen")
            .action_add ("toggle_vsync")
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


        inputCtx->action_bindToBtn ("app_terminate", input::eOrigin::keyboard, GLFW_KEY_Q, input::eButtonStatus::pressed, input::sButtonModifier(input::eButtonModifier::LCTRL));
        inputCtx->action_bindToBtn ("app_terminate", input::eOrigin::window, GOS_BUTTON_WINDOW_CLOSE, input::eButtonStatus::pressed);
        inputCtx->action_bindToBtn ("toggle_fullscreen", input::eOrigin::keyboard, GLFW_KEY_ENTER, input::eButtonStatus::pressed, input::sButtonModifier(input::eButtonModifier::LALT));
        inputCtx->action_bindToBtn ("toggle_vsync", input::eOrigin::keyboard, GLFW_KEY_BACKSPACE, input::eButtonStatus::pressed, input::sButtonModifier(input::eButtonModifier::LALT));
        inputCtx->action_bindToBtn ("show_all_actions", input::eOrigin::keyboard, GLFW_KEY_F1, input::eButtonStatus::pressed, input::sButtonModifier(input::eButtonModifier::LCTRL, input::eButtonModifier::LSHIFT));
        
        inputCtx->action_bindToBtn ("toggle_mouse_mode", input::eOrigin::keyboard, GLFW_KEY_TAB, input::eButtonStatus::pressed);
        inputCtx->action_bindToBtn ("move_forward", input::eOrigin::keyboard, GLFW_KEY_W, input::eButtonStatus::both);
        inputCtx->action_bindToBtn ("move_backward", input::eOrigin::keyboard, GLFW_KEY_S, input::eButtonStatus::both);
        inputCtx->action_bindToBtn ("strafe_left", input::eOrigin::keyboard, GLFW_KEY_A, input::eButtonStatus::both);
        inputCtx->action_bindToBtn ("strafe_right", input::eOrigin::keyboard, GLFW_KEY_D, input::eButtonStatus::both);
        inputCtx->action_bindToBtn ("strafe_up", input::eOrigin::keyboard, GLFW_KEY_Q, input::eButtonStatus::both);
        inputCtx->action_bindToBtn ("strafe_down", input::eOrigin::keyboard, GLFW_KEY_Z, input::eButtonStatus::both);

        inputCtx->action_bindToAxleREL ("rotateX",  input::eOrigin::mouse, input::eAxle::y, input::eAxleDirection::both);
        inputCtx->action_bindToAxleREL ("rotateY",  input::eOrigin::mouse, input::eAxle::x, input::eAxleDirection::both);    
    }

    return true;
}

//************************************
void Engine::toggleVSync()
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
void Engine::priv_handleInput()
{
    gos::input::pollEvents();

    input::ResolvedEvtList evtList;
    input::resolveEvents (gpu->getWindow(), inputCtx, &evtList);

    i16 value;
    while (1)
    {
        const u32 actionID = evtList.nextActionID(&value);
        if (0 == actionID)
            break;
        switch (actionID)
        {
        default:
            //virtual_onInputEvent (actionID, value, evtList.getMouseStatus(), evtList.getBtnModifier());
            break;

        case COMPILE_TIME_STR_CRC32("show_all_actions"):
            inputCtx->logAllMappedInput();
            break;

        case COMPILE_TIME_STR_CRC32("app_terminate"):
            bQuitEngine = true;
            break;

        case COMPILE_TIME_STR_CRC32("toggle_fullscreen"):
            this->toggleFullscreen();
            break;

        case COMPILE_TIME_STR_CRC32("toggle_vsync"):
            this->toggleVSync();
            break;

        case COMPILE_TIME_STR_CRC32("toggle_mouse_mode"):
            input::window_toggleMouseMode(gpu->getWindow());
            break;
        }
    }
}


//******************************** 
bool Engine::run()
{
    if (bQuitEngine)
        return false;

    priv_handleInput();
    return true;
}