#include "VulkanApp.h"

using namespace gos;

//************************************
void GLFW_kb_key_callback (GLFWwindow* window, int key, UNUSED_PARAM(int scancode), int action, int mods)
{
    VulkanApp *app = reinterpret_cast<VulkanApp*> (glfwGetWindowUserPointer(window));


    char kname[32];
#define MACRO_HELPME(glDEFINE_senza_GLFW)\
    case GLFW_##glDEFINE_senza_GLFW: sprintf_s (kname, sizeof(kname), #glDEFINE_senza_GLFW); break;

    switch (key)
    {
    default:
        {
            const char *key_name = glfwGetKeyName (key, scancode);
            if (NULL != key_name)
                strcpy_s (kname, sizeof(kname), key_name);
            else
                sprintf_s (kname, sizeof(kname), "??KEY_NONAME??"); 
        }
        break;

    case GLFW_KEY_ESCAPE:    sprintf_s (kname, sizeof(kname), "KEY_ESCAPE"); break;
    MACRO_HELPME(KEY_ENTER)
    MACRO_HELPME(KEY_TAB)
    MACRO_HELPME(KEY_BACKSPACE)
    MACRO_HELPME(KEY_INSERT)
    MACRO_HELPME(KEY_DELETE)
    MACRO_HELPME(KEY_RIGHT)
    MACRO_HELPME(KEY_LEFT)
    MACRO_HELPME(KEY_DOWN)
    MACRO_HELPME(KEY_UP)
    MACRO_HELPME(KEY_PAGE_UP)
    MACRO_HELPME(KEY_PAGE_DOWN)
    MACRO_HELPME(KEY_HOME)
    MACRO_HELPME(KEY_END)
    MACRO_HELPME(KEY_CAPS_LOCK)
    MACRO_HELPME(KEY_SCROLL_LOCK)
    MACRO_HELPME(KEY_NUM_LOCK)
    MACRO_HELPME(KEY_PRINT_SCREEN)
    MACRO_HELPME(KEY_PAUSE)
    MACRO_HELPME(KEY_F1)
    MACRO_HELPME(KEY_F2)
    MACRO_HELPME(KEY_F3)
    MACRO_HELPME(KEY_F4)
    MACRO_HELPME(KEY_F5)
    MACRO_HELPME(KEY_F6)
    MACRO_HELPME(KEY_F7)
    MACRO_HELPME(KEY_F8)
    MACRO_HELPME(KEY_F9)
    MACRO_HELPME(KEY_F10)
    MACRO_HELPME(KEY_F11)
    MACRO_HELPME(KEY_F12)
    MACRO_HELPME(KEY_KP_0)
    MACRO_HELPME(KEY_KP_1)
    MACRO_HELPME(KEY_KP_2)
    MACRO_HELPME(KEY_KP_3)
    MACRO_HELPME(KEY_KP_4)
    MACRO_HELPME(KEY_KP_5)
    MACRO_HELPME(KEY_KP_6)
    MACRO_HELPME(KEY_KP_7)
    MACRO_HELPME(KEY_KP_8)
    MACRO_HELPME(KEY_KP_9)
    MACRO_HELPME(KEY_KP_DECIMAL)
    MACRO_HELPME(KEY_KP_DIVIDE)
    MACRO_HELPME(KEY_KP_MULTIPLY)
    MACRO_HELPME(KEY_KP_SUBTRACT)
    MACRO_HELPME(KEY_KP_ADD)
    MACRO_HELPME(KEY_KP_ENTER)
    MACRO_HELPME(KEY_KP_EQUAL)
    MACRO_HELPME(KEY_LEFT_SHIFT)
    MACRO_HELPME(KEY_LEFT_CONTROL)
    MACRO_HELPME(KEY_LEFT_ALT)
    MACRO_HELPME(KEY_LEFT_SUPER)
    MACRO_HELPME(KEY_RIGHT_SHIFT)
    MACRO_HELPME(KEY_RIGHT_CONTROL)
    MACRO_HELPME(KEY_RIGHT_ALT)
    MACRO_HELPME(KEY_RIGHT_SUPER)
    MACRO_HELPME(KEY_MENU)
    }
#undef MACRO_HELPME
    
    
    if (GLFW_RELEASE == action)
        printf ("KEYB: (%d,%d) %s released\n", key, scancode, kname);
    else if (GLFW_PRESS == action)
        printf ("KEYB: (%d,%d) %s pressed\n", key, scancode, kname);



    if ((mods & GLFW_MOD_ALT) != 0) //se ALT e' premuto
    {
        if (action == GLFW_RELEASE)
        {
            //ALT + ENTER  = fullscreen 
            if (key == GLFW_KEY_ENTER)
                app->toggleFullscreen();    

            //ALT + BACKSPACE = toggle VSync
            if (GLFW_KEY_BACKSPACE == key)
                app->toggleVSync();
        }            
    }
}


//************************************
VulkanApp::VulkanApp()
{ 
    gpu = NULL; 

    controller.bindBtnPress ("ALT INVIO", input::eOrigin::keyboard, GLFW_KEY_ENTER, input::sButtonModifier(input::eButtonModifier::LALT));
    controller.bindBtnPress ("ALT BACKSPACE", input::eOrigin::keyboard, GLFW_KEY_BACKSPACE, input::sButtonModifier(input::eButtonModifier::LALT));

    controller.bindBtnPress ("toggle mouse mode", input::eOrigin::keyboard, GLFW_KEY_TAB);

    controller.bindBtnPress ("Exit app", input::eOrigin::keyboard, GLFW_KEY_Q, input::sButtonModifier(input::eButtonModifier::LCTRL));
    controller.bindBtnPress ("Exit app", input::eOrigin::window, BUTTON_WINDOW_CLOSE);
   // controller.bindBtnPress ("Exit app", input::eOrigin::keyboard, GLFW_KEY_F4, input::sButtonModifier(input::eButtonModifier::LCTRL));

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
        gos::logger::log (eTextColor::white, "ALT + BASKPACE = toggle vsync\n");
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
    controller.beginParse (evtList);
    while (1)
    {
        const u32 event32 = controller.nextEvent();
        if (0 == event32)
            break;

        switch (event32)
        {
        default:
            virtual_onInputEvent (event32);
            break;

        case COMPILE_TIME_STR_CRC32("Exit app"):
            bQuitApp = true;
            break;

        case COMPILE_TIME_STR_CRC32("ALT INVIO"):
            this->toggleFullscreen();
            break;

        case COMPILE_TIME_STR_CRC32("ALT BACKSPACE"):
            this->toggleVSync();
            break;

        case COMPILE_TIME_STR_CRC32("toggle mouse mode"):
            input::window_toggleMouseMode(gpu->getWindow());
            break;
        }                    
    }
}    
