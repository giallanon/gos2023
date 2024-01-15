#include "VulkanExample4.h"


using namespace gos;


//************************************
VulkanExample4::VulkanExample4()
{
    nextTimeRotate_msec = 0;
    rotation_grad = 0;
}

//************************************
void VulkanExample4::virtual_explain()
{
    gos::logger::log ("Esperimenti con Uniform buffer\n");
}


//************************************
void VulkanExample4::virtual_onCleanup() 
{
    gpu->deleteResource (idxBufferHandle);
    gpu->deleteResource (stgBufferHandle);
    gpu->deleteResource (vtxBufferHandle);
    gpu->deleteResource (vtxShaderHandle);
    gpu->deleteResource (fragShaderHandle);
    gpu->deleteResource (pipelineHandle);
    gpu->deleteResource (renderLayoutHandle);
    gpu->deleteResource (frameBufferHandle);
    gpu->deleteResource (descrLayoutHandle);
    gpu->deleteResource (uboHandle);
}    


//************************************
bool VulkanExample4::virtual_onInit ()
{
    /*
    * 
    * TODO: Sto cercando di capire come funzionano i descriptr
    * 
    * VkDescriptorSetLayout     descrive il layout di un descriptorSet
    *                           il layout indica il binding (quale shader puo' usarelo e in quale binding slot)
    * 
    *
    * VkDescriptorPool          pool dal quale allocare un descriptorSet
    *
    VkDescriptorPoolSize poolSize{};
        poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSize.descriptorCount = 16;

    VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = 1;
        poolInfo.pPoolSizes = &poolSize;                                    //num max Descriptor allocabili per ogni tipo di pool
        poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);     //num max DescriptorSET allocabili

        */














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
    fs::addAlias ("@shader", "shader/example4", eAliasPathMode::relativeToAppFolder);
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
    {
        if (!createDescriptorSetLayout (gpu, &descrLayoutHandle))
            return false;

        gpu->pipeline_createNew (renderLayoutHandle, &pipelineHandle)
            .addShader (vtxShaderHandle)
            .addShader (fragShaderHandle)
            .setVtxDecl (vtxDeclHandle)
            .depthStencil()
                .zbuffer_enable(true)
                .zbuffer_enableWrite(true)
                .zbuffer_setFn (eZFunc::LESS)
                .stencil_enable(false)
            .end() //depth stencil
            .descriptor_add (descrLayoutHandle)
            .end ();

        if (pipelineHandle.isInvalid())
        {
            gos::logger::err ("VulkanApp::init() => can't create pipeline\n");
            return false;
        }
    }

    //non mi serve piu'
    gpu->deleteResource (vtxDeclHandle);


    //creo un buffer per UBO
    if (!gpu->uniformBuffer_create (sizeof(sUniformBufferObject), &uboHandle))
    {
        gos::logger::err ("VulkanApp::init() => GPU::uniformBuffer_create\n");
        return false;
    }


    return true;
}    


//************************************
bool VulkanExample4::createVertexIndexStageBuffer()
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
    void  *ptToMappedStagingBuffer;
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
bool VulkanExample4::copyIntoVtxBuffer()
{
    const u32 sizeInByte = sizeof(Vertex) * NUM_VERTEX;

    //apparentemente non e' necessario mappare/unmappare lo stagin buffer ogni volta
    //gpu->stagingBuffer_unmap (stgBufferHandle);    ptToMappedMemory = NULL;

    //copio i Vtx nello staging array
    void  *ptToMappedStagingBuffer;
    if (!gpu->stagingBuffer_map (stgBufferHandle, 0, sizeInByte, &ptToMappedStagingBuffer))
    {
        gos::logger::err ("VulkanApp::copyIntoVtxBuffer() => gpu->stagingBuffer_map() failed\n");
        return false;
    } 
    memcpy (ptToMappedStagingBuffer, vertexList, sizeInByte);


    gpu->stagingBuffer_copyToBuffer (stgBufferHandle, vtxBufferHandle, 0, 0, sizeInByte);
    
    return true;
}



//************************************
bool VulkanExample4::createDescriptorSetLayout (GPU *gpu, GPUDescrLayoutHandle *out)
{
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    uboLayoutBinding.pImmutableSamplers = nullptr; // Optional


    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &uboLayoutBinding;

    if (!gpu->descrLayout_create (layoutInfo, out))
    {
        gos::logger::err ("VulkanApp::createDescriptorSetLayout() => gpu->descrLayout_create failed => \n");
        return false;
    }

    return true;
}

//************************************
bool VulkanExample4::recordCommandBuffer (VkCommandBuffer *out_commandBuffer)
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
void VulkanExample4::virtual_onRun()
{
    mainLoop();
}

//**********************************
void VulkanExample4::doCPUStuff ()
{
    fpsMegaTimer.onFrameBegin(FPSTIMER_CPU);

    glfwPollEvents();

    //prepare frame
    {
        const u64 timeNow_msec = gos::getTimeSinceStart_msec();
        if (timeNow_msec >= nextTimeRotate_msec)
        {
            nextTimeRotate_msec = timeNow_msec + 300;
            rotation_grad+=1.0f;
            ubo.world.identity();
            ubo.world.buildRotationAboutY (math::gradToRad(rotation_grad));

            ubo.view.buildLookAt (gos::vec3f(2.0f, 2.0f, -2.0f), gos::vec3f(0.0f, 0.0f, 0.0f), gos::vec3f(0.0f, 1.0f, 0.0f));

            ubo.proj.buildPerspective (gpu->swapChain_calcAspectRatio(),  math::gradToRad(45), 0.1f, 10.0f);
        }
        gpu->uniformBuffer_mapCopyUnmap (uboHandle, 0, sizeof(sUniformBufferObject), &ubo);
    }

    //do some stuff
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
void VulkanExample4::mainLoop()
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

