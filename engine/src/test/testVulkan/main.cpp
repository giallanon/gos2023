#include "gosGPU.h"
#include "VulkanExample1.h"
#include "VulkanExample2.h"
#include "VulkanExample3.h"
#include "VulkanExample4.h"

//******************************** 
template<class VKAPP>
void runExample (gos::GPU *gpu, const char *title)
{
    VKAPP app;
    if (app.init(gpu, title))
    {
        app.run();
        app.cleanup();
    }
}


//******************************** 
int main()
{
    gos::sGOSInit init;
    init.memory_setDefaultForGame();
    init.setLogMode (gos::sGOSInit::eLogMode::both_console_and_file);
    if (gos::init (init, "testVulkan"))
    {
        gos::GPU gpu;
        if (gpu.init (800, 600, false, gos::getAppName()))
        {
            //runExample<VulkanExample1>(&gpu, "VulkanExample1");
            //runExample<VulkanExample2>(&gpu, "VulkanExample2");
            //runExample<VulkanExample3>(&gpu, "VulkanExample3");
            runExample<VulkanExample4>(&gpu, "VulkanExample4");
            gpu.deinit();
        }

        gos::deinit();
    }


    return 0;
}