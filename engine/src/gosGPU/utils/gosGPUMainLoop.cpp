#include "gosGPUMainLoop.h"
#include "../gosGPU.h"
#include "../../gos/gosUtils.h"

using namespace gos;
using namespace gos::gpu;


/************************************************************************************
 *
 *  AquireSwapChainImage
 * 
 *************************************************************************************/
void AquireSwapChainImage::setup (gos::GPU *gpuIN)          { gpu=gpuIN; gpu->fence_create (false, &fence); }
void AquireSwapChainImage::unsetup()                        { if (NULL == gpu) return; gpu->fence_destroy (fence); gpu = NULL; }
bool AquireSwapChainImage::tryAcquire ()
{
    assert (NULL != gpu);
    
    if (eStato::idle == stato)
    {
        timerFPS.onFrameBegin();
        stato = eStato::acquiring;
    }

    if (eStato::acquiring == stato)
    {
        if (!gpu->swapChain_acquireImage (&acquiredImg, 0, VK_NULL_HANDLE, fence))
            return false;
        stato = eStato::waitingFence;
    }

    if (eStato::waitingFence == stato)
    {
        if (!gpu->fence_isSignaled (fence))
            return false;

        timerFPS.onFrameEnd();
        stato = eStato::idle;
        gpu->fence_reset(fence);
        return true;
    }

    //se arrivo qui, e' successo qualcosa di strano perche' i casi li ho gia' gestiti tutti sopra
    DBGBREAK;
    return false;
}



/************************************************************************************
 *
 *  GFXJob
 * 
 *************************************************************************************/
void GFXJob::setup (gos::GPU *gpuIN)         { gpu=gpuIN; gpu->fence_create (false, &fence); }
void GFXJob::unsetup()                       { if (NULL == gpu) return; gpu->fence_destroy (fence); gpu = NULL; }
void GFXJob::priv_submit (const GPUCmdBufferHandle &cmdBufferHandle, u32 swapChainImageIndexIN)
{
    assert (eStato::idle == stato);
    timerFPS.onFrameBegin();
    stato = eStato::jobInProgress;
    swapChainImageIndex = swapChainImageIndexIN;
    swapChainAutoID = gpu->swapChain_getCurrentAutoID();


    VkCommandBuffer vkCommandBuffer_GFX;
    gpu->toVulkan (cmdBufferHandle, &vkCommandBuffer_GFX);


    VkPipelineStageFlags waitStages[] = { 0 }; //{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

    //dico a GPU di eseguire <cmdBufferHandle>
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 0; //1;
    submitInfo.pWaitSemaphores = NULL; //semaphoresToBeWaitedBeforeStarting;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &vkCommandBuffer_GFX;

    //semaforo che GPU segnalera' al termine dell'esecuzione di questo batch di lavoro
    //submitInfo.signalSemaphoreCount = 0; //1;
    //submitInfo.pSignalSemaphores = &semaphore;

    //submitto il batch a GPU e indico che deve segnalare <fence> quando ha finito 
    VkResult result = vkQueueSubmit (gpu->REMOVE_getGfxQHandle(), 1, &submitInfo, fence);
    if (VK_SUCCESS != result)
    {
        stato = eStato::idle;
        gos::logger::err ("GFXJob::submit() => vkQueueSubmit() => %s\n", string_VkResult(result));
    }
}

//*****************************************************
bool GFXJob::hasFinished()
{
    if (eStato::jobInProgress == stato)
    {
        if (!gpu->fence_isSignaled(fence))
            return false;
        timerFPS.onFrameEnd();
        gpu->fence_reset(fence);
        stato = eStato::idle;
        
        //se nel frattempo la swapchain e' stata ricreata, non posso presentare perche' l'immagine non e' + valida
        if (u32MAX != swapChainImageIndex && swapChainAutoID == gpu->swapChain_getCurrentAutoID())
            gpu->swapChain_present (NULL, 0, swapChainImageIndex);
    }

    return true;
}



/************************************************************************************
 *
 *  MainLoop2
 * 
 *************************************************************************************/
void MainLoop2::run ()
{
    //chiedo una immagine alla swapchain, ne accumulo fino a 2
    if (acquiredList.getNElem() < 2)
    {
        if (acquire.tryAcquire ())
            acquiredList.push (acquire.acquiredImg);
    }

    if (acquire.gpu->swapChain_wasRecreated())
        acquiredList.reset();

    gfxJobFinished = gfxJob.hasFinished();
    
    //statistiche
    const u64 timenow_msec = gos::getTimeSinceStart_msec();
    if (timenow_msec >= nextTimePrintInfo_msec)
    {
        nextTimePrintInfo_msec = timenow_msec + printInfoFreq_msec;

        printf ("cpu: avg %.2fms [fps: %.01f]    gpu: avg %.2fms [fps: %.01f]    acquire: avg %.2fms [fps: %.01f]\n",
            cpuTimerFPS.getAvgFrameTime_ms(), cpuTimerFPS.getAvgFPS(),
            gfxJob.timerFPS.getAvgFrameTime_ms(), gfxJob.timerFPS.getAvgFPS(),
            acquire.timerFPS.getAvgFrameTime_ms(), acquire.timerFPS.getAvgFPS());
    }
}

bool MainLoop2::gfxJob_canSubmit (SwapchainImg *out)
{
    if (gfxJobFinished && acquiredList.getNElem() > 0)
    {
        acquiredList.pop (out);
        return true;
    }

    return false;
}

void MainLoop2::gfxJob_submitAndPresent (const GPUCmdBufferHandle &cmdBufferHandle, const SwapchainImg &info)
{
    gfxJob.submitAndPresent (cmdBufferHandle, info);
}