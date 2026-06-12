#include "VulkanExample1.h"


using namespace gos;


//************************************
void VulkanExample1::virtual_explain()
{
}

//************************************
void VulkanExample1::virtual_onCleanup() 
{
    gpu->deleteResource (vtxShaderHandle);
    gpu->deleteResource (fragShaderHandle);
    gpu->deleteResource (pipelineHandle);
}    


//************************************
bool VulkanExample1::virtual_onInit ()
{
    //carico gli shader
    if (!gpu->vtxshader_createFromFile ("shader/example1/shader.vert.spv", "main", &vtxShaderHandle))
    {
        gos::logger::err ("VulkanApp::init() => can't create vert shader\n");
        return false;
    }
    if (!gpu->pxlshader_createFromFile ("shader/example1/shader.frag.spv", "main", &fragShaderHandle))
    {
        gos::logger::err ("VulkanApp::init() => can't create frag shader\n");
        return false;
    }
    
    //pipeline
    gpu::Pipeline_def def;
    def
        .reset()
        .set_cullMode (eCullMode::CCW)
        .set_drawPrimitive (eDrawPrimitive::trisList)
        .shader_add (vtxShaderHandle)
        .shader_add (fragShaderHandle)
        .rt_add (eImageFormat::_SAME_AS_CURRENT_SWAPCHAIN);


    if (!gpu->pipeline_createNew (def, &pipelineHandle))
    {
        gos::logger::err ("VulkanApp::init() => can't create pipeline\n");
        return false;
    }
   

    return true;
}    


//************************************
bool VulkanExample1::recordCommandBuffer (GPUCmdBufferHandle &cmdBufferHandle, gpu::SwapchainImg &swapChainImage)
{
    gos::gpu::CmdBufferWriter2 cw;
    gpu::RenderCtx rctx;
    cw  .begin (gpu, cmdBufferHandle)
        .setViewport (gpu->viewport_getDefault())
        .imageTransition (swapChainImage.image, eImageLayout::undefined, eImageLayout::color_attachment_optimal)
        .renderCtx_define_begin(&rctx)
            .withRenderArea (gpu->swapChain_getWidth(), gpu->swapChain_getHeight())
            .withRT (swapChainImage.imageView, eAttachmentLoadOp::clear, eAttachmentStoreOp::store, gos::ColorHDR(0,0.01f,0))
        .define_end();

    rctx.bindPipeline (pipelineHandle)
        .draw(3, 1, 0, 0)
        .end_render_ctx();

    cw  .imageTransition (swapChainImage.image, eImageLayout::color_attachment_optimal, eImageLayout::presentation)
        .end();

        
    return true;
}

/************************************
 * renderizza inviando command buffer a GPU e poi aspettando che questa
 * abbia finito il suo lavoro
 */
void VulkanExample1::virtual_onRun()
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
        handleInput();
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

