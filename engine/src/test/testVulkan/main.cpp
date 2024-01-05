#include "gosGPU.h"
#include "VulkanExample1.h"
#include "VulkanExample2.h"

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
        if (gpu.init (800, 600, gos::getAppName()))
        {
            //runExample<VulkanExample1>(&gpu, "VulkanExample1");
            runExample<VulkanExample2>(&gpu, "VulkanExample2");
            gpu.deinit();
        }

        gos::deinit();
    }
#ifdef GOS_PLATFORM__WINDOWS
    printf ("\nPress any key to terminate\n");
    _getch();
#endif

    return 0;
}