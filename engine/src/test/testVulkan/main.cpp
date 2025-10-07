#include "gosGPU.h"
#include "gosInput.h"
#include "VulkanExample1.h"
#include "VulkanExample2.h"
#include "VulkanExample3.h"
#include "VulkanExample4.h"
#include "VulkanExample5.h"
#include "VulkanExample6.h"
#include "VulkanExample7.h"

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

template<class VKAPP_NOWIN>
void runExampleNoWin (gos::GPU *gpu)
{
    VKAPP_NOWIN app;
    if (app.init(gpu))
    {
        app.run();
        app.cleanup();
    }
}

//******************************** 
void testWithWindow()
{
    if (gos::input::init())
    {
        GOSWinHandle mainWin;
        if (gos::input::window_create (1024, 768, gos::getAppName(), &mainWin))
        {
            gos::GPU gpu;
            if (gpu.init (mainWin, false))
            {
                // runExample<VulkanExample1>(&gpu, "VulkanExample1");
                // runExample<VulkanExample2>(&gpu, "VulkanExample2");
                // runExample<VulkanExample3>(&gpu, "VulkanExample3");
                // runExample<VulkanExample4>(&gpu, "VulkanExample4");
                // runExample<VulkanExample5>(&gpu, "VulkanExample5");
                runExample<VulkanExample6>(&gpu, "VulkanExample6");
                gpu.deinit();
            }
            gos::input::window_destroy (mainWin);
        }

        gos::input::deinit();
    }

}

//******************************** 
void testWindoless()
{
    gos::GPU gpu;
    if (gpu.init (GOSWinHandle::INVALID(), false))
    {
        runExampleNoWin<VulkanExample7>(&gpu);  // VulkanExample7
        gpu.deinit();
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
        //testWindoless();
        testWithWindow();

        gos::deinit();
    }


    return 0;
}