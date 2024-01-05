#include "VulkanExample2.h"


using namespace gos;


//************************************
VulkanExample2::VulkanExample2()
{
}

//************************************
void VulkanExample2::virtual_onCleanup() 
{
    gpu->deleteResource (vtxBufferHandle);
    gpu->deleteResource (vtxShaderHandle);
    gpu->deleteResource (fragShaderHandle);
    gpu->deleteResource (pipelineHandle);
    gpu->deleteResource (renderLayoutHandle);
    gpu->deleteResource (frameBufferHandle);
}    


//************************************
bool VulkanExample2::virtual_onInit ()
{
    //vertici
    vertexList[0].pos.set (0.0f, -0.5f);
    vertexList[0].colorRGB.set (1.0f, 0.0f, 0.0f);
    vertexList[1].pos.set (0.5f, 0.5);
    vertexList[1].colorRGB.set (0.0f, 1.0f, 0.0f);
    vertexList[2].pos.set (-0.5f, 0.5);
    vertexList[2].colorRGB.set (0.0f, 0.0f, 1.0f);


    if (!createVertexBuffer())
    {
        gos::logger::err ("VulkanApp::init() => can't create VtxBuffer\n");
        return false;
    }

    //Vtx declaration
    GPUVtxDeclHandle vtxDeclHandle;
    gpu->vtxDecl_createNew (&vtxDeclHandle)
        .addStream(eVtxStreamInputRate::perVertex)
        .addLayout (0, offsetof(Vertex, pos), eDataFormat::_2f32)        //position
        .addLayout (1, offsetof(Vertex, colorRGB), eDataFormat::_3f32)   //color
        .end();
    if (vtxDeclHandle.isInvalid())
    {
        gos::logger::err ("VulkanApp::init() => can't create vtxDeclHandle\n");
        return false;
    }


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
    if (!gpu->vtxshader_createFromFile ("shader/example2/shader.vert.spv", "main", &vtxShaderHandle))
    {
        gos::logger::err ("VulkanApp::init() => can't create vert shader\n");
        return false;
    }
    if (!gpu->fragshader_createFromFile ("shader/example2/shader.frag.spv", "main", &fragShaderHandle))
    {
        gos::logger::err ("VulkanApp::init() => can't create frag shader\n");
        return false;
    }

    //creo la pipeline
    gpu->pipeline_createNew (renderLayoutHandle, &pipelineHandle)
        .addShader (vtxShaderHandle)
        .addShader (fragShaderHandle)
        .setVtxDecl (vtxDeclHandle)
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


    //non mi serve piu'
    gpu->deleteResource (vtxDeclHandle);

    return true;
}    


//************************************
bool VulkanExample2::createVertexBuffer()
{
    const u32 sizeInByte = sizeof(Vertex) * NUM_VERTEX;
    if (!gpu->vertexBuffer_create (sizeInByte, &vtxBufferHandle))
    {
        gos::logger::err ("VulkanApp::createVertexBuffer() => gpu->vertexBuffer_create() failed\n");
        return false;
    }

    if (!gpu->vertexBuffer_copyBufferToGPU (vtxBufferHandle, 0, vertexList, sizeInByte))
    {
        gos::logger::err ("VulkanApp::createVertexBuffer() => gpu->vertexBuffer_copyBufferToGPU() failed\n");
        return false;
    }
    return true;
}

//************************************
bool VulkanExample2::recordCommandBuffer (gos::GPU *gpuIN, 
                                            const GPURenderLayoutHandle &renderLayoutHandle, 
                                            const GPUFrameBufferHandle &frameBufferHandle,
                                            const GPUPipelineHandle &pipelineHandle,
                                            const GPUVtxBufferHandle &vtxBufferHandle,
                                            VkCommandBuffer *out_commandBuffer)
{
    assert (out_commandBuffer);

    //recupero il vulkan render pass
    VkRenderPass vkRenderPassHandle = VK_NULL_HANDLE;
    if (!gpuIN->renderLayout_toVulkan (renderLayoutHandle, &vkRenderPassHandle))
    {
        gos::logger::err ("VulkanApp::recordCommandBuffer() => invalid renderLayoutHandle\n");
        return false;
    }

    //recupero il frame buffer
    VkFramebuffer vkFrameBufferHandle;
    if (!gpuIN->frameBuffer_toVulkan (frameBufferHandle, &vkFrameBufferHandle))
    {
        gos::logger::err ("VulkanApp::recordCommandBuffer() => invalid frameBufferHandle\n");
        return false;
    }

    //recupero vulkan pipeline
    VkPipeline          vkPipelineHandle;
    VkPipelineLayout    vkPipelineLayoutHandle;
    if (!gpuIN->pipeline_toVulkan (pipelineHandle, &vkPipelineHandle, &vkPipelineLayoutHandle))
    {
        gos::logger::err ("VulkanApp::recordCommandBuffer() => invalid pipelineHandle\n");
        return false;
    }

    //recupero il vtxBuffer
    VkBuffer vkVtxBuffer;
    if (!gpuIN->vertexBuffer_toVulkan (vtxBufferHandle, &vkVtxBuffer))
    {
        gos::logger::err ("VulkanApp::recordCommandBuffer() => invalid vtxBufferHandle\n");
        return false;
    }






    //uso la viewport di default di GPU che e' sempre grande tanto quanto la main window
    const gos::gpu::Viewport *viewport = gpuIN->viewport_getDefault();


    //begin command buffer
    VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0; // Optional
        beginInfo.pInheritanceInfo = nullptr; // Optional

    VkResult result = vkBeginCommandBuffer (*out_commandBuffer, &beginInfo);
    if (VK_SUCCESS != result)
    {
        gos::logger::err ("VulkanApp::recordCommandBuffer() => vkBeginCommandBuffer() => %s\n", string_VkResult(result));
        return false;
    }

    //begin render pass
    VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = vkRenderPassHandle;
        renderPassInfo.framebuffer = vkFrameBufferHandle;
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = { viewport->getW(), viewport->getH() };
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;    

    vkCmdBeginRenderPass (*out_commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    
    //bindo la pipeline
    vkCmdBindPipeline (*out_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkPipelineHandle);

    //bindo il vtx buffer a partire dal layout=0
    static const u8 VTXBUFFER__FIRST_VTX_STREAM_INDEX = 0;
    static const u8 VTXBUFFER__NUM_STREAM = 1;
    VkBuffer        vtxBufferList[VTXBUFFER__NUM_STREAM] = { vkVtxBuffer };
    VkDeviceSize    vtxBufferOffsetsList[VTXBUFFER__NUM_STREAM] = {0};    
    vkCmdBindVertexBuffers (*out_commandBuffer, VTXBUFFER__FIRST_VTX_STREAM_INDEX, VTXBUFFER__NUM_STREAM, vtxBufferList, vtxBufferOffsetsList);


    //setto la viewport
    VkViewport vkViewport {0.0f, 0.0f, viewport->getW_f32(), viewport->getH_f32(), 0.0f, 1.0f };
    vkCmdSetViewport(*out_commandBuffer, 0, 1, &vkViewport);

    VkRect2D scissor { 0, 0, viewport->getW(), viewport->getH() };
    vkCmdSetScissor (*out_commandBuffer, 0, 1, &scissor);

    //draw primitive
    vkCmdDraw (*out_commandBuffer, NUM_VERTEX, 1, 0, 0);

    //fine del render pass
    vkCmdEndRenderPass (*out_commandBuffer);

    //fine del command buffer
    result = vkEndCommandBuffer (*out_commandBuffer);
    if (VK_SUCCESS != result)
    {
        gos::logger::err ("VulkanApp::recordCommandBuffer() => vkEndCommandBuffer() => %s\n", string_VkResult(result));
        return false;
    }    
    
    return true;
}

/************************************
 * renderizza inviando command buffer a GPU e poi aspettando che questa
 * abbia finito il suo lavoro
 */
void VulkanExample2::virtual_onRun()
{
    VkCommandBuffer     vkCommandBuffer;
    gpu->createCommandBuffer (&vkCommandBuffer);

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
    bool                bNeedToRecreateSwapChain = false;
    while (!glfwWindowShouldClose (gpu->getWindow()))
    {
        if (bNeedToRecreateSwapChain)
        {
            bNeedToRecreateSwapChain = false;
            //priv_recreateFrameBuffers (gpu, renderLayoutHandle);
        }

//printf ("frame begin\n");
        frameTimer.start();
        fpsTimer.onFrameBegin();

        glfwPollEvents();

        //draw frames
        cpuWaitTimer.start();
            gpu->fence_wait (&inFlightFence);
//printf ("  CPU waited GPU fence for %ld us\n", cpuWaitTimer.elapsed_usec());

        //recupero una immagine dalla swap chain, attendo per sempre e indico [imageAvailableSemaphore] come
        //semaforo che GPU deve segnalare quando questa operazione e' ok
        acquireImageTimer.start();
            
        if (gpu->newFrame (&bNeedToRecreateSwapChain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE))
        {
            gpu->fence_reset (&inFlightFence);
//printf ("  CPU waited vkAcquireNextImageKHR %ld us\n", acquireImageTimer.elapsed_usec());
        
            //command buffer che opera su [imageIndex]
            //recordCommandBuffer(gpu, renderLayoutHandle, frameBufferHandleList[imageIndex], pipelineHandle, &vkCommandBuffer);
            recordCommandBuffer (gpu, renderLayoutHandle, frameBufferHandle, pipelineHandle, vtxBufferHandle, &vkCommandBuffer);

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

    gpu->semaphore_destroy (imageAvailableSemaphore);
    gpu->semaphore_destroy (renderFinishedSemaphore);
    gpu->fence_destroy (inFlightFence);
}

