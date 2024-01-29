#ifndef _GPUMainLoop_h_
#define _GPUMainLoop_h_
#include "gosGPU.h"
#include "FPSMegaTimer.h"
#include "../../gos/gosUtils.h"

/**************************************************
 * GPUMainLoop
 * 
*/
class GPUMainLoop
{
public:
    struct RunResult
    {
        u32     flag;
    };


public:
            GPUMainLoop ();

    void    setup (gos::GPU *gpuIN, FPSMegaTimer *fpsMegaTimer);
    void    unsetup();
  
    bool    run ();

    bool    canSubmitGFXJob () const;
    void    submitGFXJob (const GPUCmdBufferHandle &cmdBufferHandle);

private:
    static const u8     RESULTBIT_CAN_SUMBIT_GFX_JOB = 0; 

    enum class eStato : u8
    {
        waitingOnFence_inFlight,
        waitingOnFence_swapChainReady,
        askingNewFrame,
        waitingForAJob,
        unknown
    };

private:
    gos::GPU            *gpu;
    FPSMegaTimer        *fpsMegaTimer;
    eStato              stato;
    VkSemaphore         semaphore_renderFinished;
    VkFence             fence_inFlight;
    VkFence             fenceSwapChainReady;
    GPUCmdBufferHandle  commandBuffer_GFX;

    bool                canAccept_GFXJob;

};

#endif //_GPUMainLoop_h_