#include "VulkanExample4.h"
#include "../gosGeom/gosGeomCamera3.h"


using namespace gos;


//************************************
VulkanExample4::VulkanExample4()
{
    anim.reset();
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
    gpu->deleteResource (uboHandle);
    gpu->deleteResource (descrSetInstancerHandle);
    gpu->deleteResource (descrSetLayoutHandle);
    gpu->deleteResource (descrPoolHandle);
}    


//************************************
bool VulkanExample4::virtual_onInit ()
{
    /*
    * 
    * TODO: Sto cercando di capire come funzionano i descriptr
    * 
    * [descriptor] è un puntatore ad una risorsa
    *       Per esempio, un "buffer descriptor" punta a un UBO, mentre un "image descriptor" punta ad una texture
    * 
    * 
    * [descriptor-set] è semplicemente una collezione di [descriptor] che vengono uppati/aggiornati tutti in un colpo solo
    * 
    * In linea di massima, crea un [descriptor-set] livello di complessita. Un classico esempio è:
    *   [descriptor-set 1] uniform buffer con dentro matV e matP    (unico upload per tutta l'intera scena)
    *   [descriptor-set 2] texture per material                     (cambiano ogni volta che cambia il materiale)
    *   [descriptor-set 3] world matrix dell'instanza del modello   (cambia ad ogni oggetto che renderizziamo)
    * 
    * 
    * 
    * [descriptor-set-layout] è un insieme di [descriptor-set].
    *       All'interno del set, bisogna indicare un "binding number" per ogni risorsa del set, a partire da 0.
    * 
    *       N [descriptor-set-layout] vanno poi bindati alla pipeline. Il primo set sara' il set 0, il secondo il set 1 e via dicendo.
    *       Esempio di pipeline con 3 set e vari binding per set:
    *           set #0 con matV @binding 0  e matP @binding 1
    *           set #1 con diffuse texture @binding 0  e specular-texture @binding 1 
    *           set #3 con model matW @ binding 0
    * 
    *  
    * 
    * [descriptor-pool] servono per allocare [descriptor-set]
    *   VkDescriptorPoolCreateInfo.maxSets = numero massimo di [descriptor-set] allocabili dal pool
    *   VkDescriptorPoolCreateInfo.poolSizeCount = num di elementi in pPoolSizes
    *   VkDescriptorPoolCreateInfo.pPoolSizes = array di VkDescriptorPoolSize ognuno dei quali indica che tipo di descriptor posso allocare (uniform, texture..) e quanti
    *                                           descriptor di quel tipo posso allocare
    * 
    */


    //vertici
    {
        u8 nv = 0;
        u32 ni = 0;
        f32 z, r, g, b;


        //front (green)
        z = -8; r = 0; g = 1; b = 0;
        vertexList[nv++].set(-1, 1, z, r, g, b);
        vertexList[nv++].set(1, 1, z, r, g, b);
        vertexList[nv++].set(1, -1, z, r, g, b);
        vertexList[nv++].set(-1, -1, z, r, g, b);
        indexList[ni++] = (nv - 4); indexList[ni++] = (nv - 3); indexList[ni++] = (nv - 2);
        indexList[ni++] = (nv - 2); indexList[ni++] = (nv - 1); indexList[ni++] = (nv - 4);

        //back (red)
        z = 1; r = 1; g = 0; b = 0;
        vertexList[nv++].set(-1, 1, z, r, g, b);
        vertexList[nv++].set(1, 1, z, r, g, b);
        vertexList[nv++].set(1, -1, z, r, g, b);
        vertexList[nv++].set(-1, -1, z, r, g, b);
        indexList[ni++] = (nv - 4); indexList[ni++] = (nv - 3); indexList[ni++] = (nv - 2);
        indexList[ni++] = (nv - 2); indexList[ni++] = (nv - 1); indexList[ni++] = (nv - 4);



    }


    //creo un cubo




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
        .addLayout (0, offsetof(Vertex, pos), eDataFormat::_3f32)        //position
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

    //Creo il descriptorSet layoutm con un solo UNIFORM BUFFER per il VTX SHADER
    gpu->descrSetLayout_createNew(&descrSetLayoutHandle)
        .add (VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
        .end();
    if (descrSetLayoutHandle.isInvalid())
    {
        gos::logger::err ("VulkanApp::init() => can't create descriptor set\n");
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
        .end() //depth stencil
        .setCullMode (eCullMode::CCW)
        .descriptor_add (descrSetLayoutHandle)
        .end ();

    if (pipelineHandle.isInvalid())
    {
        gos::logger::err ("VulkanApp::init() => can't create pipeline\n");
        return false;
    }

    //non mi serve piu'
    gpu->deleteResource (vtxDeclHandle);


    //creo un buffer per UBO
    if (!gpu->uniformBuffer_create (sizeof(sUniformBufferObject), &uboHandle))
    {
        gos::logger::err ("VulkanApp::init() => GPU::uniformBuffer_create\n");
        return false;
    }


    //creo un descriptor pool
    gpu->descrPool_createNew (&descrPoolHandle)
        .setMaxNumDescriptorSet(4)
        .addPool_uniformBuffer()
        .end();
    if (descrPoolHandle.isInvalid())
    {
        gos::logger::err ("VulkanApp::init() => can't create descriptor pool\n");
        return false;
    }

    //alloco una istanza del descriptorSet
    if (!gpu->descrSetInstance_createNew (descrPoolHandle, descrSetLayoutHandle, &descrSetInstancerHandle))
    {
        gos::logger::err ("VulkanApp::init() => can't create descriptorSet instance\n");
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
bool VulkanExample4::recordCommandBuffer (VkCommandBuffer *out_commandBuffer)
{
    assert (out_commandBuffer);


    //aggiorno UBO
    gos::gpu::DescrSetInstanceWriter descrWriter;
    descrWriter.begin (gpu, descrSetInstancerHandle)
        .updateUniformBuffer (0, uboHandle)
        .end();


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

    //recupero il descrSetInstance
    VkDescriptorSet vkDescrSetHandle;
    if (!gpu->toVulkan (descrSetInstancerHandle, &vkDescrSetHandle))
    {
        gos::logger::err ("VulkanApp::recordCommandBuffer() => invalid descrSetInstace handle\n");
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
    VkClearValue clearColor = {{{0.0f, 0.0f, 0.01f, 1.0f}}};
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


    //bindo il descrSetLayout
    vkCmdBindDescriptorSets (*out_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkPipelineLayoutHandle, 0, 1, &vkDescrSetHandle, 0, nullptr);


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
        if (timeNow_msec >= anim.nextTimeRotate_msec)
        {
            mat4x4f matT;
            mat4x4f matR;

            anim.nextTimeRotate_msec = timeNow_msec + 16;
            anim.rotation_grad+=1.0f;
anim.rotation_grad=2.0f;
            anim.zPos += anim.zInc;
            if (anim.zPos >= 10 || anim.zPos < 0)
                anim.zInc = -anim.zInc;
            
            matR.buildRotationAboutY (math::gradToRad(anim.rotation_grad));
            matT.buildTranslation (0,0,anim.zPos);
            ubo.world = matT * matR;
//            ubo.world.identity();


            gos::geom::Camera3 cam;
            cam.setPerspectiveFovLH(gpu->swapChain_calcAspectRatio(),  math::gradToRad(45), 0.1f, 50.0f);
            cam.pos.identity();
            cam.pos.warp (0, 1, -19);
            cam.pos.lookAt (vec3f(0,0,0));
            cam.markUpdated();
            ubo.view = cam.getMatV();
            ubo.proj = cam.getMatP();


            



            vec4f vIN[4];
            vIN[0].set (0,0,0,1);
            vIN[1].set (0,0,1,1);
            vIN[2].set (0,0,10,1);
            vIN[3].set (0,0,100,1);
            vec4f vOUT[4];
            for (u32 i = 0; i < 4; i++)
            {
                //vec4f v (vertexList[i].pos.x, vertexList[i].pos.y, vertexList[i].pos.z, 1);
                //vOUT[i] = math::vecTransform (ubo.proj, v);
                vOUT[i] = math::vecTransform (ubo.proj, vIN[i]);
            }
            vOUT[0].w = 1;


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

