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


    const gpu::CommandBuffer *cmdBuffer = gpu->get_info(cmdBufferHandle);


    VkPipelineStageFlags semaphore_waitStages = { 0 };

    //dico a GPU di eseguire <cmdBufferHandle>
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 0;
    submitInfo.pWaitSemaphores = NULL;
    submitInfo.pWaitDstStageMask = &semaphore_waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmdBuffer->vkHandle;

    //submitto il batch a GPU e indico che deve segnalare <fence> quando ha finito 
    VkResult result = gpu->queue_submit (cmdBuffer->whichQ, 1, &submitInfo, fence);
    if (VK_SUCCESS != result)
    {
        stato = eStato::idle;
        gos::logger::err ("GFXJob::submit() => gpu->queue_submit() => %s\n", string_VkResult(result));
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
 *  TransferJob
 * 
 *************************************************************************************/
void TransferJob::setup (gos::GPU *gpuIN)         { gpu=gpuIN; gpu->fence_create (false, &fence); }
void TransferJob::unsetup()                       { if (NULL == gpu) return; gpu->fence_destroy (fence); gpu = NULL; }
void TransferJob::submit (const GPUCmdBufferHandle &cmdBufferHandle)
{
    assert (eStato::idle == stato);
    stato = eStato::jobInProgress;

    const gpu::CommandBuffer *cmdBuffer = gpu->get_info(cmdBufferHandle);

    VkPipelineStageFlags semaphore_waitStages = { 0 };
	semaphore_waitStages = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

    //dico a GPU di eseguire <cmdBufferHandle>
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 0;
    submitInfo.pWaitSemaphores = NULL;
    submitInfo.pWaitDstStageMask = &semaphore_waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmdBuffer->vkHandle;

    //submitto il batch a GPU e indico che deve segnalare <fence> quando ha finito 
    VkResult result = gpu->queue_submit (cmdBuffer->whichQ, 1, &submitInfo, fence);
    if (VK_SUCCESS != result)
    {
        stato = eStato::idle;
        gos::logger::err ("TransferJob::submit() => gpu->queue_submit() => %s\n", string_VkResult(result));
    }
}

//*****************************************************
bool TransferJob::hasFinished()
{
    if (eStato::jobInProgress == stato)
    {
        if (!gpu->fence_isSignaled(fence))
            return false;
        gpu->fence_reset(fence);
        stato = eStato::idle;
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

        logger::log (eTextColor::white, "cpu: avg %.2fms [fps: %.01f]    cmd: avg %.2fms [fps: %.01f]    gpu: avg %.2fms [fps: %.01f]    acquire: avg %.2fms [fps: %.01f]\n",
            cpuTimerFPS.getAvgFrameTime_ms(), cpuTimerFPS.getAvgFPS(),
            cmdBufferTimerFPS.getAvgFrameTime_ms(), cmdBufferTimerFPS.getAvgFPS(),
            gfxJob.getTimerFPS()->getAvgFrameTime_ms(), gfxJob.getTimerFPS()->getAvgFPS(),
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