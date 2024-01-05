#include "VulkanApp.h"

using namespace gos;


//************************************
void GLFW_key_callback (GLFWwindow* window, int key, UNUSED_PARAM(int scancode), int action, int mods)
{
    VulkanApp *app = reinterpret_cast<VulkanApp*> (glfwGetWindowUserPointer(window));
    if (key == GLFW_KEY_ENTER && action == GLFW_RELEASE)
    {
        if ((mods & GLFW_MOD_ALT) != 0)
            app->toggleFullscreen();    //ALT + ENTER
    }
}


//************************************
bool VulkanApp::init (gos::GPU *gpuIN, const char *title)
{
    gpu = gpuIN;

    glfwSetWindowTitle (gpu->getWindow(), title);
    glfwSetWindowUserPointer (gpu->getWindow(), this);
    glfwSetKeyCallback (gpu->getWindow(), GLFW_key_callback);

    return virtual_onInit();
}    

