#include "gosGPU.h"
#include "gosInput.h"

using namespace gos;


//********************************** */
void foreverLoop (gos::GPU *gpu)
{
    gpu::MainLoop gpuLoop;
    gpuLoop.setup (gpu);

    while (1)
    {
        gpuLoop.stat_onCPUFrameBegin();
        input::pollEvents();
        gpuLoop.stat_onCPUFrameEnd();
        gpuLoop.stat_printReport();


        gpuLoop.run ();
        /*if (gpuLoop.swapchainRecreated())
            cam.changeAspectRatioPerspectiveFovLH (gpu->swapChain_calcAspectRatio());
        if (gpuLoop.canSubmitGFXJob())
        {
            recordCommandBuffer (cmdBufferHandle);
            gpuLoop.submitGFXJob (cmdBufferHandle);
        }*/

    }

    //aspetto che GPU abbia finito tutto cio' che ha in coda
    gpu->waitIdle();

}


//******************************** 
int main()
{
    gos::sGOSInit init;
    init.memory_setDefaultForGame();

    init.setLogMode (gos::sGOSInit::eLogMode::both_console_and_file);
    if (!gos::init (init, "simpleWorld"))
        return -1;

    if (!gos::input::init())
        return -2;

    GOSWinHandle mainWin;
    if (!gos::input::window_create (1024, 768, gos::getAppName(), &mainWin))
        return -3;
        
    gos::GPU gpu;
    if (!gpu.init (mainWin, false))
        return -4;
    
    foreverLoop (&gpu);
    
    gpu.deinit();
    gos::input::window_destroy (mainWin);
    gos::input::deinit();
    gos::deinit();
    return 0;
}