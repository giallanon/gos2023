#include "VulkanExample2.h"


using namespace gos;


//************************************
VulkanExample2::VulkanExample2()
{
    nextTimeMoveVtx_msec = 0;
    direction = -1;
    ptToMappedMemory = NULL;
}

//************************************
void VulkanExample2::virtual_explain()
{
    gos::logger::log ("esperimenti con Vtx Buffer di tipo 'mappable'");
}


//************************************
void VulkanExample2::virtual_onCleanup() 
{
    if (NULL != ptToMappedMemory)
    {
        gpu->vertexBuffer_unmap (vtxBufferHandle);
        ptToMappedMemory = NULL;
    }
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
        .requireRendertarget (eRenderTargetUsage::presentation, gpu->swapChain_getImageFormat(), true)
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
    if (!gpu->vertexBuffer_create (sizeInByte, eVIBufferMode::mappale, &vtxBufferHandle))
    {
        gos::logger::err ("VulkanApp::createVertexBuffer() => gpu->vertexBuffer_create() failed\n");
        return false;
    }

    //mappo il vtxBuffer in una regione di spazio accessibile a CPU
    //Se memcopio roba qui dentro, GPU se ne accorge (vedi moveVertex())
    if (!gpu->vertexBuffer_map (vtxBufferHandle, 0, sizeInByte, &ptToMappedMemory))
    {
        gos::logger::err ("VulkanApp::createVertexBuffer() => gpu->vertexBuffer_Map() failed\n");
        return false;
    }

    return true;
}


//************************************
void VulkanExample2::moveVertex()
{
    const u64 timeNow_msec = gos::getTimeSinceStart_msec();
    if (timeNow_msec < nextTimeMoveVtx_msec)
        return;
    nextTimeMoveVtx_msec = timeNow_msec + 15;
    
    vertexList[0].pos.y += direction* 0.01f;
    if (vertexList[0].pos.y <= -1.0f)
        direction = 1;
    else if (vertexList[0].pos.y >= -0.5f)
        direction = -1;

    //copio i vtx modificati nella zona mappata del vtxBuffer
    const u32 sizeInByte = sizeof(Vertex) * NUM_VERTEX;
    memcpy (ptToMappedMemory, vertexList, sizeInByte);
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

    //recupero il vtxBuffer
    VkBuffer vkVtxBuffer;
    if (!gpuIN->toVulkan (vtxBufferHandle, &vkVtxBuffer))
    {
        gos::logger::err ("VulkanApp::recordCommandBuffer() => invalid vtxBufferHandle\n");
        return false;
    }


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
        renderPassInfo.renderArea.extent = { renderAreaW, renderAreaH };
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
    const gos::gpu::Viewport *viewport = gpuIN->viewport_get(gpuIN->viewport_getDefault());
    VkViewport vkViewport {0.0f, 0.0f, (viewport->getW_f32()), viewport->getH_f32(), 0.0f, 1.0f };
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
    mainLoop_3();
}

//**********************************
void VulkanExample2::doCPUStuff ()
{
    fpsMegaTimer.onFrameBegin(FPSTIMER_CPU);

    glfwPollEvents();

    moveVertex();

    //do stuff
    i32 tot = 0;
    for (u32 i=0; i<1000; i++)
    {
        const f32 r1 = gos::random01() * 100000.0f;
        const f32 r2 = gos::random01() * 100000.0f;
        
        if (sqrtf (r1 * r1) - r2*r2 < 0)
            tot++;
        else
            tot--;
    }
    if (tot < 0)
        printf ("A\n");


    fpsMegaTimer.onFrameEnd(FPSTIMER_CPU);
    fpsMegaTimer.printReport();
}


//**********************************
void VulkanExample2::mainLoop_3()
{
    GPUCmdBufferHandle  cmdBufferHandle;
    gpu->cmdBuffer_create (eGPUQueueType::gfx, &cmdBufferHandle);
        
    //I semafori sono oggetti di sync tra GPU & GPU (non e' un errore e' proprio GPU-GPU)
    //Fence sono oggetti di sync tra GPU & CPU (a differenza dei semafori che riguardano solo la CPU)
    VkSemaphore         semaphore_renderFinished;
    //VkSemaphore         semaphore_imageReady;
    VkFence             inFlightFence;
    VkFence             fenceSwapChainReady;
    gpu->semaphore_create (&semaphore_renderFinished);
    //gpu->semaphore_create (&semaphore_imageReady);
    gpu->fence_create (true, &inFlightFence);
    gpu->fence_create (false, &fenceSwapChainReady);


    VkResult            result;
    while (!glfwWindowShouldClose (gpu->getWindow()))
    {
        doCPUStuff ();
        //attende che il precedente batch sia terminato
        if (gpu->fence_wait (inFlightFence, 0))
        {
            fpsMegaTimer.onFrameEnd (FPSTIMER_GPU);
            //Chiedo a GPU una immagine dalla swap chain, non attendo nemmeno 1 attimo e indico [semaphore_imageReady] come
            //semaforo che GPU deve segnalare quando questa operazione e' ok. Indico inoltre [fenceSwapChainReady] come fence da segnalre
            //quando l'immagine e' disponibile
            //Questa fn ritorna quando GPU e' in grado di determinare quale sara' la prossima immagine sulla quale renderizzare.
            //Quando GPU ha questa informazione, non vuol dire pero' che l'immagine e' gia' immediatamente disponibile per l'uso.
            //E' per questo che si usa [semaphore_imageReady] e [fenceSwapChainReady], per sapere quando davvero l'immagine sara' disponibile
            if (gpu->newFrame (0, VK_NULL_HANDLE, fenceSwapChainReady))
            {
                //A questo GPU ha capito quale sara' l'immagine che prima o poi mi dara', ma non e' detto che questa sia gia' disponibile
                //Lo diventa quando [fenceSwapChainReady] e' segnalata.
                //Fino ad allora posso farmi i fatti miei

                //Intanto che aspetto che GPU renda disponibile una immagine, faccio le mie cose
                while (!gpu->fence_wait (fenceSwapChainReady, 0))
                {
                    doCPUStuff ();
                }

                fpsMegaTimer.onFrameEnd(FPSTIMER_FPS);
                fpsMegaTimer.onFrameBegin(FPSTIMER_FPS);

                //arrivo qui quando GPU mi ha finalmente dato l'immagine
                fpsMegaTimer.onFrameBegin(FPSTIMER_GPU);

                gpu->fence_reset (fenceSwapChainReady);
                gpu->fence_reset (inFlightFence);
            
                //command buffer: indica il lavoro che GPU deve fare
                VkCommandBuffer vkCommandBuffer;
                gpu->toVulkan (cmdBufferHandle, &vkCommandBuffer);
                recordCommandBuffer (gpu, renderLayoutHandle, frameBufferHandle, pipelineHandle, vtxBufferHandle, &vkCommandBuffer);

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
                        submitInfo.pCommandBuffers = &vkCommandBuffer;

                        //semaforo che GPU segnalera' al termine dell'esecuzione di questo batch di lavoro
                        submitInfo.signalSemaphoreCount = 1;
                        submitInfo.pSignalSemaphores = &semaphore_renderFinished;

                        //submitto il batch a GPU e indico che deve segnalare [inFlightFence] quando ha finito 
                        result = vkQueueSubmit (gpu->REMOVE_getGfxQHandle(), 1, &submitInfo, inFlightFence);
                        if (VK_SUCCESS != result)
                            gos::logger::err ("vkQueueSubmit() => %s\n", string_VkResult(result));
                }

                //presentazione
                //Indico a GPU che deve attendere [renderFinishedSemaphore] prima di presentare
                gpu->present (&semaphore_renderFinished, 1);
            }
        }
    }

    //aspetto che GPU abbia finito tutto cio' che ha in coda
    gpu->waitIdle();

    gpu->deleteResource (cmdBufferHandle);
    //gpu->semaphore_destroy (semaphore_imageReady);
    gpu->semaphore_destroy (semaphore_renderFinished);
    gpu->fence_destroy (fenceSwapChainReady);
    gpu->fence_destroy (inFlightFence);
}

