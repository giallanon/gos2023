#include "VulkanExample2.h"


using namespace gos;


//************************************
VulkanExample2::VulkanExample2()
{
    nextTimeMoveVtx_msec = 0;
    direction = -1;
}

//************************************
void VulkanExample2::virtual_explain()
{
    gos::logger::log ("esperimenti con Vtx Buffer di tipo 'mappable'");
}


//************************************
void VulkanExample2::virtual_onCleanup() 
{
    gpu->deleteResource (vtxBufferHandle);
    gpu->deleteResource (vtxShaderHandle);
    gpu->deleteResource (fragShaderHandle);
    gpu->deleteResource (pipelineHandle);
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


    //vertex buffer
    if (!gpu->vertexBuffer_create (sizeof(Vertex) * NUM_VERTEX, eMemAccessMode::shared_cpuW_autoSync, &vtxBufferHandle))
    {
        gos::logger::err ("VulkanApp::virtual_onInit() => gpu->vertexBuffer_create() failed\n");
        return false;
    }


    //carico gli shader
    if (!gpu->vtxshader_createFromFile ("shader/example2/shader.vert.spv", "main", &vtxShaderHandle))
    {
        gos::logger::err ("VulkanApp::init() => can't create vert shader\n");
        return false;
    }
    if (!gpu->pxlshader_createFromFile ("shader/example2/shader.frag.spv", "main", &fragShaderHandle))
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
            .add(0, 0, eDataFormat::_2f32)
            .add(1, 8, eDataFormat::_3f32)
            .endVtxStream();


    if (!gpu->pipeline_createNew (def, &pipelineHandle))
    {
        gos::logger::err ("VulkanApp::init() => can't create pipeline\n");
        return false;
    };

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
    gpu->writeAndSync (vtxBufferHandle, 0, vertexList, sizeInByte);
}

//************************************
bool VulkanExample2::recordCommandBuffer (GPUCmdBufferHandle &cmdBufferHandle, gpu::SwapchainImg &swapChainImage)
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
            .draw(NUM_VERTEX, 1, 0, 0)
            .endRender()
        .imageTransition (swapChainImage.image, eImageLayout::color_attachment_optimal, eImageLayout::presentation)
        .end();

        
    return true;
}

//**********************************
void VulkanExample2::doCPUStuff ()
{
    handleInput();

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
}

/************************************
 * renderizza inviando command buffer a GPU e poi aspettando che questa
 * abbia finito il suo lavoro
 */
void VulkanExample2::virtual_onRun()
{
    gpu::MainLoop2 mainLoop;
    mainLoop.setup (gpu);


    //command buffer 
    GPUCmdBufferHandle  cmdBufferHandle;
    gpu->cmdBuffer_create (eGPUQueueFamily::gfx, &cmdBufferHandle);


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
            recordCommandBuffer (cmdBufferHandle, swapchainImg);
            mainLoop.gfxJob_submitAndPresent (cmdBufferHandle, swapchainImg);
        }
    }

    //aspetto che GPU abbia finito tutto cio' che ha in coda
    gpu->waitIdle();

    //free
    gpu->deleteResource (cmdBufferHandle);
}

