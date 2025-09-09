#include "gosGPUMainLoop.h"
#include "../gosGPU.h"
#include "../../gos/gosUtils.h"

using namespace gos;
using namespace gos::gpu;

//************************************************
MainLoop::MainLoop()
{
    gpu = NULL;
    stato = eStato::askingNewSwapchainImg;
    canAccept_GFXJob = false;

    fpsMegaTimer.addTimer ("CPU");
    fpsMegaTimer.addTimer ("GPU");
    fpsMegaTimer.addTimer ("FPS");
}

//************************************************
MainLoop::~MainLoop()
{
    if (NULL == gpu)
        return;
    unsetup();
    gpu = NULL;
}

//************************************************
void MainLoop::setup (gos::GPU *gpuIN)
{
    gpu = gpuIN;

    //I semafori sono oggetti di sync tra GPU & GPU (non e' un errore e' proprio GPU-GPU)
    //Fence sono oggetti di sync tra GPU & CPU (a differenza dei semafori che riguardano solo la CPU)
    gpu->semaphore_create (&semaphore_renderFinished);
    gpu->fence_create (false, &fence_inFlight);
    gpu->fence_create (false, &fence_swapChainImgReady);
}

//************************************************
void MainLoop::unsetup()
{
    gpu->semaphore_destroy (semaphore_renderFinished);
    gpu->fence_destroy (fence_swapChainImgReady);
    gpu->fence_destroy (fence_inFlight);
}


//************************************************
bool MainLoop::canSubmitGFXJob () const
{
    return canAccept_GFXJob;
}

//************************************************
void MainLoop::submitGFXJob (const GPUCmdBufferHandle &cmdBufferHandle)
{
    assert (canAccept_GFXJob);
    
    if (canAccept_GFXJob)
    {
        commandBuffer_GFX = cmdBufferHandle;
        canAccept_GFXJob = false;
    }
}


//************************************************
bool MainLoop::run ()
{
    bSwapchainRecreated = false;
    if (eStato::waitingOnFence_inFlight == stato)
    {
        //attende che il precedente batch sia terminato
        if (!gpu->fence_wait (fence_inFlight, 0))
            return false;
        gpu->fence_reset (fence_inFlight);
        fpsMegaTimer.onFrameEnd (1);
        stato = eStato::askingNewSwapchainImg;
    }


    if (eStato::askingNewSwapchainImg == stato)
    {
        //Chiedo a GPU una immagine dalla swap chain, non attendo nemmeno 1 attimo e indico [fence_swapChainImgReady] come
        //oggetto che GPU deve segnalare quando questa operazione e' ok e  l'immagine e' davvero disponibile per il rendering.
        //Se la fn ritorna true, allora poi dobbiamo attendere che [fence_swapChainImgReady] sia segnalata prima di avere davvero a disposizione
        //l'immagine
        if (!gpu->swapChain_acquireImage (0, VK_NULL_HANDLE, fence_swapChainImgReady))
            return false;

        bSwapchainRecreated = gpu->swapChain_wasRecreated();
        canAccept_GFXJob = true;
        stato = eStato::fenceWaiting_swapChainImg;
    }

    if (eStato::fenceWaiting_swapChainImg == stato)
    {
        //A questo punto GPU ha capito quale sara' l'immagine che prima o poi mi dara', ma non e' detto che questa sia gia' disponibile
        //Lo diventa quando [fence_swapChainImgReady] e' segnalata.
        //Fino ad allora posso farmi i fatti miei

        //Intanto che aspetto che GPU renda disponibile una immagine, faccio le mie cose
        if (!gpu->fence_wait (fence_swapChainImgReady, 0))
            return canAccept_GFXJob;
        gpu->fence_reset (fence_swapChainImgReady);
        stato = eStato::waitingForAJob;
    }

    if (eStato::waitingForAJob == stato)
    {
        if (canAccept_GFXJob)
            return true;

        //se arriviamo qui vuol dire che un job e' stato schedulato
        stato = eStato::waitingOnFence_inFlight;

        fpsMegaTimer.onFrameEnd (2);
        fpsMegaTimer.onFrameBegin(2);

        //arrivo qui quando GPU mi ha finalmente dato l'immagine
        fpsMegaTimer.onFrameBegin(1);


        //submit del command buffer
        {
            VkCommandBuffer vkCommandBuffer_GFX;
            gpu->toVulkan (commandBuffer_GFX, &vkCommandBuffer_GFX);
            
            //indico a GPU che quando questo bacth di lavoro arriva nello state di presentazione [VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT],
            //allora, prima di iniziare quello stage, deve aspettare che il semaforo [semaphore_imageReady] sia segnalato (il che implica
            //che GPU ha finalmente a disposizione l'immagine che ci ha promesso in newFram()
            //
            //Questo batch quindi segnala 2 cose quando ha finito:
            //  1- renderFinishedSemaphore (che serve a GPU per far partire la present()
            //  2- inFlightFence che serve a CPU per sapere che il lavoro che ha submittato e' stato completato
            VkPipelineStageFlags waitStages[] = {0}; //{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
            //VkSemaphore semaphoresToBeWaitedBeforeStarting[] = { semaphore_imageReady }; 

            VkSubmitInfo submitInfo{};
                submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                submitInfo.waitSemaphoreCount = 0; //1;
                submitInfo.pWaitSemaphores = NULL; //semaphoresToBeWaitedBeforeStarting;
                submitInfo.pWaitDstStageMask = waitStages;
                submitInfo.commandBufferCount = 1;        
                submitInfo.pCommandBuffers = &vkCommandBuffer_GFX;

                //semaforo che GPU segnalera' al termine dell'esecuzione di questo batch di lavoro
                submitInfo.signalSemaphoreCount = 1;
                submitInfo.pSignalSemaphores = &semaphore_renderFinished;

                //submitto il batch a GPU e indico che deve segnalare [inFlightFence] quando ha finito 
                VkResult result = vkQueueSubmit (gpu->REMOVE_getGfxQHandle(), 1, &submitInfo, fence_inFlight);
                if (VK_SUCCESS != result)
                    gos::logger::err ("vkQueueSubmit() => %s\n", string_VkResult(result));
        }

        //presentazione
        //Indico a GPU che deve attendere [renderFinishedSemaphore] prima di presentare
        gpu->swapChain_present (&semaphore_renderFinished, 1);
        return false;
    }
    

    DBGBREAK;
    return false;
}


//****************************
void AquireSwapChainImage::setup (gos::GPU *gpuIN)          { gpu=gpuIN; gpu->fence_create (false, &fence); }
void AquireSwapChainImage::unsetup()                        { if (NULL == gpu) return; gpu->fence_destroy (fence); gpu = NULL; }

//*****************************************************
bool AquireSwapChainImage::tryAcquire (VkImage *out_image)
{
    assert (NULL != gpu);
    assert (NULL != out_image);
    
    if (eStato::idle == stato)
    {
        timerFPS.onFrameBegin();
        stato = eStato::acquiring;
    }

    if (eStato::acquiring == stato)
    {
        if (!gpu->swapChain_acquireImage_ex (&imageIndex, 0, VK_NULL_HANDLE, fence))
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
        *out_image = gpu->swapChain_getImageByIndex(imageIndex);
        return true;
    }

    //se arrivo qui, e' successo qualcosa di strano perche' i casi li ho gia' gestiti tutti sopra
    DBGBREAK;
    return false;
}



//*****************************************************
void PresentGFXJob::setup (gos::GPU *gpuIN)         { gpu=gpuIN; gpu->fence_create (false, &fence); }
void PresentGFXJob::unsetup()                       { if (NULL == gpu) return; gpu->fence_destroy (fence); gpu = NULL; }

//*****************************************************
void PresentGFXJob::submit (const GPUCmdBufferHandle &cmdBufferHandle, u32 swapChainImageIndexIN)
{
    assert (eStato::idle == stato);
    stato = eStato::jobInProgress;
    swapChainImageIndex = swapChainImageIndexIN;
    swapChainAutoID = gpu->swapChain_getCurrentAutoID();


    VkCommandBuffer vkCommandBuffer_GFX;
    gpu->toVulkan (cmdBufferHandle, &vkCommandBuffer_GFX);



    VkPipelineStageFlags waitStages[] = { 0 }; //{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

    //dico a GPU di eseguire <cmdBufferHandle> e di segnalare <semaphore> quando il job e' terminato
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 0; //1;
    submitInfo.pWaitSemaphores = NULL; //semaphoresToBeWaitedBeforeStarting;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &vkCommandBuffer_GFX;

    //semaforo che GPU segnalera' al termine dell'esecuzione di questo batch di lavoro
    submitInfo.signalSemaphoreCount = 0; //1;
    //submitInfo.pSignalSemaphores = &semaphore;

    //submitto il batch a GPU e indico che deve segnalare <fence> quando ha finito 
    VkResult result = vkQueueSubmit (gpu->REMOVE_getGfxQHandle(), 1, &submitInfo, fence);
    if (VK_SUCCESS != result)
        gos::logger::err ("vkQueueSubmit() => %s\n", string_VkResult(result));
}

//*****************************************************
bool PresentGFXJob::hasFinished()
{
    if (eStato::jobInProgress == stato)
    {
        if (!gpu->fence_isSignaled(fence))
            return false;

        //se nel frattempo la swapchain e' stata ricreata, non posso presentare perche' l'immagine non e' + valida
        if (swapChainAutoID == gpu->swapChain_getCurrentAutoID())
            gpu->swapChain_present_ex (NULL, 0, swapChainImageIndex);

        gpu->fence_reset(fence);
        stato = eStato::idle;
    }

    return true;
}