#include "VulkanExample7.h"
#include "../gosImage/gosImageBuilder.h"
#include "../gosImage/gosImageUtils.h"

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
    gpu->deleteResource(rtReadback);

    gpu->deleteResource (texHandle);
    gpu->deleteResource(descrSetInstanceHandle);
    gpu->deleteResource(descrPoolHandle);
    gpu->deleteResource(viewportHandle);
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

    //vtx buffer
    const gpu::Pipeline2 *pipeInfo = NULL;
    gpu->toVulkan(thePipe->handle_pipe, &pipeInfo);
    if (!gpu->vertexBuffer_create (32 * pipeInfo->vtx_stridePerStream[0], eMemAccessMode::shared_cpuW_autoSync, &vtxBufferHandle))
    {
        gos::logger::err ("VulkanApp::virtual_onInit() => gpu->vertexBuffer_create() failed\n");
        return false;
    }

    //INDEX BUFFER
    if (!gpu->indexBuffer_create (64 * sizeof(u16), eMemAccessMode::shared_cpuW_autoSync, &idxBufferHandle))
    {
        gos::logger::err ("VulkanApp::virtual_onInit() => gpu->indexBuffer_create() failed\n");
        return false;
    }

    //creo il sampler
    gpu->sampler_create (gpu::SamplerDesc(), &samplerHandle);

    
    //carico una texture
    {
        gos::Image im;
        /*gos::image::Builder builder;
        builder.begin (gos::getScrapAllocator(), &im)
            .beginTexture2D (eImageFormat::U8_RGBA, TEX_WIDTH, TEX_HEIGHT, 1)
            .setMipMapDataFromFile (0, "shader/example7/res/03-image/godus_01.png", image::Builder::eFilter::sRGB_to_RGB)
            //.setMipMapDataFromFile (0, "shader/example7/res/03-image/godus_01.png", image::Builder::eFilter::none)
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
        gpu->texture_create2D (&im, 0, eMemAccessMode::onGPU, &texHandle);
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

    gos::gpu::DescrSetInstanceWriter descrWriter;
    descrWriter.begin (gpu, descrSetInstanceHandle)
        .bindCombinedTextureAndSampler (0, texHandle, samplerHandle)
        .end();    


    
    //risorse di rendering
    const gpu::Texture *tex_info = gpu->getInfo (texHandle);
    tex_width = tex_info->dimx;
    tex_height = tex_info->dimy;
    rt_width = gos::utils::calcClosestPowerOf2(tex_info->dimx);
    rt_height = gos::utils::calcClosestPowerOf2(tex_info->dimy);
    
    gpu->viewport_create (0, 0, rt_width, rt_height, &viewportHandle);

    if (!gpu->renderTarget_create (rt_width, rt_height, eImageFormat::U8_RGBA, &rt1))
        return false;
    if (!gpu->renderTarget_create (rt_width, rt_height, eImageFormat::U8_RGBA, eMemAccessMode::readback, &rtReadback))
        return false;        
    return true;
}    

//************************************
bool VulkanExample7::sampleImage (GPUCmdBufferHandle &cmdBufferHandle, u32 dstW, u32 dstH)
{
    const asset::Asset_pipe *pipe;
    if (!theHub.getAsset(assetPipe1, &pipe))
        return false;

    struct Vertex1
    {
        u32     x;
        u32     y;
        vec2f   tutv;
    };

    //vtxbuffer
    static constexpr u32 X = 0;
    static constexpr u32 Y = 0;
    Vertex1 vtxSRC[4];
    vtxSRC[0].x = X;            vtxSRC[0].y = Y;            vtxSRC[0].tutv.set (0,0);
    vtxSRC[1].x = X+dstW-1;     vtxSRC[1].y = Y;            vtxSRC[1].tutv.set (1,0);
    vtxSRC[2].x = X+dstW-1;     vtxSRC[2].y = Y+dstH-1;     vtxSRC[2].tutv.set (1,1);
    vtxSRC[3].x = X;            vtxSRC[3].y = Y+dstH-1;     vtxSRC[3].tutv.set (0,1);
    gpu->writeAndSync (vtxBufferHandle, 0, vtxSRC, sizeof(Vertex1) * 4);

    //idx buffer
    u16 indexSRC[6] = { 0, 1, 2,    2, 3, 0 };
    gpu->writeAndSync (idxBufferHandle, 0, indexSRC, sizeof(u16) * 6);

    vec2f screenWH;
    screenWH.set ((f32)rt_width, (f32)rt_height);



    gos::gpu::pipe2::CmdBufferWriter2 cw;
    cw
        .begin (gpu, cmdBufferHandle)
        .setViewport (viewportHandle)
        //.setViewport (gpu->viewport_getDefault())
    
        .imageTransition (rt1, eImageLayout::undefined, eImageLayout::color_attachment_optimal)
        .imageTransition (rtReadback, eImageLayout::undefined, eImageLayout::transfer_dst)
        .beginRender()
            .withRenderArea (rt1)
            .withRT (rt1, eAttachmentLoadOp::clear, eAttachmentStoreOp::dont_care, gos::ColorHDR(0, 1.0f, 0))
            .bindPipeline (pipe->handle_pipe)
            .bindDescriptorSet(descrSetInstanceHandle, 0)
            .bindVtxBuffer(vtxBufferHandle)
            .bindIdxBufferU16(idxBufferHandle)
            .pushConstant (0, &screenWH, sizeof(screenWH))
        .drawIndexed (6, 1, 0, 0, 0)
        .endRender()
    .imageTransition (rt1, eImageLayout::color_attachment_optimal, eImageLayout::transfer_src)
    .copyImageToImage (rt1, rtReadback, { rt_width, rt_height}, { rt_width, rt_height} )
    .imageTransition (rtReadback, eImageLayout::transfer_dst, eImageLayout::general);

    return cw.end();
}

//************************************
bool VulkanExample7::recordCommandBuffer (GPUCmdBufferHandle &cmdBufferHandle, gpu::SwapchainImg &swapchainImg)
{
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
    
    vec2f screenWH;
    screenWH.set ((f32)gpu->swapChain_getWidth(), (f32)gpu->swapChain_getHeight());

    cw
    .imageTransition (rt1, eImageLayout::undefined, eImageLayout::color_attachment_optimal)
    .imageTransition (rtReadback, eImageLayout::undefined, eImageLayout::transfer_dst)
    .beginRender()
        .withRenderArea (rt1)
        .withRT (rt1, eAttachmentLoadOp::clear, eAttachmentStoreOp::dont_care, gos::ColorHDR(0, 0.1f, 0.1f))
        .bindPipeline (pipe->handle_pipe)
        .bindDescriptorSet(descrSetInstanceHandle, 0)
        .bindVtxBuffer(vtxBufferHandle)
        .bindIdxBufferU16(idxBufferHandle)
        .pushConstant (0, &screenWH, sizeof(screenWH))
        //.drawIndexed (6, 1, 0, 0, 0)
        .endRender()
    .imageTransition (rt1, eImageLayout::color_attachment_optimal, eImageLayout::transfer_src)
    .copyImageToImage (rt1, rtReadback, gpu->swapChain_getImageExten2D(), gpu->swapChain_getImageExten2D())
    .copyImageToImage (rt1, swapchainImg.image, gpu->swapChain_getImageExten2D(), gpu->swapChain_getImageExten2D())
    .imageTransition (rtReadback, eImageLayout::transfer_dst, eImageLayout::general);

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
    GPUCmdBufferHandle  cmdBufferHandle;
    gpu->cmdBuffer_create (eGPUQueueType::gfx, &cmdBufferHandle);

    u32 mipmap_w = tex_width / 2;
    u32 mipmap_h = tex_height / 2;

    gpu::GFXJob job;
    job.setup (gpu);
    {

        sampleImage (cmdBufferHandle, mipmap_w, mipmap_h);
        job.submit (cmdBufferHandle);
        while (!job.hasFinished())
            gpu->waitIdle();

        gpu::sMappedImage m;
        if (gpu->map (rtReadback, &m))
        {
            gpu->image_manualSync_cpuRead(&m ,1);
            if (!image::utils::saveBufferRGBAasTGA (m.host_image_pt, rt_width, rt_height, "shader/example7/pippo.tga"))
            {
                DBGBREAK;
            }
            gpu->image_unmap (m);
        }        
    }
    job.unsetup();


    /*main loop
    gpu::MainLoop2 mainLoop;
    mainLoop.setup (gpu);
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
            if (10 == frameNum)
            {
                gpu::sMappedImage m;
                if (gpu->map (rtReadback, &m))
                {
                    gpu->image_manualSync_cpuRead(&m ,1);
                    if (!image::utils::saveBufferRGBAasTGA (m.host_image_pt, 1024, 1024, "shader/example7/pippo.tga"))
                    {
                        DBGBREAK;
                    }
                    gpu->image_unmap (m);
                }
            }

            recordCommandBuffer (cmdBufferHandle, swapchainImg);
            mainLoop.gfxJob_submitAndPresent (cmdBufferHandle, swapchainImg);
            frameNum++;
        }
    }*/

    //aspetto che GPU abbia finito tutto cio' che ha in coda
    gpu->waitIdle();

    //free
    gpu->deleteResource (cmdBufferHandle);
}

