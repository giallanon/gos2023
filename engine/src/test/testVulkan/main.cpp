#include "gosGPU.h"
#include "VKExample1.h"

//******************************** 
void runExample1 (gos::GPU *gpu)
{
    VulkanExample1 app;
    if (app.init(gpu))
    {
        app.mainLoop();
        app.cleanup();
    }
}


//******************************** 
int main()
{
    gos::sGOSInit init;
    init.memory_setDefaultForGame();
    if (gos::init (init, "testVulkan"))
    {
        gos::GPU gpu;
        if (gpu.init (800, 600, gos::getAppName()))
        {
            runExample1 (&gpu);
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