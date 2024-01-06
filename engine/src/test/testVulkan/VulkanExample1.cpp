#include "VulkanExample1.h"


using namespace gos;


//************************************
void VulkanExample1::virtual_explain()
{
}

//************************************
void VulkanExample1::virtual_onCleanup() 
{
    gpu->deleteResource (vtxShaderHandle);
    gpu->deleteResource (fragShaderHandle);
    gpu->deleteResource (pipelineHandle);
    gpu->deleteResource (renderLayoutHandle);
    gpu->deleteResource (frameBufferHandle);
}    


//************************************
bool VulkanExample1::virtual_onInit ()
{
    //creo il render pass
    gpu->renderLayout_createNew (&renderLayoutHandle)
        .requireRendertarget (eRenderTargetUsage::presentation, gpu->swapChain_getImageFormat(), true, gos::ColorHDR(0xff000080))
        .addSubpass_GFX()
            .useRenderTarget(0)
        .end()
    .end();
    if (renderLayoutHandle.isInvalid())
    {
        gos::logger::err ("VulkanApp::init() => can't create renderTaskLayout\n");
        return false;
    }

    //frame buffers
    gpu->frameBuffer_createNew (renderLayoutHandle, &frameBufferHandle)
        .bindRenderTarget (gpu->renderTarget_getDefault())
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
bool VulkanExample1::recordCommandBuffer (gos::GPU *gpuIN, 
                                            const GPURenderLayoutHandle &renderLayoutHandle, 
                                            const GPUFrameBufferHandle &frameBufferHandle,
                                            const GPUPipelineHandle &pipelineHandle,
                                            VkCommandBuffer *out_commandBuffer)
{
    assert (out_commandBuffer);

    //recupero il vulkan render pass
    VkRenderPass vkRenderPassHandle = VK_NULL_HANDLE;
    if (!gpuIN->toVulkan (renderLayoutHandle, &vkRenderPassHandle))
    {
        gos::logger::err ("VulkanApp::recordCommandBuffer() => invalid renderLayoutHandle\n");
        return false;
    }

    //recupero il frame buffer
    VkFramebuffer vkFrameBufferHandle;
    u32 renderAreaW;
    u32 renderAreaH;
    if (!gpuIN->toVulkan (frameBufferHandle, &vkFrameBufferHandle, &renderAreaW, &renderAreaH))
    {
        gos::logger::err ("VulkanApp::recordCommandBuffer() => invalid frameBufferHandle\n");
        return false;
    }

    //recupero vulkan pipeline
    VkPipeline          vkPipelineHandle;
    VkPipelineLayout    vkPipelineLayoutHandle;
    if (!gpuIN->toVulkan (pipelineHandle, &vkPipelineHandle, &vkPipelineLayoutHandle))
    {
        gos::logger::err ("VulkanApp::recordCommandBuffer() => invalid pipelineHandle\n");
        return false;
    }


    VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0; // Optional
        beginInfo.pInheritanceInfo = nullptr; // Optional

    VkResult result = vkBeginCommandBuffer (*out_commandBuffer, &beginInfo);
    if (VK_SUCCESS != result)
    {
        gos::logger::err ("recordCommandBuffer() => vkBeginCommandBuffer() => %s\n", string_VkResult(result));
        return false;
    }


    VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = vkRenderPassHandle;
        renderPassInfo.framebuffer = vkFrameBufferHandle;
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = { renderAreaW, renderAreaH };
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;    

    vkCmdBeginRenderPass (*out_commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline (*out_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkPipelineHandle);

    //setto la viewport
    const gos::gpu::Viewport *viewport = gpuIN->viewport_getDefault();
    VkViewport vkViewport {0.0f, 0.0f, viewport->getW_f32(), viewport->getH_f32(), 0.0f, 1.0f };
    vkCmdSetViewport(*out_commandBuffer, 0, 1, &vkViewport);

    VkRect2D scissor { 0, 0, viewport->getW(), viewport->getH() };
    vkCmdSetScissor (*out_commandBuffer, 0, 1, &scissor);

    //draw primitive
    vkCmdDraw(*out_commandBuffer, 3, 1, 0, 0);

    //fine del render pass
    vkCmdEndRenderPass (*out_commandBuffer);

    //fine del command buffer
    result = vkEndCommandBuffer (*out_commandBuffer);
    if (VK_SUCCESS != result)
    {
        gos::logger::err ("recordCommandBuffer() => vkEndCommandBuffer() => %s\n", string_VkResult(result));
        return false;
    }    
    
    return true;
}

/************************************
 * renderizza inviando command buffer a GPU e poi aspettando che questa
 * abbia finito il suo lavoro
 */
void VulkanExample1::virtual_onRun()
{
    VkCommandBuffer     vkCommandBuffer;
    gpu->createCommandBuffer (eGPUQueueType::gfx, &vkCommandBuffer);

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
    while (!glfwWindowShouldClose (gpu->getWindow()))
    {
//printf ("frame begin\n");
        frameTimer.start();
        fpsTimer.onFrameBegin();

        glfwPollEvents();

        //draw frames
        cpuWaitTimer.start();
            gpu->fence_wait (inFlightFence);
//printf ("  CPU waited GPU fence for %ld us\n", cpuWaitTimer.elapsed_usec());

        //recupero una immagine dalla swap chain, attendo per sempre e indico [imageAvailableSemaphore] come
        //semaforo che GPU deve segnalare quando questa operazione e' ok
        acquireImageTimer.start();
            
        if (gpu->newFrame (UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE))
        {
            gpu->fence_reset (inFlightFence);
//printf ("  CPU waited vkAcquireNextImageKHR %ld us\n", acquireImageTimer.elapsed_usec());
        
            //command buffer che opera su [imageIndex]
            //recordCommandBuffer(gpu, renderLayoutHandle, frameBufferHandleList[imageIndex], pipelineHandle, &vkCommandBuffer);
            recordCommandBuffer(gpu, renderLayoutHandle, frameBufferHandle, pipelineHandle, &vkCommandBuffer);

            //submit
            VkSubmitInfo submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

            VkSemaphore semaphoresToBeWaitedBeforeStarting[] = { imageAvailableSemaphore };
            VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
            submitInfo.waitSemaphoreCount = 1;
            submitInfo.pWaitSemaphores = semaphoresToBeWaitedBeforeStarting;
            submitInfo.pWaitDstStageMask = waitStages;

            submitInfo.commandBufferCount = 1;        
            submitInfo.pCommandBuffers = &vkCommandBuffer;

            //semaforo che GPU segnalera' al termine dell'esecuzione del command buffer
            submitInfo.signalSemaphoreCount = 1;
            submitInfo.pSignalSemaphores = &renderFinishedSemaphore;

            //submitto il batch a GPU e indico che deve segnalare [inFlightFence] quando ha finito 
            result = vkQueueSubmit (gpu->REMOVE_getGfxQHandle(), 1, &submitInfo, inFlightFence);
            if (VK_SUCCESS != result)
                gos::logger::err ("vkQueueSubmit() => %s\n", string_VkResult(result));

            //presentazione
            gpu->present (&renderFinishedSemaphore, 1);
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

    gpu->deleteCommandBuffer (eGPUQueueType::gfx, vkCommandBuffer);
    gpu->semaphore_destroy (imageAvailableSemaphore);
    gpu->semaphore_destroy (renderFinishedSemaphore);
    gpu->fence_destroy (inFlightFence);
}

