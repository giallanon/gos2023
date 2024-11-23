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
    gpu->deleteResource (hDescrSetLayout);
    gpu->deleteResource (hPipeline);
    gpu->deleteResource (hUBO);
    gpu->deleteResource (hRenderLayout);
    gpu->deleteResource (hFrameBuffer);
}

//*****************************
bool SimpleLineRenderer::setup (gos::GPU *gpuIN, GPUDescrPoolHandle &descrPoolHandle)
{
    gpu = gpuIN;

    //vtx declaration
    GPUVtxDeclHandle vtxDeclHandle;
    gpu->vtxDecl_createNew (&vtxDeclHandle)
        .addStream(eVtxStreamInputRate::perVertex)
            .addLayout (0, offsetof(sVertex, pos), eDataFormat::_3f32)
            .addLayout (1, offsetof(sVertex, col), eDataFormat::_3f32)
        .end();
    if (vtxDeclHandle.isInvalid())
    {
        gos::logger::err ("SimpleLineRenderer::setup() => can't create vtxDeclHandle\n");
        return false;
    }


    //carico gli shader
    if (!gpu->vtxshader_createFromFile ("@shader/lineShader.vert.spv", "main", &hVtxShader))
    {
        gos::logger::err ("SimpleLineRenderer::setup() => can't create vert shader\n");
        return false;
    }
    if (!gpu->fragshader_createFromFile ("@shader/lineShader.frag.spv", "main", &hFragShader))
    {
        gos::logger::err ("SimpleLineRenderer::setup() => can't create frag shader\n");
        return false;
    }

    //Creo il descriptorSet layout  con un solo UNIFORM BUFFER per il VTX SHADER
    gpu->descrSetLayout_createStatic (&hDescrSetLayout)
        .add_uniformBuffer (VK_SHADER_STAGE_VERTEX_BIT)
        .end();
    if (hDescrSetLayout.isInvalid())
    {
        gos::logger::err ("SimpleLineRenderer::setup() => can't create descriptor set\n");
        return false;
    }


    //creo il render pass
    gpu->renderLayout_createNew (&hRenderLayout)
        //.requireRendertarget (gpu->swapChain_getImageFormat(), eRenderTargetUsage::storage_color_attachment_optimal, eRenderTargetUsage::presentation, false)
        .requireRendertarget (gpu->swapChain_getImageFormat(), eImageLayout::undefined, eImageLayout::presentation, eAttachmentLoadOp::load, eAttachmentStoreOp::store)
        .addSubpass_GFX()
            .writeToRenderTarget(0)
        .end()
    .end();
    if (hRenderLayout.isInvalid())
    {
        gos::logger::err ("SimpleLineRenderer::setup() => can't create renderTaskLayout\n");
        return false;
    }

    //frame buffers
    gpu->frameBuffer_createNew (hRenderLayout, &hFrameBuffer)
        .bindRenderTarget (gpu->renderTarget_getDefault())
        .end();
    if (hFrameBuffer.isInvalid())
    {
        gos::logger::err ("SimpleLineRenderer::setup() => can't create frameBufferHandle\n");
    }        

    //creo la pipeline
    gpu->pipeline_createNew (hRenderLayout, &hPipeline)
        .addShader (hVtxShader)
        .addShader (hFragShader)
        .setVtxDecl (vtxDeclHandle)
        .setCullMode (eCullMode::NONE)
        .setDrawPrimitive (eDrawPrimitive::lineList)
        .descriptor_add (hDescrSetLayout)
        //.setWireframe(true)
        .end ();

    if (hPipeline.isInvalid())
    {
        gos::logger::err ("SimpleLineRenderer::setup() => can't create pipeline\n");
        return false;
    }

    //non mi serve piu'
    gpu->deleteResource (vtxDeclHandle);


    //creo un buffer per UBO
    if (!gpu->uniformBuffer_create (sizeof(sUniformBufferObject), eVIBufferMode::shared_cpuW_autoSync, &hUBO))
    {
        gos::logger::err ("SimpleLineRenderer::setup() => GPU::uniformBuffer_create\n");
        return false;
    }
    
    //alloco una istanza del descriptorSet
    if (!gpu->descrSetInstance_createNew (descrPoolHandle, hDescrSetLayout, &hDescrSetInstance))
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
bool SimpleLineRenderer::recordCommandBuffer (gpu::CmdBufferWriter &cw, GPUStgBufferHandle hStgBuffer, gos::geom::Camera3 &cam)
{
    if (vtxList.getNElem() == 0)
        return false;
    if (idxList.getNElem() == 0)
        return false;

    if (bNeedUpdate)
    {
        bNeedUpdate = false;

        gpu->deleteResource (hVtxBuffer);
        if (!gpu->vertexBuffer_create (sizeof(sVertex) * vtxList.getNElem(), eVIBufferMode::onGPU, &hVtxBuffer))
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
        if (!gpu->indexBuffer_create (sizeof(u16) * idxList.getNElem(), eVIBufferMode::onGPU, &hIdxBuffer))
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


    cw
        //.setViewport (gpu->viewport_getDefault())
        .bindPipeline (hPipeline)
        .bindDescriptorSet (hDescrSetInstance, 0)
        //.setClearColor (0, gos::ColorHDR(0.1f, 1.0f, 0.0f))
        .renderPass_begin (hRenderLayout, hFrameBuffer)
            .bindVtxBuffer (hVtxBuffer)
            .bindIdxBufferU16 (hIdxBuffer)
            .drawIndexed (idxList.getNElem(), 1, 0, 0, 0)
        .renderPass_end();

    return true;
}    


