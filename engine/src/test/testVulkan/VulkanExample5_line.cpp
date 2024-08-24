#include "VulkanExample5.h"

using namespace gos;

//*****************************
VulkanExample5::Line::Line ()
{
    gpu = NULL;
    localAllocator = gos::getSysHeapAllocator();
    list.setup (localAllocator, 1024);
    bNeedUpdate = false;
}

//*****************************
VulkanExample5::Line::~Line ()
{
    list.unsetup();

    gpu->deleteResource (hVtxBuffer);
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
bool VulkanExample5::Line::setup (gos::GPU *gpuIN, GPUDescrPoolHandle &descrPoolHandle)
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
        gos::logger::err ("VulkanExample5::Line::setup() => can't create vtxDeclHandle\n");
        return false;
    }


    //carico gli shader
    if (!gpu->vtxshader_createFromFile ("@shader/lineShader.vert.spv", "main", &hVtxShader))
    {
        gos::logger::err ("VulkanExample5::Line::setup() => can't create vert shader\n");
        return false;
    }
    if (!gpu->fragshader_createFromFile ("@shader/lineShader.frag.spv", "main", &hFragShader))
    {
        gos::logger::err ("VulkanExample5::Line::setup() => can't create frag shader\n");
        return false;
    }

    //Creo il descriptorSet layout  con un solo UNIFORM BUFFER per il VTX SHADER
    gpu->descrSetLayout_createNew (&hDescrSetLayout)
        .add (VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
        .end();
    if (hDescrSetLayout.isInvalid())
    {
        gos::logger::err ("VulkanExample5::Line::setup() => can't create descriptor set\n");
        return false;
    }


    //creo il render pass
    gpu->renderLayout_createNew (&hRenderLayout)
        .requireRendertarget (eRenderTargetUsage::presentation, gpu->swapChain_getImageFormat(), false)
        .addSubpass_GFX()
            .useRenderTarget(0)
        .end()
    .end();
    if (hRenderLayout.isInvalid())
    {
        gos::logger::err ("VulkanExample5::Line::setup() => can't create renderTaskLayout\n");
        return false;
    }

    //frame buffers
    gpu->frameBuffer_createNew (hRenderLayout, &hFrameBuffer)
        .bindRenderTarget (gpu->renderTarget_getDefault())
        .end();
    if (hFrameBuffer.isInvalid())
    {
        gos::logger::err ("VulkanExample5::Line::setup() => can't create frameBufferHandle\n");
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
        gos::logger::err ("VulkanExample5::Line::setup() => can't create pipeline\n");
        return false;
    }

    //non mi serve piu'
    gpu->deleteResource (vtxDeclHandle);


    //creo un buffer per UBO
    if (!gpu->uniformBuffer_create (sizeof(sUniformBufferObject), &hUBO))
    {
        gos::logger::err ("VulkanExample5::Line::setup() => GPU::uniformBuffer_create\n");
        return false;
    }
    
    //alloco una istanza del descriptorSet
    if (!gpu->descrSetInstance_createNew (descrPoolHandle, hDescrSetLayout, &hDescrSetInstance))
    {
        gos::logger::err ("VulkanExample5::Line::setup() => can't create descriptorSet instance\n");
        return false;
    }
    return true;
}


//*****************************
void VulkanExample5::Line::begin()
{
    bNeedUpdate = true;
    list.reset();
    curColor.set (1,1,1);
}

//*****************************
void VulkanExample5::Line::setColor (const gos::vec3f &color)
{
    curColor = color;
}

//*****************************
void VulkanExample5::Line::addLine (const gos::vec3f &p1, const gos::vec3f &p2)
{
    sVertex v;
    v.pos = p1;
    v.col = curColor;
    list.append(v);

    v.pos = p2;
    v.col = curColor;
    list.append(v);
}

//*****************************
void VulkanExample5::Line::end()
{
}

//*****************************
bool VulkanExample5::Line::recordCommandBuffer (gpu::CmdBufferWriter &cw, GPUStgBufferHandle hStgBuffer, gos::geom::Camera3 &cam)
{
    if (list.getNElem() == 0)
        return false;

    if (bNeedUpdate)
    {
        bNeedUpdate = false;

        gpu->deleteResource (hVtxBuffer);
        if (!gpu->vertexBuffer_create (sizeof(Vertex) * list.getNElem(), eVIBufferMode::onGPU, &hVtxBuffer))
        {
            gos::logger::err ("VulkanExample5::Line::recordCommandBuffer() => gpu->vertexBuffer_create() failed\n");
            return false;
        }

        if (!gpu->stagingBuffer_uploadToGPUBuffer (hStgBuffer, list._queryPointer(), hVtxBuffer, 0, sizeof(Vertex) * list.getNElem()))
        {
            gos::logger::err ("VulkanExample5::Line::recordCommandBuffer() => gpu->stagingBuffer_uploadToGPUBuffer() failed\n");
            return false;
        }        
    }

    //upload di UBO su GPU
    ubo.camProj = cam.getMatP();
    ubo.camView = cam.getMatV();
    gpu->uniformBuffer_mapCopyUnmap (hUBO, 0, sizeof(sUniformBufferObject), &ubo);            
    gos::gpu::DescrSetInstanceWriter descrWriter;
    descrWriter.begin (gpu, hDescrSetInstance)
        .updateUniformBuffer (0, hUBO)
        .end();


    cw.setViewport (gpu->viewport_getDefault())
        .bindPipeline (hPipeline)
        .bindDescriptorSet (hDescrSetInstance)
        .setClearColor (0, gos::ColorHDR(0.1f, 0.1f, 0.3f))
        .renderPass_begin (hRenderLayout, hFrameBuffer)
            .bindVtxBuffer (hVtxBuffer)
            .draw (list.getNElem(), 1, 0, 0)
        .renderPass_end();

    return true;
}    


