#include "VulkanApp.h"

using namespace gos;

//************************************
VulkanApp::VulkanApp()
{ 
    gpu = NULL; 

    ///////////////////////
    inputMap.action_add ("global", "app_terminate");
    inputMap.action_add ("global", "toggle_fullscreen");
    inputMap.action_add ("global", "toggle_vsync");
    inputMap.action_add ("global", "show_all_actions");

    inputMap.action_add ("game", "toggle_mouse_mode");
    inputMap.action_add ("game", "move_forward");
    inputMap.action_add ("game", "move_backward");
    inputMap.action_add ("game", "strafe_left");
    inputMap.action_add ("game", "strafe_right");
    inputMap.action_add ("game", "strafe_up");
    inputMap.action_add ("game", "strafe_down");
    inputMap.action_add ("game", "rotateX");
    inputMap.action_add ("game", "rotateY");

    inputMap.action_bindToBtn ("global", "app_terminate", input::eOrigin::keyboard, GLFW_KEY_Q, input::eButtonStatus::pressed, input::sButtonModifier(input::eButtonModifier::LCTRL));
    inputMap.action_bindToBtn ("global", "app_terminate", input::eOrigin::window, GOS_BUTTON_WINDOW_CLOSE, input::eButtonStatus::pressed);
    inputMap.action_bindToBtn ("global", "toggle_fullscreen", input::eOrigin::keyboard, GLFW_KEY_ENTER, input::eButtonStatus::pressed, input::sButtonModifier(input::eButtonModifier::LALT));
    inputMap.action_bindToBtn ("global", "toggle_vsync", input::eOrigin::keyboard, GLFW_KEY_BACKSPACE, input::eButtonStatus::pressed, input::sButtonModifier(input::eButtonModifier::LALT));
    inputMap.action_bindToBtn ("global", "show_all_actions", input::eOrigin::keyboard, GLFW_KEY_F1, input::eButtonStatus::pressed, input::sButtonModifier(input::eButtonModifier::LCTRL, input::eButtonModifier::LSHIFT));
    
    inputMap.action_bindToBtn ("game", "toggle_mouse_mode", input::eOrigin::keyboard, GLFW_KEY_TAB, input::eButtonStatus::pressed);
    inputMap.action_bindToBtn ("game", "move_forward", input::eOrigin::keyboard, GLFW_KEY_W, input::eButtonStatus::both);
    inputMap.action_bindToBtn ("game", "move_backward", input::eOrigin::keyboard, GLFW_KEY_S, input::eButtonStatus::both);
    inputMap.action_bindToBtn ("game", "strafe_left", input::eOrigin::keyboard, GLFW_KEY_A, input::eButtonStatus::both);
    inputMap.action_bindToBtn ("game", "strafe_right", input::eOrigin::keyboard, GLFW_KEY_D, input::eButtonStatus::both);
    inputMap.action_bindToBtn ("game", "strafe_up", input::eOrigin::keyboard, GLFW_KEY_Q, input::eButtonStatus::both);
    inputMap.action_bindToBtn ("game", "strafe_down", input::eOrigin::keyboard, GLFW_KEY_Z, input::eButtonStatus::both);

    inputMap.action_bindToAxleREL ("game", "rotateX",  input::eOrigin::mouse, input::eAxle::y, input::eAxleDirection::both);
    inputMap.action_bindToAxleREL ("game", "rotateY",  input::eOrigin::mouse, input::eAxle::x, input::eAxleDirection::both);

}

//************************************
bool VulkanApp::init (gos::GPU *gpuIN, const char *title)
{
    gpu = gpuIN;

    gos::input::window_setTitle (gpu->getWindow(), title);
    gos::input::window_setUserPointer (gpu->getWindow(), this);
    
    if (!virtual_onInit())
        return false;


    gos::logger::log (eTextColor::white, "\n\n=======================================================\n");
    gos::logger::log (eTextColor::green, "%s\n", title);
    gos::logger::incIndent();
        virtual_explain();

        gos::logger::log ("\n");
        gos::logger::log (eTextColor::white, "ALT + ENTER = toggle fullscreen\n");
        gos::logger::log (eTextColor::white, "ALT + BACKSPACE = toggle vsync\n");
        gos::logger::log (eTextColor::white, "CTRL + SHIFT + F1 = show all key binding\n");
        gos::logger::log ("\n\n");
    gos::logger::decIndent();
    return true;
}    

//************************************
void VulkanApp::toggleVSync()
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
void VulkanApp::handleInput()
{
    gos::input::pollEvents();

    const gos::input::EvtList *evtList = gos::input::window_getEventList (gpu->getWindow());


    i16 value;
    inputMap.resolve_begin (evtList);
    while (1)
    {
        const u32 actionID = inputMap.resolve_getNextActionID(&value);
        if (0 == actionID)
            break;
        switch (actionID)
        {
        default:
            virtual_onInputEvent (actionID, value);
            break;

        case COMPILE_TIME_STR_CRC32("global.show_all_actions"):
            inputMap.logAllMappedInput();
            break;

        case COMPILE_TIME_STR_CRC32("global.app_terminate"):
            bQuitApp = true;
            break;

        case COMPILE_TIME_STR_CRC32("global.toggle_fullscreen"):
            this->toggleFullscreen();
            break;

        case COMPILE_TIME_STR_CRC32("global.toggle_vsync"):
            this->toggleVSync();
            break;

        case COMPILE_TIME_STR_CRC32("game.toggle_mouse_mode"):
            input::window_toggleMouseMode(gpu->getWindow());
            break;
        }
    }
  
}    
