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
    pipeline.deleteResources(gpu);
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
    if (!gpu->fragshader_createFromFile ("shader/example1/shader.frag.spv", "main", &fragShaderHandle))
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
        .add_rt (eImageFormat::_SAME_AS_CURRENT_SWAPCHAIN);


    if (!gpu->pipeline_v2_createNew (def, &pipeline))
    {
        gos::logger::err ("VulkanApp::init() => can't create pipeline\n");
        return false;
    }
   

    return true;
}    


//************************************
bool VulkanExample1::recordCommandBuffer (GPUCmdBufferHandle &cmdBufferHandle, gpu::AcquiredSwapchainImg &swapChainImage)
{
    gos::gpu::pipe2::CmdBufferWriter2 cw;
    cw
        .begin (gpu, cmdBufferHandle)
        .setViewport (gpu->viewport_getDefault())
        .imageTransition (swapChainImage.image, eImageLayout::undefined, eImageLayout::color_attachment_optimal)
        .beginRender()
            .withRenderArea (gpu->swapChain_getWidth(), gpu->swapChain_getHeight())
            .withRT (gpu->swapChain_getImageView(swapChainImage.index), eAttachmentLoadOp::clear, eAttachmentStoreOp::dont_care, gos::ColorHDR(0,0.01f,0))
            .bindPipeline (pipeline.pipeline_handle)
            .draw(3, 1, 0, 0)
            .endRender()
        .imageTransition (swapChainImage.image, eImageLayout::color_attachment_optimal, eImageLayout::presentation)
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
    gpu->cmdBuffer_create (eGPUQueueType::gfx, &cmdBufferHandle);


    //main loop
    while (bQuitApp == false)
    {
        mainLoop.stat_onCPUFrameBegin();
        handleInput();
        mainLoop.stat_onCPUFrameEnd();


        mainLoop.run();

        //se il job precedente e' stato presentato, posso schedularne uno nuovo
        gpu::AcquiredSwapchainImg swapchainImg;
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

