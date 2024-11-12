#include "ThePipeline.h"

using namespace gos;


//********************************
ThePipeline::ThePipeline()
{
    gpu = NULL;
    localAllocator = GOSNEW(gos::getSysHeapAllocator(), LocalAllocator)("ThePipeline");
    localAllocator->setup (1024 * 1024);

    hRenderLayout.setInvalid();
    hFrameBuffer.setInvalid();
}

//********************************
ThePipeline::~ThePipeline()
{
    unsetup();

    if (NULL != localAllocator)
    {
        GOSDELETE(gos::getSysHeapAllocator(), localAllocator);
        localAllocator = NULL;
    }

}

//********************************
void ThePipeline::unsetup()
{
    if (NULL == gpu)
        return;

    gpu->deleteResource (hRenderLayout);
    gpu->deleteResource (hFrameBuffer);
    //gpu->deleteResource (hSampler_diffuse);
    gpu->deleteResource (hDescrPool);

    textureList.unsetup ();
    gpu = NULL;
}

//********************************
bool ThePipeline::setup (gos::GPU *gpuIN)
{
    gpu = gpuIN;

    //creo il render layout
    gpu->renderLayout_createNew (&hRenderLayout)
        .requireRendertarget (gpu->swapChain_getImageFormat(), eRenderTargetUsage::dont_care, eRenderTargetUsage::presentation, true)
        .requireZBuffer (gpu->depthStencil_getDefaultFormat(), eZBufferUsage::dont_care, eZBufferUsage::dont_care, true)
        .addSubpass_GFX()
            .useRenderTarget(0)
            .useDepthStencil()
        .end()
    .end();
    if (hRenderLayout.isInvalid())
    {
        gos::logger::err ("ThePipeline::setup() => can't create renderTaskLayout\n");
        return false;
    }

    //frame buffers
    gpu->frameBuffer_createNew (hRenderLayout, &hFrameBuffer)
        .bindRenderTarget (gpu->renderTarget_getDefault())
        .bindDepthStencil (gpu->depthStencil_getDefault())
        .end();
    if (hFrameBuffer.isInvalid())
    {
        gos::logger::err ("ThePipeline::setup() => can't create frameBufferHandle\n");
        return false;
    }


    //sampler
    gpu->sampler_create (gpu::SamplerDesc(), &hSampler_diffuse);


    //creo un descriptor pool
    if (!gpu->descrPool_createNew (&hDescrPool)
        .setMaxNumDescriptorSet(3)
        .addPool_uniformBuffer()
        .addPool_sampler(8)
        .addPool_texture(NUM_MAX_TEXTURE)
        .addPool_storageBuffer(8)
        .end())
    {
        gos::logger::err ("VulkanApp::init() => can't create descriptor pool\n");
        return false;
    }


    textureList.setup (localAllocator, gpu, NUM_MAX_TEXTURE);


    return true;
}
