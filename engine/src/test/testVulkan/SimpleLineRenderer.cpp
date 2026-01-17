#include "SimpleLineRenderer.h"

using namespace gos;

//*****************************
SimpleLineRenderer::SimpleLineRenderer ()
{
    gpu = NULL;
    localAllocator = gos::getSysHeapAllocator();
    vtxList.setup (localAllocator, 1024);
    idxList.setup (localAllocator, 1024*16);
    bNeedUpdate = false;
}

//*****************************
SimpleLineRenderer::~SimpleLineRenderer ()
{
    vtxList.unsetup();
    idxList.unsetup();

    gpu->deleteResource (hVtxBuffer);
    gpu->deleteResource (hIdxBuffer);
    gpu->deleteResource (hVtxShader);
    gpu->deleteResource (hFragShader);
    gpu->deleteResource (hDescrSetInstance);
    gpu->deleteResource (pipelineHandle);
    gpu->deleteResource (hUBO);
}

//*****************************
bool SimpleLineRenderer::setup (gos::GPU *gpuIN, GPUDescrPoolHandle &descrPoolHandle)
{
    gpu = gpuIN;

    //carico gli shader
    if (!gpu->vtxshader_createFromFile ("@shader/lineShader.vert.spv", "main", &hVtxShader))
    {
        gos::logger::err ("SimpleLineRenderer::setup() => can't create vert shader\n");
        return false;
    }
    if (!gpu->pxlshader_createFromFile ("@shader/lineShader.frag.spv", "main", &hFragShader))
    {
        gos::logger::err ("SimpleLineRenderer::setup() => can't create frag shader\n");
        return false;
    }

    //pipeline
    gpu::Pipeline_def def;
    def
        .reset()
        .set_cullMode (eCullMode::NONE)
        .set_drawPrimitive (eDrawPrimitive::lineList)
        .shader_add (hVtxShader)
        .shader_add (hFragShader)
        .rt_add (eImageFormat::_SAME_AS_CURRENT_SWAPCHAIN)
        .zbuffer_define (eImageFormat::_DEPTH_BEST, false, eZFunc::ALWAYS)
        .vtxStream_add (eVtxStreamInputRate::perVertex)
            .add (00, offsetof(sVertex, pos), eDataFormat::_3f32)
            .add (1, offsetof(sVertex, col), eDataFormat::_3f32)
            .endVtxStream()
        .descriptorset_add()
            .add (0, eGPUDescriptrorType::UNIFORM_BUFFER, 1, eGPUDescriptrorUsage::vtx_shader)
            .endDescriptorSet();

    if (!gpu->pipeline_createNew (def, &pipelineHandle))
    {
        gos::logger::err ("VulkanApp::init() => can't create pipeline\n");
        return false;
    };    

    //creo un buffer per UBO
    if (!gpu->uniformBuffer_create (sizeof(sUniformBufferObject), eMemAccessMode::shared_cpuW_autoSync, &hUBO))
    {
        gos::logger::err ("SimpleLineRenderer::setup() => GPU::uniformBuffer_create\n");
        return false;
    }
    
    //alloco una istanza del descriptorSet
    if (!gpu->descrSetInstance_create (descrPoolHandle, pipelineHandle, 0, &hDescrSetInstance))
    {
        gos::logger::err ("SimpleLineRenderer::setup() => can't create descriptorSet instance\n");
        return false;
    }
    return true;
}


//*****************************
void SimpleLineRenderer::begin()
{
    bNeedUpdate = true;
    vtxList.reset();
    idxList.reset();
    curColor.set (1,1,1);
}

//*****************************
void SimpleLineRenderer::setColor (const gos::vec3f &color)
{
    curColor = color;
}

//*****************************
u16 SimpleLineRenderer::addVtx (const gos::vec3f &p)
{
    sVertex v;
    v.pos = p;
    v.col = curColor;

    const u32 n = vtxList.getNElem();
    vtxList[n] = v;
    return n;
}

//*****************************
void SimpleLineRenderer::line (u16 v0, u16 v1)
{
    assert (v0 < vtxList.getNElem());
    assert (v1 < vtxList.getNElem());
    idxList.append (v0);
    idxList.append (v1);
}

//*****************************
void SimpleLineRenderer::addLine (const gos::vec3f &p1, const gos::vec3f &p2)
{
    const u16 i1 = addVtx(p1);
    const u16 i2 = addVtx(p2);
    line (i1, i2);
}

//*****************************
void SimpleLineRenderer::end()
{
    printf ("SimpleLineRenderer() => num vertex=%d, numLine=%d\n", vtxList.getNElem(), idxList.getNElem() / 2);
}

//*****************************
bool SimpleLineRenderer::recordCommandBuffer (gpu::CmdBufferWriter2 &cw, VkImageView rt, GPUStgBufferHandle hStgBuffer, gos::geom::Camera3 &cam)
{
    if (vtxList.getNElem() == 0)
        return false;
    if (idxList.getNElem() == 0)
        return false;

    if (bNeedUpdate)
    {
        bNeedUpdate = false;

        gpu->deleteResource (hVtxBuffer);
        if (!gpu->vertexBuffer_create (sizeof(sVertex) * vtxList.getNElem(), eMemAccessMode::onGPU, &hVtxBuffer))
        {
            gos::logger::err ("SimpleLineRenderer::recordCommandBuffer() => gpu->vertexBuffer_create() failed\n");
            return false;
        }

        if (!gpu->stagingBuffer_uploadToGPUBuffer (hStgBuffer, vtxList._queryPointer(), hVtxBuffer, 0, sizeof(sVertex) * vtxList.getNElem()))
        {
            gos::logger::err ("SimpleLineRenderer::recordCommandBuffer() => gpu->stagingBuffer_uploadToGPUBuffer() failed\n");
            return false;
        }        

        gpu->deleteResource (hIdxBuffer);
        if (!gpu->indexBuffer_create (sizeof(u16) * idxList.getNElem(), eMemAccessMode::onGPU, &hIdxBuffer))
        {
            gos::logger::err ("SimpleLineRenderer::recordCommandBuffer() => gpu->indexBuffer_create() failed\n");
            return false;
        }

        if (!gpu->stagingBuffer_uploadToGPUBuffer (hStgBuffer, idxList._queryPointer(), hIdxBuffer, 0, sizeof(u16) * idxList.getNElem()))
        {
            gos::logger::err ("SimpleLineRenderer::recordCommandBuffer() => gpu->stagingBuffer_uploadToGPUBuffer() failed\n");
            return false;
        }        

    }

    //upload di UBO su GPU
    ubo.camProj = cam.getMatP();
    ubo.camView = cam.getMatV();
    gpu->writeAndSync (hUBO, 0, &ubo, sizeof(sUniformBufferObject));            
    gos::gpu::DescrSetInstanceWriter descrWriter;
    descrWriter.begin (gpu, hDescrSetInstance)
        .bindUniformBuffer (0, hUBO)
        .end();



    gpu::RenderCtx rctx;
    cw  .renderCtx_define_begin(&rctx)
            .withRenderArea (gpu->swapChain_getWidth(), gpu->swapChain_getHeight())
            .withRT (rt, eAttachmentLoadOp::load, eAttachmentStoreOp::dont_care)
        .define_end();

    rctx.bindPipeline (pipelineHandle)
        .bindDescriptorSet (hDescrSetInstance, 0)
        .bindVtxBuffer (hVtxBuffer)
        .bindIdxBufferU16 (hIdxBuffer)
        .drawIndexed (idxList.getNElem(), 1, 0, 0, 0)
        .end_render_ctx();

    return true;
}    


