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

    //pipeline
    gpu::pipe2::Pipeline_def def;
    def
        .reset()
        .set_cullMode (eCullMode::CCW)
        .set_drawPrimitive (eDrawPrimitive::trisList)
        .shader_add (vtxShaderHandle)
        .shader_add (fragShaderHandle)
        .add_rt (eImageFormat::_SAME_AS_CURRENT_SWAPCHAIN)
        .vtxStream_add (eVtxStreamInputRate::perVertex)
            .add(0, offsetof(Vertex, pos), eDataFormat::_2f32)
            .add(1, offsetof(Vertex, colorRGB), eDataFormat::_3f32)
            .endVtxStream();


    if (!gpu->pipeline_createNew (def, &pipelineHandle))
    {
        gos::logger::err ("VulkanApp::init() => can't create pipeline\n");
        return false;
    };

    return true;
}    


//************************************
bool VulkanExample3::createVertexIndexStageBuffer()
{
    const u32 sizeInByte = sizeof(Vertex) * NUM_VERTEX;
    if (!gpu->vertexBuffer_create (sizeInByte, eMemAccessMode::onGPU, &vtxBufferHandle))
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

    if (!gpu->indexBuffer_create (sizeof(u16)*NUM_INDEX, eMemAccessMode::onGPU, &idxBufferHandle))
    {
        gos::logger::err ("VulkanApp::createVertexIndexStageBuffer() => gpu->indexBuffer_create() failed\n");
        return false;
    }


    //copio gli indici nell'idxBuffer tramite uno staging buffer
    if (!gpu->stagingBuffer_uploadToGPUBuffer (stgBufferHandle, indexList, idxBufferHandle, 0, sizeof(u16) * NUM_INDEX))
    {
        gos::logger::err ("VulkanApp::createVertexIndexStageBuffer() => gpu->stagingBuffer_uploadToGPUBuffer() failed\n");
        return false;
    }

    return true;
}

//************************************
bool VulkanExample3::copyIntoVtxBuffer()
{
    //copio i Vtx in vtxBuffer tramite lo staging array
    const u32 sizeInByte = sizeof(Vertex) * NUM_VERTEX;
    return gpu->stagingBuffer_uploadToGPUBuffer (stgBufferHandle, vertexList, vtxBufferHandle, 0, sizeInByte);

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
bool VulkanExample3::recordCommandBuffer (GPUCmdBufferHandle &cmdBufferHandle, gpu::SwapchainImg &swapChainImage)
{
    gos::gpu::pipe2::CmdBufferWriter2 cw;
    cw
        .begin (gpu, cmdBufferHandle)
        .setViewport (gpu->viewport_getDefault())
        .imageTransition (swapChainImage.image, eImageLayout::undefined, eImageLayout::color_attachment_optimal)
        .beginRender()
            .withRenderArea (gpu->swapChain_getWidth(), gpu->swapChain_getHeight())
            .withRT (swapChainImage.imageView, eAttachmentLoadOp::clear, eAttachmentStoreOp::dont_care, gos::ColorHDR(0,0,0))
            .bindPipeline (pipelineHandle)
            .bindVtxBuffer(vtxBufferHandle)
            .bindIdxBufferU16(idxBufferHandle)

            //draw primitive:
            //      num-index => num di vertici che verranno passati al vxtshader
            //      num-instances => 1 come minimo
            //      ofsset-idxBuffer  => passare 1 significa che si parte dall'indice [1] dell'idxBuffer (quindi non parliamo di un offset in byte)
            //      index-base = specifies an offset to add to the indices in the index buffer.
            //      offset-instancin = non lo so...            
            .drawIndexed(NUM_INDEX, 1, 0, 0, 0)
            
            .endRender()
        .imageTransition (swapChainImage.image, eImageLayout::color_attachment_optimal, eImageLayout::presentation)
        .end();

        
    return true;
}

//**********************************
void VulkanExample3::doCPUStuff ()
{
    handleInput();

    //prepare vtx
    moveVertex();

    //do some other stuff
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
}


/************************************
 * renderizza inviando command buffer a GPU e poi aspettando che questa
 * abbia finito il suo lavoro
 */
void VulkanExample3::virtual_onRun()
{
    gpu::MainLoop2 mainLoop;
    mainLoop.setup (gpu);


    //command buffer 
    GPUCmdBufferHandle  cmdBufferHandle;
    gpu->cmdBuffer_create (eGPUQueueType::gfx, &cmdBufferHandle);


    //main loop
    while (bQuitApp == false)
    {
        mainLoop.stat_onCPUFrameBegin();
        doCPUStuff();
        mainLoop.stat_onCPUFrameEnd();


        mainLoop.run();

        //se il job precedente e' stato presentato, posso schedularne uno nuovo
        gpu::SwapchainImg swapchainImg;
        if (mainLoop.gfxJob_canSubmit(&swapchainImg))
        {
            copyIntoVtxBuffer();
            recordCommandBuffer (cmdBufferHandle, swapchainImg);
            mainLoop.gfxJob_submitAndPresent (cmdBufferHandle, swapchainImg);
        }
    }

    //aspetto che GPU abbia finito tutto cio' che ha in coda
    gpu->waitIdle();

    //free
    gpu->deleteResource (cmdBufferHandle);
}


