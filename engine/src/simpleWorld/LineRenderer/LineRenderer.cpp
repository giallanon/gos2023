#include "LineRenderer.h"


using namespace gos;



//********************************
LineRenderer::LineRenderer()
{
    gpu = NULL;
}

//********************************
LineRenderer::~LineRenderer()
{
    if (NULL != gpu)
    {
        gpu->deleteResource (hPipeline);
        gpu->deleteResource (hVtxShader);
        gpu->deleteResource (hFragShader);
        gpu->deleteResource (hVtxBuffer);
        gpu->deleteResource (hIdxBuffer);
        gpu->deleteResource (vtxDeclHandle);

        gpu->deleteResource (descr2_ssboHandle);
        gpu->deleteResource (descr2_instance);
        gpu->deleteResource (descr2_layout);

        gpu = NULL;
    }
}

//********************************
bool LineRenderer::setup (ThePipeline *thePipelineIN)
{
    thePipeline = thePipelineIN;
    localAllocator = thePipeline->localAllocator;
    gpu = thePipeline->gpu;
    if (!priv_setupVulkan())
        return false;
    return true;
}

//********************************
bool LineRenderer::priv_setupVulkan()
{
    //vtx declaration
    gpu->vtxDecl_createNew (&vtxDeclHandle)
        .addStream(eVtxStreamInputRate::perVertex)
            .addLayout (0, 0, eDataFormat::_3f32)
        .end();
    if (vtxDeclHandle.isInvalid())
    {
        gos::logger::err ("LineRenderer::priv_setupVulkan() => can't create vtxDeclHandle\n");
        return false;
    }
    

    //Creo il descriptorSet layout set = 2, binding = 0
    //if (!gpu->descrSetLayout_create (&descr2_layout)
    if (!gpu->descrSetLayout_create_updAfterBind (&descr2_layout)
        .add_storageBuffer (VK_SHADER_STAGE_VERTEX_BIT)
        .end())
    {
        gos::logger::err ("LineRenderer::priv_setupVulkan() => can't create descriptor set 2\n");
        return false;
    }       
    
    //pipeline
    if (!priv_createPipeline())
    {
        gos::logger::err ("LineRenderer::setup() => can't create pipeline\n");
        return false;
    }

    //vtx buffer
    if (!gpu->vertexBuffer_create (128, eVIBufferMode::shared_cpuW_manualSync, &hVtxBuffer))
    {
        gos::logger::err ("LineRenderer::setup() => can't create vtxbuffer\n");
        return false;
    }
    else
    {
        sVertex vtx[4];
        vtx[0].pos.set (0, -0.5f, 0);
        vtx[1].pos.set (1, -0.5f, 0);
        vtx[2].pos.set (1,  0.5f, 0);
        vtx[3].pos.set (0,  0.5f, 0);

        gpu::sMappedBuffer mapped;
        gpu->map (hVtxBuffer, 0, sizeof(sVertex) * 4, &mapped);
        memcpy (mapped.host_pt, vtx, sizeof(sVertex) * 4);
        gpu->buffer_manualSync (&mapped, 1);
        gpu->buffer_unmap(mapped);        
    }

    //idx buffer
    if (!gpu->indexBuffer_create (128, eVIBufferMode::shared_cpuW_manualSync, &hIdxBuffer))
    {
        gos::logger::err ("LineRenderer::setup() => can't create idxBuffer\n");
        return false;
    }
    else
    {
        const u16 idx[6] = { 0,1,2, 2,3,0 };

        gpu::sMappedBuffer mapped;
        gpu->map (hIdxBuffer, 0, sizeof(u16) * 6, &mapped);
        memcpy (mapped.host_pt, idx, sizeof(u16) * 6);
        gpu->buffer_manualSync (&mapped, 1);
        gpu->buffer_unmap(mapped);        
    }
    

    if (!priv_createDescriptorInstance())
        return false;


    return true;
}

//********************************
bool LineRenderer::priv_createPipeline()
{
    //carico gli shader
    if (!gpu->vtxshader_createFromFile ("@shader/lineRenderer.vert.spv", "main", &hVtxShader))
    {
        gos::logger::err ("LineRenderer::priv_createPipeline() => can't create vert shader\n");
        return false;
    }

    if (!gpu->fragshader_createFromFile ("@shader/lineRenderer.frag.spv", "main", &hFragShader))
    {
        gos::logger::err ("LineRenderer::priv_createPipeline() => can't create frag shader\n");
        return false;
    }    

    //creo la pipeline
    thePipeline->createPipeline (vtxDeclHandle, &hPipeline)
        .setVtxDecl (vtxDeclHandle)
        .addShader (hVtxShader)
        .addShader (hFragShader)
        .descriptor_add (descr2_layout)
        .pushConstant_add (VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(sLineInfo), &pc_lineInfo)
        .setCullMode (eCullMode::CCW)
        .setDrawPrimitive (eDrawPrimitive::trisList)
    .end ();
    if (hPipeline.isInvalid())
    {
        gos::logger::err ("LineRenderer::priv_createPipeline() => can't create pipeline\n");
        return false;
    }

    return true;
}

//********************************
bool LineRenderer::priv_createDescriptorInstance()
{
    //creo una istanza del descrittore2
    if (!thePipeline->createDescriptorInstance (descr2_layout, &descr2_instance))
    {
        gos::logger::err ("LineRenderer::priv_createDescriptorInstance() => can't create descriptorSet instance 2\n");
        return false;
    }

    //creo un buffer per SSBO
    if (!gpu->storageBuffer_create (PER_INSTANCE_SSBO__SIZEOF_ONE_ELEMENT * PER_INSTANCE_SSBO__NUM_MAX_ELEM, eVIBufferMode::shared_cpuW_manualSync, &descr2_ssboHandle))
    {
        gos::logger::err ("LineRenderer::priv_createDescriptorInstance() => GPU::storageBuffer_create\n");
        return false;
    }

    //bind del buffer al descrittore
    gos::gpu::DescrSetInstanceWriter descrWriter;
    descrWriter.begin (gpu, descr2_instance)
        .bindStorageBuffer (0, descr2_ssboHandle)
        .end();


    //scrivo i SSBO
    {
        u32 ct = 0;
        sPerInstanceData    data[64];
        data[ct++].pos.set (0, 0.5f, 0, 0);
        data[ct++].pos.set (3, 0.5f, 0, 0);
        data[ct++].pos.set (3, 0.5f, 3.0f, 0);
        data[ct++].pos.set (0, 0.5f, 3.0f, 0);
        data[ct++].pos.set (0, 0.5f, 0, 0);

        data[ct++].pos.set (0, 3.5f, 0, 0);
        data[ct++].pos.set (3, 3.5f, 0, 0);
        data[ct++].pos.set (3, 3.5f, 3.0f, 0);
        data[ct++].pos.set (0, 3.5f, 3.0f, 0);
        data[ct++].pos.set (0, 3.5f, 0, 0);

        const u32 howMuchToCopy = sizeof(sPerInstanceData) * ct;
        gpu::sMappedBuffer mapped;
        gpu->map (descr2_ssboHandle, 0,howMuchToCopy, &mapped);
        memcpy (mapped.host_pt, data, howMuchToCopy);
        gpu->buffer_manualSync (&mapped, 1);
        gpu->buffer_unmap (mapped);
    }

    return true;
}



//************************************
bool LineRenderer::recordCommandBuffer (gpu::CmdBufferWriter &cw, gos::geom::Camera3 *cam)
{
    //rendering
    cw.bindPipeline (hPipeline)
        .renderPass_begin (thePipeline->hRenderLayout, thePipeline->hFrameBuffer)
            .bindDescriptorSet (thePipeline->descriptorBase_get()->instance, 0)
            .bindDescriptorSet (thePipeline->descriptorScene_get()->instance, 1)
            .bindDescriptorSet (descr2_instance, 2)
            .bindVtxBuffer(hVtxBuffer)
            .bindIdxBufferU16(hIdxBuffer);


    sLineInfo lineInfo;
    lineInfo.width = 15;
            
    cw.pushConstant (pc_lineInfo, &lineInfo, sizeof(lineInfo));

    u32 firstInstance = 0;
    u32 nInstanceToDraw = 4;
    cw.drawIndexed (6,nInstanceToDraw,0,0,firstInstance);
    firstInstance += (nInstanceToDraw+1);

    nInstanceToDraw = 4;
    cw.drawIndexed (6,nInstanceToDraw,0,0,firstInstance);
    firstInstance += (nInstanceToDraw+1);

    cw.renderPass_end();
    return true;
}


