#include "GPUMainLoop.h"

//************************************************
GPUMainLoop::GPUMainLoop()
{
    stato = eStato::askingNewFrame;
    canAccept_GFXJob = false;
}

//************************************************
void GPUMainLoop::setup (gos::GPU *gpuIN, FPSMegaTimer *fpsMegaTimerIN)
{
    gpu = gpuIN;
    fpsMegaTimer = fpsMegaTimerIN;

    //I semafori sono oggetti di sync tra GPU & GPU (non e' un errore e' proprio GPU-GPU)
    //Fence sono oggetti di sync tra GPU & CPU (a differenza dei semafori che riguardano solo la CPU)
    gpu->semaphore_create (&semaphore_renderFinished);
    gpu->fence_create (false, &fence_inFlight);
    gpu->fence_create (false, &fenceSwapChainReady);
}

//************************************************
void GPUMainLoop::unsetup()
{
    gpu->semaphore_destroy (semaphore_renderFinished);
    gpu->fence_destroy (fenceSwapChainReady);
    gpu->fence_destroy (fence_inFlight);
}


//************************************************
bool GPUMainLoop::canSubmitGFXJob () const
{
    return canAccept_GFXJob;
}

//************************************************
void GPUMainLoop::submitGFXJob (const VkCommandBuffer &vkCommandBuffer)
{
    assert (canAccept_GFXJob);
    
    if (canAccept_GFXJob)
    {
        vkCommandBuffer_GFX = vkCommandBuffer;
        canAccept_GFXJob = false;
    }
}

//************************************************
bool GPUMainLoop::run ()
{
    if (eStato::waitingOnFence_inFlight == stato)
    {
        //attende che il precedente batch sia terminato
        if (!gpu->fence_wait (fence_inFlight, 0))
            return false;
        gpu->fence_reset (fence_inFlight);
        fpsMegaTimer->onFrameEnd (FPSTIMER_GPU);
        stato = eStato::askingNewFrame;
    }


    if (eStato::askingNewFrame == stato)
    {
        //Chiedo a GPU una immagine dalla swap chain, non attendo nemmeno 1 attimo e indico [semaphore_imageReady] come
        //semaforo che GPU deve segnalare quando questa operazione e' ok. Indico inoltre [fenceSwapChainReady] come fence da segnalre
        //quando l'immagine e' disponibile
        //Questa fn ritorna quando GPU e' in grado di determinare quale sara' la prossima immagine sulla quale renderizzare.
        //Quando GPU ha questa informazione, non vuol dire pero' che l'immagine e' gia' immediatamente disponibile per l'uso.
        //E' per questo che si usa [semaphore_imageReady] e [fenceSwapChainReady], per sapere quando davvero l'immagine sara' disponibile
        if (!gpu->newFrame (0, VK_NULL_HANDLE, fenceSwapChainReady))
            return false;

        canAccept_GFXJob = true;
        stato = eStato::waitingOnFence_swapChainReady;
    }

    if (eStato::waitingOnFence_swapChainReady == stato)
    {
        //A questo GPU ha capito quale sara' l'immagine che prima o poi mi dara', ma non e' detto che questa sia gia' disponibile
        //Lo diventa quando [fenceSwapChainReady] e' segnalata.
        //Fino ad allora posso farmi i fatti miei

        //Intanto che aspetto che GPU renda disponibile una immagine, faccio le mie cose
        if (!gpu->fence_wait (fenceSwapChainReady, 0))
            return canAccept_GFXJob;
        gpu->fence_reset (fenceSwapChainReady);
        stato = eStato::waitingForAJob;
    }

    if (eStato::waitingForAJob == stato)
    {
        if (canAccept_GFXJob)
            return true;

        //se arriviamo qui vuol dire che un job e' stato schedulato
        stato = eStato::waitingOnFence_inFlight;

        fpsMegaTimer->onFrameEnd (FPSTIMER_FPS);
        fpsMegaTimer->onFrameBegin(FPSTIMER_FPS);

        //arrivo qui quando GPU mi ha finalmente dato l'immagine
        fpsMegaTimer->onFrameBegin(FPSTIMER_GPU);


        //submit del command buffer
        {
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
        gpu->present (&semaphore_renderFinished, 1);
        return false;
    }
    

    DBGBREAK;
    return false;
}

