#include "VulkanApp.h"

using namespace gos;


//************************************
void GLFW_key_callback (GLFWwindow* window, int key, UNUSED_PARAM(int scancode), int action, int mods)
{
    VulkanApp *app = reinterpret_cast<VulkanApp*> (glfwGetWindowUserPointer(window));


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
bool VulkanApp::init (gos::GPU *gpuIN, const char *title)
{
    gpu = gpuIN;

    glfwSetWindowTitle (gpu->getWindow(), title);
    glfwSetWindowUserPointer (gpu->getWindow(), this);
    glfwSetKeyCallback (gpu->getWindow(), GLFW_key_callback);

    if (!virtual_onInit())
        return false;

    gos::logger::log (eTextColor::white, "\n\n=======================================================\n");
    gos::logger::log (eTextColor::green, "%s\n", title);
    gos::logger::log (eTextColor::white, "ALT + ENTER = toggle fullscreen\n");
    gos::logger::log (eTextColor::white, "ALT + BASKPACE = toggle vsync\n");
    gos::logger::log ("\n\n");
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

