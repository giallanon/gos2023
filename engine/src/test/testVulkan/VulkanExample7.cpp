#include "VulkanExample7.h"
#include "../gosImage/gosImageBuilder.h"


using namespace gos;


//************************************
void VulkanExample7::virtual_explain()
{
}

//************************************
void VulkanExample7::virtual_onCleanup() 
{
    theHub.unload (assetPipe1);

    gpu->deleteResource(vtxBufferHandle);
    gpu->deleteResource(idxBufferHandle);
    gpu->deleteResource(rt1);

    gpu->deleteResource (texHandle);
    gpu->deleteResource(descrSetInstanceHandle);
    gpu->deleteResource(descrPoolHandle);
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


    //quad 1
    static constexpr u32 TEX_WIDTH = 830;
    static constexpr u32 TEX_HEIGHT = 413;
    {
        struct Vertex1
        {
            u32     x;
            u32     y;
            vec2f   tutv;
        };

        static constexpr u32 X = 0;
        static constexpr u32 Y = 0;
        Vertex1 vtxSRC[8];

        vtxSRC[0].x = X;            vtxSRC[0].y = Y;            vtxSRC[0].tutv.set (0,0);
        vtxSRC[1].x = X+TEX_WIDTH-1;    vtxSRC[1].y = Y;            vtxSRC[1].tutv.set (1,0);
        vtxSRC[2].x = X+TEX_WIDTH-1;    vtxSRC[2].y = Y+TEX_HEIGHT-1;   vtxSRC[2].tutv.set (1,1);
        vtxSRC[3].x = X;            vtxSRC[3].y = Y+TEX_HEIGHT-1;   vtxSRC[3].tutv.set (0,1);
        gpu->writeAndSync (vtxBufferHandle, 0, vtxSRC, sizeof(Vertex1) * 4);

        u16 indexSRC[6] = { 0, 1, 2,    2, 3, 0 };
        gpu->writeAndSync (idxBufferHandle, 0, indexSRC, sizeof(u16) * 6);
    }

    //creo il sampler
    gpu->sampler_create (gpu::SamplerDesc(), &samplerHandle);

    
    //carico una texture
    {
        gos::Image im;
/*        gos::image::Builder builder;
        builder.begin (gos::getScrapAllocator(), &im)
            .beginTexture2D (eImageFormat::U8_RGBA, TEX_WIDTH, TEX_HEIGHT, 1)
            .setMipMapDataFromFile (0, "shader/example7/res/03-image/godus_01.png", image::Builder::eFilter::sRGB_to_RGB)
            .endTexture2D()
        .end();
        if (builder.anyError())
        {
            gos::logger::err ("VulkanApp::init() => can't build image'\n");
            return false;
        }
        image::save (im, "shader/example7/godus_01.gosimage");
        image::free (gos::getScrapAllocator(), im);
        */
        image::load (gos::getScrapAllocator(), "shader/example7/godus_01.gosimage", &im);
        gpu->texture_create2D (&im, 0, &texHandle);
        image::free (gos::getScrapAllocator(), im);
    }


    //creo un descriptor pool
    gpu->descrPool_createNew (&descrPoolHandle)
        .setMaxNumDescriptorSet(4)
        .addPool_uniformBuffer()
        .addPool_combinedTextureAndSampler(1)
        .end();
    if (descrPoolHandle.isInvalid())
    {
        gos::logger::err ("VulkanApp::init() => can't create descriptor pool\n");
        return false;
    }

    //alloco una istanza del descriptorSet
    if (!gpu->pipeline_createDescrSetInstance (thePipe->handle_pipe, 0, descrPoolHandle, &descrSetInstanceHandle))
    {
        gos::logger::err ("VulkanApp::init() => can't create descriptorSet instance\n");
        return false;
    }

    return true;
}    


//************************************
bool VulkanExample7::recordCommandBuffer (GPUCmdBufferHandle &cmdBufferHandle, gpu::SwapchainImg &swapchainImg)
{
    //aggiorno UBO
    static u8 bind_once = 0;
    if (0 == bind_once)
    {
        bind_once = 1;

        gos::gpu::DescrSetInstanceWriter descrWriter;
        descrWriter.begin (gpu, descrSetInstanceHandle)
            .bindCombinedTextureAndSampler (0, texHandle, samplerHandle)
            .end();
    }


    gos::gpu::pipe2::CmdBufferWriter2 cw;
    cw
        .begin (gpu, cmdBufferHandle)
        .imageTransition (swapchainImg.image, eImageLayout::undefined, eImageLayout::transfer_dst)
        .setViewport (gpu->viewport_getDefault());

    do_recordCommandBuffer (cw, swapchainImg);
    cw.imageTransition (swapchainImg.image, eImageLayout::transfer_dst, eImageLayout::presentation);    
    return cw.end();
}

//************************************
bool VulkanExample7::do_recordCommandBuffer (gpu::pipe2::CmdBufferWriter2 &cw, gpu::SwapchainImg &swapchainImg)
{
    const asset::Asset_pipe *pipe;
    if (!theHub.getAsset(assetPipe1, &pipe))
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
        .bindDescriptorSet(descrSetInstanceHandle, 0)
        .bindVtxBuffer(vtxBufferHandle)
        .bindIdxBufferU16(idxBufferHandle)
        .pushConstant (0, &screenWH, sizeof(screenWH))
        .drawIndexed (6, 1, 0, 0, 0)
        .endRender()
    .imageTransition (rt1, eImageLayout::color_attachment_optimal, eImageLayout::transfer_src)
    .copyImageToImage (rt1, swapchainImg.image, gpu->swapChain_getImageExten2D(), gpu->swapChain_getImageExten2D());        

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
            recordCommandBuffer (cmdBufferHandle, swapchainImg);
            mainLoop.gfxJob_submitAndPresent (cmdBufferHandle, swapchainImg);
        }
    }

    //aspetto che GPU abbia finito tutto cio' che ha in coda
    gpu->waitIdle();

    //free
    gpu->deleteResource (cmdBufferHandle);
}

