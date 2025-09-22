#include "VulkanExample7.h"



using namespace gos;


//************************************
void VulkanExample7::virtual_explain()
{
}

//************************************
void VulkanExample7::virtual_onCleanup() 
{
    theHub.unload (assetPipe1);
    theHub.unload (assetPipe2);

    gpu->deleteResource(vtxBufferHandle);
    gpu->deleteResource(idxBufferHandle);
    gpu->deleteResource(rt1);
}    


//************************************
bool VulkanExample7::virtual_onInit ()
{
    //builder per ricompilare gli asset se necessario
    {
        gos::asset::Builder builder;
        builder.buildAll("shader/example7", true);
    }

    //theHub
    theHub.setup ("shader/example7", gpu);
    theHub.getHandle("pipe_1", &assetPipe1);
    theHub.getHandle("pipe_2", &assetPipe2);

    const asset::Asset_pipe *thePipe;
    theHub.getAssetWithTimeout (assetPipe1, 5000, &thePipe);

    //risorse di rendering
    const eImageFormat IMG_FORMAT = eImageFormat::U8_RGBA;
    if (!gpu->renderTarget_create ("0-", "0-", IMG_FORMAT, &rt1))
        return false;


    //vtx buffer
    const gpu::Pipeline2 *pipeInfo = NULL;
    gpu->toVulkan(thePipe->handle_pipe, &pipeInfo);
    if (!gpu->vertexBuffer_create (32 * pipeInfo->vtx_stridePerStream[0], eVIBufferMode::shared_cpuW_autoSync, &vtxBufferHandle))
    {
        gos::logger::err ("VulkanApp::virtual_onInit() => gpu->vertexBuffer_create() failed\n");
        return false;
    }

    //INDEX BUFFER
    if (!gpu->indexBuffer_create (64 * sizeof(u16), eVIBufferMode::shared_cpuW_autoSync, &idxBufferHandle))
    {
        gos::logger::err ("VulkanApp::virtual_onInit() => gpu->indexBuffer_create() failed\n");
        return false;
    }


    //tris 1
    {
        struct Vertex1
        {
            u32     x;
            u32     y;
            vec3f   rgb;
        };

        Vertex1 vtxSRC[8];

        vtxSRC[0].x = 1;   vtxSRC[0].y = 1;   vtxSRC[0].rgb.set(1,0,0);
        vtxSRC[1].x = 100; vtxSRC[1].y = 1;   vtxSRC[1].rgb.set(0,1,0);
        vtxSRC[2].x = 100; vtxSRC[2].y = 200; vtxSRC[2].rgb.set(0,0,1);
        vtxSRC[3].x = 1;   vtxSRC[3].y = 200; vtxSRC[3].rgb.set(1,1,0);
        gpu->writeAndSync (vtxBufferHandle, 0, vtxSRC, sizeof(Vertex1) * 4);

        u16 indexSRC[6] = { 0, 1, 2,    2, 3, 0 };
        gpu->writeAndSync (idxBufferHandle, 0, indexSRC, sizeof(u16) * 6);
    }

    {
        struct Vertex2
        {
            u32     x;
            u32     y;
            vec2f   tutv;
        };
        Vertex2 vtxSRC[8];

        vtxoffset2 = sizeof(Vertex2)*8;
            vtxSRC[0].x = 300; vtxSRC[0].y = 1;   vtxSRC[0].tutv.set (0,0);
            vtxSRC[1].x = 500; vtxSRC[1].y = 1;   vtxSRC[1].tutv.set (1,0);
            vtxSRC[2].x = 500; vtxSRC[2].y = 200; vtxSRC[2].tutv.set (1,1);
            vtxSRC[3].x = 300; vtxSRC[3].y = 200; vtxSRC[3].tutv.set (0,1);
            gpu->writeAndSync (vtxBufferHandle, vtxoffset2, vtxSRC, sizeof(Vertex2) * 4);

        idxoffset2 = sizeof(u16)*8;
            const u16 indexSRC[6] = { 0, 1, 2,    2, 3, 0 };
            gpu->writeAndSync (idxBufferHandle, idxoffset2, indexSRC, sizeof(u16) * 6);
    }



    return true;
}    


//************************************
bool VulkanExample7::recordCommandBuffer (GPUCmdBufferHandle &cmdBufferHandle, VkImage swapChainImage)
{
    gos::gpu::pipe2::CmdBufferWriter2 cw;
    cw
        .begin (gpu, cmdBufferHandle)
        .imageTransition (swapChainImage, eImageLayout::undefined, eImageLayout::transfer_dst)
        .setViewport (gpu->viewport_getDefault());

    do_recordCommandBuffer (cw, swapChainImage);
    cw.imageTransition (swapChainImage, eImageLayout::transfer_dst, eImageLayout::presentation);    
    return cw.end();
}

bool VulkanExample7::do_recordCommandBuffer (gpu::pipe2::CmdBufferWriter2 &cw, VkImage swapChainImage)
{
    const asset::Asset_pipe *pipe;
    if (!theHub.getAsset(assetPipe1, &pipe))
        return false;
    
    const asset::Asset_pipe *pipe2;
    if (!theHub.getAsset(assetPipe2, &pipe2))
        return false;

    GPUDepthStencilHandle zbHandle = gpu->depthStencil_getDefault();
    vec2f screenWH;
    screenWH.set ((f32)gpu->swapChain_getWidth(), (f32)gpu->swapChain_getHeight());

    cw
    .imageTransition (rt1, eImageLayout::undefined, eImageLayout::color_attachment_optimal)
    .imageTransition (zbHandle, eImageLayout::undefined, eImageLayout::depth_attachment_optimal)
    .beginRender()
        .withRenderArea (rt1)
        .withRT (rt1, eAttachmentLoadOp::clear, eAttachmentStoreOp::dont_care, gos::ColorHDR(0, 0.1f, 0.1f))
        .withZB (zbHandle, eAttachmentLoadOp::clear, eAttachmentStoreOp::dont_care)
        .bindPipeline (pipe->handle_pipe)
        .bindVtxBuffer(vtxBufferHandle)
        .bindIdxBufferU16(idxBufferHandle)
        .pushConstant (0, &screenWH, sizeof(screenWH))
        .drawIndexed (6, 1, 0, 0, 0)

        .bindPipeline (pipe2->handle_pipe)
        .bindVtxBuffer(vtxBufferHandle, vtxoffset2)
        .bindIdxBufferU16(idxBufferHandle, idxoffset2)
        .drawIndexed (6, 1, 0, 0, 0)
        .endRender()
    .imageTransition (rt1, eImageLayout::color_attachment_optimal, eImageLayout::transfer_src)
    .copyImageToImage (rt1, swapChainImage, gpu->swapChain_getImageExten2D(), gpu->swapChain_getImageExten2D());        

    return true;
}

//**********************************
void VulkanExample7::doCPUStuff ()
{
    handleInput();
}



/************************************
 * renderizza inviando command buffer a GPU e poi aspettando che questa
 * abbia finito il suo lavoro
 */
void VulkanExample7::virtual_onRun()
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
        theHub.update (gos::getTimeSinceStart_msec());
        doCPUStuff ();
        mainLoop.stat_onCPUFrameEnd();


        mainLoop.run();

        // if (gpu->swapChain_wasRecreated())
        //     cam.changeAspectRatioPerspectiveFovLH (gpu->swapChain_calcAspectRatio());


        //se il job precedente e' stato presentato, posso schedularne uno nuovo
        gpu::SwapchainImg swapchainImg;
        if (mainLoop.gfxJob_canSubmit(&swapchainImg))
        {
            recordCommandBuffer (cmdBufferHandle, swapchainImg.image);
            mainLoop.gfxJob_submitAndPresent (cmdBufferHandle, swapchainImg);
        }
    }

    //aspetto che GPU abbia finito tutto cio' che ha in coda
    gpu->waitIdle();

    //free
    gpu->deleteResource (cmdBufferHandle);
}

