#include "VulkanExample3.h"


using namespace gos;


//************************************
VulkanExample3::VulkanExample3()
{
    nextTimeMoveVtx_msec = 0;
    direction = -1;
    ptToMappedStagingBuffer = NULL;
}

//************************************
void VulkanExample3::virtual_explain()
{
    gos::logger::log ("Introduzione della class GPUMainLoop, che si occupa di gestire la sync con i frame!\n");
    gos::logger::log ("Esperimenti con Vtx Buffer di tipo 'GPU only'\n");
    gos::logger::log ("Introduzione idx buffer\n");
}


//************************************
void VulkanExample3::virtual_onCleanup() 
{
    gpu->deleteResource (idxBufferHandle);
    gpu->deleteResource (stgBufferHandle);
    gpu->deleteResource (vtxBufferHandle);
    gpu->deleteResource (vtxShaderHandle);
    gpu->deleteResource (fragShaderHandle);
    gpu->deleteResource (pipelineHandle);
    gpu->deleteResource (renderLayoutHandle);
    gpu->deleteResource (frameBufferHandle);
}    


//************************************
bool VulkanExample3::virtual_onInit ()
{
    //vertici
    u32 n=0;
    vertexList[n++].set(-0.5f, -0.5f,       1.0f, 0.0f, 0.0f);
    vertexList[n++].set(0.5f, -0.5f,        0.0f, 1.0f, 0.0f);
    vertexList[n++].set(0.5f, 0.5f,         0.0f, 0.0f, 1.0f);
    vertexList[n++].set(-0.5f, 0.5f,        1.0f, 1.0f, 1.0f);
    assert (n==NUM_VERTEX);

    n = 0;
    indexList[n++] = 0;
    indexList[n++] = 1;
    indexList[n++] = 2;
    indexList[n++] = 2;
    indexList[n++] = 3;
    indexList[n++] = 0;
    assert (n==NUM_INDEX);


    if (!createVertexIndexStageBuffer())
    {
        gos::logger::err ("VulkanApp::init() => can't create buffers\n");
        return false;
    }

    copyIntoVtxBuffer ();

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
    fs::addAlias ("@shader", "shader/example3", eAliasPathMode::relativeToAppFolder);
    if (!gpu->vtxshader_createFromFile ("@shader/shader.vert.spv", "main", &vtxShaderHandle))
    {
        gos::logger::err ("VulkanApp::init() => can't create vert shader\n");
        return false;
    }
    if (!gpu->fragshader_createFromFile ("@shader/shader.frag.spv", "main", &fragShaderHandle))
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
bool VulkanExample3::createVertexIndexStageBuffer()
{
    const u32 sizeInByte = sizeof(Vertex) * NUM_VERTEX;
    if (!gpu->vertexBuffer_create (sizeInByte, eVIBufferMode::onGPU, &vtxBufferHandle))
    {
        gos::logger::err ("VulkanApp::createVertexIndexStageBuffer() => gpu->vertexBuffer_create() failed\n");
        return false;
    }

    //Creo anche uno staging buffer
    if (!gpu->stagingBuffer_create (sizeInByte, &stgBufferHandle))
    {
        gos::logger::err ("VulkanApp::createVertexIndexStageBuffer() => gpu->stagingBuffer_create() failed\n");
        return false;
    }

    if (!gpu->indexBuffer_create (sizeof(u16)*NUM_INDEX, eVIBufferMode::onGPU, &idxBufferHandle))
    {
        gos::logger::err ("VulkanApp::createVertexIndexStageBuffer() => gpu->indexBuffer_create() failed\n");
        return false;
    }


    //Mappo lo staging buffer
    if (!gpu->stagingBuffer_map (stgBufferHandle, 0, sizeInByte, &ptToMappedStagingBuffer))
    {
        gos::logger::err ("VulkanApp::createVertexIndexStageBuffer() => gpu->stagingBuffer_map() failed\n");
        return false;
    } 


    // ci copio gli index
    memcpy (ptToMappedStagingBuffer, indexList, sizeof(u16) * NUM_INDEX);

    //copio stage buffer in GPU
    if (!gpu->stagingBuffer_copyToBuffer (stgBufferHandle, idxBufferHandle, 0, 0, sizeof(u16) * NUM_INDEX))
    {
        gos::logger::err ("VulkanApp::createVertexIndexStageBuffer() => gpu->stagingBuffer_copyToBuffer() failed\n");
        return false;
    } 

    return true;
}

//************************************
bool VulkanExample3::copyIntoVtxBuffer()
{
    const u32 sizeInByte = sizeof(Vertex) * NUM_VERTEX;

    //apparentemente non e' necessario mappare/unmappare lo stagin buffer ogni volta
    //gpu->stagingBuffer_unmap (stgBufferHandle);    ptToMappedMemory = NULL;

    //copio i Vtx nello staging array
    memcpy (ptToMappedStagingBuffer, vertexList, sizeInByte);


    gpu->stagingBuffer_copyToBuffer (stgBufferHandle, vtxBufferHandle, 0, 0, sizeInByte);
    
    return true;
}

//************************************
void VulkanExample3::moveVertex()
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
}

//************************************
bool VulkanExample3::recordCommandBuffer (VkCommandBuffer *out_commandBuffer)
{
    assert (out_commandBuffer);

    //recupero il vulkan render pass
    VkRenderPass vkRenderPassHandle = VK_NULL_HANDLE;
    if (!gpu->toVulkan (renderLayoutHandle, &vkRenderPassHandle))
    {
        gos::logger::err ("VulkanApp::recordCommandBuffer() => invalid renderLayoutHandle\n");
        return false;
    }

    //recupero il frame buffer
    VkFramebuffer vkFrameBufferHandle;
    u32 renderAreaW;
    u32 renderAreaH;
    if (!gpu->toVulkan (frameBufferHandle, &vkFrameBufferHandle, &renderAreaW, &renderAreaH))
    {
        gos::logger::err ("VulkanApp::recordCommandBuffer() => invalid frameBufferHandle\n");
        return false;
    }

    //recupero vulkan pipeline
    VkPipeline          vkPipelineHandle;
    VkPipelineLayout    vkPipelineLayoutHandle;
    if (!gpu->toVulkan (pipelineHandle, &vkPipelineHandle, &vkPipelineLayoutHandle))
    {
        gos::logger::err ("VulkanApp::recordCommandBuffer() => invalid pipelineHandle\n");
        return false;
    }

    //recupero il vtxBuffer
    VkBuffer vkVtxBuffer;
    if (!gpu->toVulkan (vtxBufferHandle, &vkVtxBuffer))
    {
        gos::logger::err ("VulkanApp::recordCommandBuffer() => invalid vtxBufferHandle\n");
        return false;
    }

    //recupero il idxBuffer
    VkBuffer vkIdxBuffer;
    if (!gpu->toVulkan (idxBufferHandle, &vkIdxBuffer))
    {
        gos::logger::err ("VulkanApp::recordCommandBuffer() => invalid idxBufferHandle\n");
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

    //bindo idxBuffer
    vkCmdBindIndexBuffer (*out_commandBuffer, vkIdxBuffer, 0, VK_INDEX_TYPE_UINT16);


    //setto la viewport
    const gos::gpu::Viewport *viewport = gpu->viewport_getDefault();
    VkViewport vkViewport {0.0f, 0.0f, (viewport->getW_f32()), viewport->getH_f32(), 0.0f, 1.0f };
    vkCmdSetViewport(*out_commandBuffer, 0, 1, &vkViewport);

    VkRect2D scissor { 0, 0, viewport->getW(), viewport->getH() };
    vkCmdSetScissor (*out_commandBuffer, 0, 1, &scissor);

    //draw primitive:
    //      num-index => num di vertici che verranno passati al vxtshader
    //      num-instances => 1 come minimo
    //      ofsset-idxBuffer  => passare 1 significa che si parte dall'indice [1] dell'idxBuffer (quindi non parliamo di un offset in byte)
    //      index-base = specifies an offset to add to the indices in the index buffer.
    //      offset-instancin = non lo so...
    vkCmdDrawIndexed(*out_commandBuffer, NUM_INDEX, 1, 0, 0, 0);

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
void VulkanExample3::virtual_onRun()
{
    mainLoop();
}

//**********************************
void VulkanExample3::doCPUStuff ()
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
void VulkanExample3::mainLoop()
{
    GPUMainLoop gpuLoop;
    gpuLoop.setup (gpu, &fpsMegaTimer);

    //command buffer 
    VkCommandBuffer         vkCommandBuffer_GFX;
    gpu->createCommandBuffer (eGPUQueueType::gfx, &vkCommandBuffer_GFX);

    //main loop
    while (!glfwWindowShouldClose (gpu->getWindow()))
    {
        doCPUStuff ();


        gpuLoop.run ();
        if (gpuLoop.canSubmitGFXJob())
        {
            copyIntoVtxBuffer();
            recordCommandBuffer (&vkCommandBuffer_GFX);
            gpuLoop.submitGFXJob (vkCommandBuffer_GFX);
        }

    }

    //aspetto che GPU abbia finito tutto cio' che ha in coda
    gpu->waitIdle();

    //free
    gpu->deleteCommandBuffer (eGPUQueueType::gfx, vkCommandBuffer_GFX);
    gpuLoop.unsetup();
}

