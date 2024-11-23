#include "VulkanExample7.h"


using namespace gos;


//************************************
void VulkanExample7::virtual_explain()
{
}

//************************************
void VulkanExample7::virtual_onCleanup() 
{
    gpu->deleteResource (vtxShaderHandle);
    gpu->deleteResource (fragShaderHandle);
    gpu->deleteResource (pipelineHandle);
    gpu->deleteResource (renderLayoutHandle);
    gpu->deleteResource (frameBufferHandle);
    gpu->deleteResource (rt1);
}    


//************************************
bool VulkanExample7::virtual_onInit ()
{
    //creo un renderLayout
    gpu->renderLayout_createNew (&renderLayoutHandle)
        .requireRendertarget (gos::eImageFormat::U8_RGBA, eImageLayout::undefined, eImageLayout::transfer_src, eAttachmentLoadOp::clear, eAttachmentStoreOp::store)
        .addSubpass_GFX()
            .writeToRenderTarget(0)
        .end()
    .end();
    if (renderLayoutHandle.isInvalid())
    {
        gos::logger::err ("VulkanApp::init() => can't create renderTaskLayout\n");
        return false;
    }



    //creo un RT grosso tanto quanto la swapchain
    if (!gpu->renderTarget_create ("0-", "0-", gos::eImageFormat::U8_RGBA, &rt1))
    {
        gos::logger::err ("VulkanApp::init() => can't create render target\n");
        return false;
    }

    //creo un frame buffer per il renderLayout
    gpu->frameBuffer_createNew (renderLayoutHandle, &frameBufferHandle)
        //.bindRenderTarget (gpu->renderTarget_getDefault())
        .bindRenderTarget (rt1)
        .end();
    if (frameBufferHandle.isInvalid())
    {
        gos::logger::err ("VulkanApp::init() => can't create frameBufferHandle\n");
        return false;
    }



    //carico gli shader
    if (!gpu->vtxshader_createFromFile ("shader/example1/shader.vert.spv", "main", &vtxShaderHandle))
    {
        gos::logger::err ("VulkanApp::init() => can't create vert shader\n");
        return false;
    }
    if (!gpu->fragshader_createFromFile ("shader/example1/shader.frag.spv", "main", &fragShaderHandle))
    {
        gos::logger::err ("VulkanApp::init() => can't create frag shader\n");
        return false;
    }

    //creo la pipeline
    gpu->pipeline_createNew (renderLayoutHandle, &pipelineHandle)
        .addShader (vtxShaderHandle)
        .addShader (fragShaderHandle)
        .setVtxDecl (GPUVtxDeclHandle::INVALID())
        .depthStencil()
            .zbuffer_enable(true)
            .zbuffer_enableWrite(true)
            .zbuffer_setFn (eZFunc::LESS)
            .stencil_enable(false)
            .end()
        .end ();
        
    if (pipelineHandle.isInvalid())
    {
        gos::logger::err ("VulkanApp::init() => can't create pipeline\n");
        return false;
    }

    return true;
}    


//************************************
bool VulkanExample7::recordCommandBuffer (GPUCmdBufferHandle &cmdBufferHandle)
{
    gos::gpu::CmdBufferWriter cw;

    cw.begin (gpu, cmdBufferHandle)
        .setViewport (gpu->viewport_getDefault())
        .bindPipeline (pipelineHandle)
        .setClearColor (0, gos::ColorHDR(0, 0, 0))
        .setDepthBufferColor(1, 0)
        .renderPass_begin (renderLayoutHandle, frameBufferHandle)
            .draw(3, 1, 0, 0)
        .renderPass_end();


    //ora voglio copiare il contenuto di RT1 (che ho appena renderizzato) nella immagine della swap-chain corrente
    VkImage swapChainImage = gpu->swapChain_getCurImage();
    const gpu::RenderTarget *rtInfo = gpu->getInfo (rt1);

    cw  
        //.imageTransition (rtInfo->image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
        .imageTransition (swapChainImage, eImageLayout::undefined, eImageLayout::transfer_dst)
        .copyImageToImage (rtInfo->image, swapChainImage, gpu->swapChain_getImageExten2D(), gpu->swapChain_getImageExten2D())
        .imageTransition (swapChainImage, eImageLayout::transfer_dst, eImageLayout::presentation);
        

    cw.end();    



    return true;
}

/************************************
 * renderizza inviando command buffer a GPU e poi aspettando che questa
 * abbia finito il suo lavoro
 */
void VulkanExample7::virtual_onRun()
{
    GPUCmdBufferHandle  cmdBufferHandle;
    gpu->cmdBuffer_create (eGPUQueueType::gfx, &cmdBufferHandle);


    VkSemaphore         imageAvailableSemaphore;
    VkSemaphore         renderFinishedSemaphore;
    gpu->semaphore_create (&imageAvailableSemaphore);
    gpu->semaphore_create (&renderFinishedSemaphore);

    VkFence             inFlightFence;
    gpu->fence_create (true, &inFlightFence);


    VkResult            result;
    gos::TimerFPS       fpsTimer;
    gos::Timer          cpuWaitTimer;
    gos::Timer          frameTimer;
    gos::Timer          acquireImageTimer;
    while (bQuitApp == false)
    {
//printf ("frame begin\n");
        frameTimer.start();
        fpsTimer.onFrameBegin();

        handleInput();

        //draw frames
        cpuWaitTimer.start();
            gpu->fence_wait (inFlightFence);
//printf ("  CPU waited GPU fence for %ld us\n", cpuWaitTimer.elapsed_usec());

        //recupero una immagine dalla swap chain, attendo per sempre e indico [imageAvailableSemaphore] come
        //semaforo che GPU deve segnalare quando questa operazione e' ok
        acquireImageTimer.start();
            
        if (gpu->swapChain_acquireImage (UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE))
        {
            gpu->fence_reset (inFlightFence);
//printf ("  CPU waited vkAcquireNextImageKHR %ld us\n", acquireImageTimer.elapsed_usec());
        
            //command buffer che opera su [imageIndex]
            recordCommandBuffer(cmdBufferHandle);

            //submit
            VkCommandBuffer vkCmdBuffer;
            gpu->toVulkan (cmdBufferHandle, &vkCmdBuffer);

            VkSubmitInfo submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

            VkSemaphore semaphoresToBeWaitedBeforeStarting[] = { imageAvailableSemaphore };
            VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
            submitInfo.waitSemaphoreCount = 1;
            submitInfo.pWaitSemaphores = semaphoresToBeWaitedBeforeStarting;
            submitInfo.pWaitDstStageMask = waitStages;

            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &vkCmdBuffer;

            //semaforo che GPU segnalera' al termine dell'esecuzione del command buffer
            submitInfo.signalSemaphoreCount = 1;
            submitInfo.pSignalSemaphores = &renderFinishedSemaphore;

            //submitto il batch a GPU e indico che deve segnalare [inFlightFence] quando ha finito 
            result = vkQueueSubmit (gpu->REMOVE_getGfxQHandle(), 1, &submitInfo, inFlightFence);
            if (VK_SUCCESS != result)
                gos::logger::err ("vkQueueSubmit() => %s\n", string_VkResult(result));

            //presentazione
            gpu->swapChain_present (&renderFinishedSemaphore, 1);
//printf ("  total frame time: %ldus\n", frameTimer.elapsed_usec());
        }


        if (fpsTimer.onFrameEnd())
        {
            const float usec = fpsTimer.getAvgFrameTime_usec();
            const float msec = usec/ 1000.0f;
            printf ("Avg frame time: %.2fms [%.2fus] [fps: %.01f]\n", msec, usec, fpsTimer.getAvgFPS());
        }
    }

    //aspetto che GPU abbia finito tutto cio' che ha in coda
    gpu->waitIdle();

    gpu->deleteResource (cmdBufferHandle);
    gpu->semaphore_destroy (imageAvailableSemaphore);
    gpu->semaphore_destroy (renderFinishedSemaphore);
    gpu->fence_destroy (inFlightFence);
}

