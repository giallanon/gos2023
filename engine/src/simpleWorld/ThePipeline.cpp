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

    //descr set 0
    gpu->deleteResource (descriptorBase.instance);
    gpu->deleteResource (descriptorBase.layout);

    //descr set 1
    gpu->deleteResource (descriptorScene.descr.instance);
    gpu->deleteResource (descriptorScene.descr.layout);
    gpu->deleteResource (descriptorScene.uboHandle);

    pipeStep_clearBuffer.free (gpu);
    pipeStep_present.free (gpu);

    gpu->deleteResource (hRenderLayout);

    gpu->deleteResource (hFrameBuffer);
    //gpu->deleteResource (hSampler0_bilinearFiltering);
    gpu->deleteResource (hDescrPool);

    gpu->deleteResource (vtxDeclHandle);

    vbibstBuffer.unsetup();
    textureList.unsetup ();
    gpu = NULL;
}

//********************************
bool ThePipeline::setup (gos::GPU *gpuIN)
{
    gpu = gpuIN;

    if (!priv_setupVertexDecl())
        return false;

    //vtx idx e stage buffer
    vbibstBuffer.setup (gpu, sizeof(sVertex));

    //creo un descriptor pool
    if (!gpu->descrPool_createNew (&hDescrPool)
        .setMaxNumDescriptorSet(32)
        .addPool_uniformBuffer(8)
        .addPool_sampler(8)
        .addPool_texture(NUM_MAX_TEXTURE)
        .addPool_storageBuffer(8)
        .end())
    {
        gos::logger::err ("ThePipeline::setup() => can't create descriptor pool\n");
        return false;
    }

    if (!priv_createDescriptorBase())
        return false;

    if (!priv_createDescriptorScene())
        return false;

    //creo la pipe step di clear-buffer
    {
        gpu->renderLayout_createNew (&pipeStep_clearBuffer.hRenderLayout)
            .requireRendertarget (gpu->swapChain_getImageFormat(), eImageLayout::undefined, eImageLayout::color_attachment_optimal, eAttachmentLoadOp::clear, eAttachmentStoreOp::store)
            .requireZBuffer (gpu->depthStencil_getDefaultFormat(), eDepthStencilLayout::undefined, eDepthStencilLayout::depth_attachment_optimal, eAttachmentLoadOp::clear, eAttachmentStoreOp::store)
            .addSubpass_GFX()
                .writeToRenderTarget(0)
                .writeToDepthStencil()
            .end()
        .end();
        if (pipeStep_clearBuffer.hRenderLayout.isInvalid())
        {
            gos::logger::err ("ThePipeline::setup() => can't create hRenderLayout for pipeStep_clearBuffer\n");
            return false;
        }

        if (!gpu->vtxshader_createFromFile ("@shader/PIPE_stage_clear.vert.spv", "main", &pipeStep_clearBuffer.hVtxShader))
        {
            gos::logger::err ("ThePipeline::setup() => can't create hVtxShader for pipeStep_clearBuffer\n");
            return false;
        }        


        gpu->pipeline_createNew (pipeStep_clearBuffer.hRenderLayout, &pipeStep_clearBuffer.hPipeline)
            .addShader (pipeStep_clearBuffer.hVtxShader)
            .depthStencil()
                .zbuffer_enable(true)
                .zbuffer_enableWrite(true)
                .zbuffer_setFn (eZFunc::LESS)
                .stencil_enable(false)
            .end() //depth stencil
            .setCullMode (eCullMode::CCW)
            .setDrawPrimitive (eDrawPrimitive::trisList)
        .end ();
        if (pipeStep_clearBuffer.hPipeline.isInvalid())
        {
            gos::logger::err ("ThePipeline::setup() => can't create hPipeline for pipeStep_clearBuffer\n");
            return false;
        }        
    }

    //creo la pipe step di presentazione
    {
        gpu->renderLayout_createNew (&pipeStep_present.hRenderLayout)
            .requireRendertarget (gpu->swapChain_getImageFormat(), eImageLayout::color_attachment_optimal, eImageLayout::presentation, eAttachmentLoadOp::load, eAttachmentStoreOp::dont_care)
            .requireZBuffer (gpu->depthStencil_getDefaultFormat(), eDepthStencilLayout::depth_attachment_optimal, eDepthStencilLayout::depth_attachment_optimal, eAttachmentLoadOp::load, eAttachmentStoreOp::dont_care)
            .addSubpass_GFX()
                .writeToRenderTarget(0)
                .writeToDepthStencil()
            .end()
        .end();
        if (pipeStep_present.hRenderLayout.isInvalid())
        {
            gos::logger::err ("ThePipeline::setup() => can't create hRenderLayout for pipeStep_present\n");
            return false;
        }

        if (!gpu->vtxshader_createFromFile ("@shader/PIPE_stage_clear.vert.spv", "main", &pipeStep_present.hVtxShader))
        {
            gos::logger::err ("ThePipeline::setup() => can't create hVtxShader for pipeStep_present\n");
            return false;
        }        


        gpu->pipeline_createNew (pipeStep_present.hRenderLayout, &pipeStep_present.hPipeline)
            .addShader (pipeStep_present.hVtxShader)
            .depthStencil()
                .zbuffer_enable(true)
                .zbuffer_enableWrite(true)
                .zbuffer_setFn (eZFunc::LESS)
                .stencil_enable(false)
            .end() //depth stencil*/
        .end ();
        if (pipeStep_present.hPipeline.isInvalid())
        {
            gos::logger::err ("ThePipeline::setup() => can't create hPipeline for pipeStep_present\n");
            return false;
        }        
    }    


    //questo invece e' il render layout da utilizzarsi per gli step intermedi di rendering
    gpu->renderLayout_createNew (&hRenderLayout)
        .requireRendertarget (gpu->swapChain_getImageFormat(), eImageLayout::color_attachment_optimal, eImageLayout::color_attachment_optimal, eAttachmentLoadOp::load, eAttachmentStoreOp::store)
        .requireZBuffer (gpu->depthStencil_getDefaultFormat(), eDepthStencilLayout::depth_attachment_optimal, eDepthStencilLayout::depth_attachment_optimal, eAttachmentLoadOp::load, eAttachmentStoreOp::store)
        .addSubpass_GFX()
            .writeToRenderTarget(0)
            .writeToDepthStencil()
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

    return true;
}


//********************************
bool ThePipeline::priv_setupVertexDecl()
{
    gpu->vtxDecl_createNew (&vtxDeclHandle)
        .addStream(eVtxStreamInputRate::perVertex)
            .addLayout (0, 0, eDataFormat::_3f32)
            .addLayout (1, 12, eDataFormat::_3f32)
            .addLayout (2, 24, eDataFormat::_2f32)
        .end();
    if (vtxDeclHandle.isInvalid())
    {
        gos::logger::err ("ThePipeline::priv_setupVertexDecl() => can't create vtxDeclHandle\n");
        return false;
    }


    gos::shape::VtxLayoutWriter vtxLayoutW(&vtxLayout);
    vtxLayoutW.begin()
        .addPos3 (offsetof(sVertex,pos))
        .addNorm3 (offsetof(sVertex,norm))
        .addTexCoord (offsetof(sVertex,tutv0))
    .end();

    return true;
}

//********************************
bool ThePipeline::createDescriptorInstance (const GPUDescrSetLayoutHandle &layout, GPUDescrSetInstanceHandle *out_instance)
{
    return gpu->descrSetInstance_createNew (hDescrPool, layout, out_instance);
}


//********************************
bool ThePipeline::priv_createDescriptorBase()
{
    gos::gpu::DescrSetInstanceWriter descrWriter;

    //descriptor base: texture & sampler
    if (!gpu->descrSetLayout_createDynamic (&descriptorBase.layout)
        .add_sampler (VK_SHADER_STAGE_FRAGMENT_BIT, 8)                  //set 0, binding 0
        .add_texture (VK_SHADER_STAGE_FRAGMENT_BIT, NUM_MAX_TEXTURE)    //set 0, binding 1
        .end())
    {
        gos::logger::err ("ThePipeline::priv_createDescriptorBase() => can't create descriptor set 1\n");
        return false;
    }

    if (!createDescriptorInstance(&descriptorBase))
    {
        gos::logger::err ("ThePipeline::priv_createDescriptorBase() => can't create descriptorSet instance 1\n");
        return false;
    }    


    //creo i sampler
    {
        gpu::SamplerDesc desc;

        //sampler2d: bilinear filtering
        desc.reset();
        gpu->sampler_create (desc, &hSampler0_bilinearFiltering);

        //sampler2d: point filtering
        desc.reset();
        desc.minFilter = desc.magFilter = eSamplerFilter::point;
        desc.mipFilter = eSamplerMipFilter::nearest;
        desc.bAnisotropic = false;
        gpu->sampler_create (desc, &hSampler1_pointFiltering);
        
    }


    //bindo i sampler alla descriptor instance
    descrWriter.begin (gpu, descriptorBase.instance)
        .bindSamplerInArray (0, hSampler0_bilinearFiltering, 0)
        .bindSamplerInArray (0, hSampler1_pointFiltering, 1)
        .end();


    //alloco l'array per le texture
    textureList.setup (localAllocator, gpu, NUM_MAX_TEXTURE);
    return true;
}

//********************************
bool ThePipeline::decriptorBase_addTextureIfNotExitst (const GPUTextureHandle &hTexture, u16 *out_index)
{
    assert (NULL != out_index);
    assert (hTexture.isValid());

    u16 indexOfTexture;
    switch (textureList.addIfNotExists (hTexture, &indexOfTexture))
    {
    default:
    case 0:
        gos::logger::err ("ThePipeline::addTextureIfNotExitst () => can't add texture\n");
        return false;

    case 1:
        //la texture non esisteva nell'array, e' stata inserita ora per la prima volta
        //Devo aggiornare il descriptor set
        {
            gos::gpu::DescrSetInstanceWriter descrWriter;
            descrWriter.begin (gpu, descriptorBase.instance)
                .bindTextureInArray (1, hTexture, indexOfTexture)
            .end();

        }
        break;

    case 2:
        //la texture era gia' nell'array, non dove fare nulla di speciale
        break;
    }


    *out_index= indexOfTexture;
    return true;
}

//********************************
bool ThePipeline::priv_createDescriptorScene()
{
    //Creo il descriptorSet layout 1 (scene data)
    if (!gpu->descrSetLayout_createStatic (&descriptorScene.descr.layout)
        .add_uniformBuffer (VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT) //set 1, binding 0
        .end())
    {
        gos::logger::err ("ThePipeline::priv_createDescriptorScene() => can't create descriptor set 0\n");
        return false;
    }

    if (!createDescriptorInstance(&descriptorScene.descr))
    {
        gos::logger::err ("ThePipeline::priv_createDescriptorScene() => can't create descriptorSet instance 0\n");
        return false;
    }

 
    //creo un buffer per UBO
    if (!gpu->uniformBuffer_create (sizeof(sSceneData), eVIBufferMode::shared_cpuW_autoSync, &descriptorScene.uboHandle))
    {
        gos::logger::err ("ThePipeline::priv_createDescriptorScene() => GPU::uniformBuffer_create\n");
        return false;
    }

    //Bind del UBO al descrittore
    gos::gpu::DescrSetInstanceWriter descrWriter;

    descrWriter.begin (gpu, descriptorScene.descr.instance)
        .bindUniformBuffer (0, descriptorScene.uboHandle)
        .end();
 
    return true;
}


//********************************
bool ThePipeline::shape_uploadToVBIB (const gos::Shape *shape, tpp::sBoundShapeInfo *out_info)
{
    if (!vbibstBuffer.upload (shape, out_info))
    {
        gos::logger::err ("ThePipeline::shape_uploadToVBIB() => can't upload to VtxBuffer\n");
        return false;
    }

    return true;
}

//********************************
gos::GPU::PipelineBuilder& ThePipeline::createPipeline (GPUPipelineHandle *out_handle)
{
    return createPipeline (vtxDeclHandle, out_handle);
}
gos::GPU::PipelineBuilder& ThePipeline::createPipeline (const GPUVtxDeclHandle hVtxDeclHandle_IN, GPUPipelineHandle *out_handle)
{
    return gpu->pipeline_createNew (hRenderLayout, out_handle)
        .setVtxDecl (hVtxDeclHandle_IN)
        .depthStencil()
            .zbuffer_enable(true)
            .zbuffer_enableWrite(true)
            .zbuffer_setFn (eZFunc::LESS)
            .stencil_enable(false)
        .end() //depth stencil
        .setCullMode (eCullMode::CCW)
        .setDrawPrimitive (eDrawPrimitive::trisList)
        .descriptor_add (descriptorBase.layout)
        .descriptor_add (descriptorScene.descr.layout);        
}

//********************************
bool ThePipeline::beginFrame (Context &ctx)
{
    //aggiornamento del descrittore con i dati di scena
    {
        gos::vec3f lightDir (-0.2f, -0.6f, 0.2f);
        //gos::vec3f lightDir (0, -1, 0);
        lightDir.normalize();
    
        const f32 ambientLightIntensity = 0.1f;
    
        descriptorScene.sceneData.camVP = ctx.cam->getMatVP();
        descriptorScene.sceneData.lightDir.set (lightDir, ambientLightIntensity);
        descriptorScene.sceneData.screenWH.set ((f32)gpu->swapChain_getWidth(), (f32)gpu->swapChain_getHeight());
        gpu->writeAndSync (descriptorScene.uboHandle, 0, &descriptorScene.sceneData, sizeof(descriptorScene.sceneData));            
    }

    //rendering: step0, clear buffer
    ctx.cw->setViewport (gpu->viewport_getDefault())
        .bindPipeline (pipeStep_clearBuffer.hPipeline)
        .setClearColor (0, gos::ColorHDR(0, 0.1f, 0.3f))
        .setDepthBufferColor(1, 0)
        .renderPass_begin (pipeStep_clearBuffer.hRenderLayout, this->hFrameBuffer)
        .renderPass_end();

    return true;
}

//********************************
void ThePipeline::endFrame(Context &ctx)
{
    //rendering: step: preset
    ctx.cw->bindPipeline (pipeStep_present.hPipeline)
        .renderPass_begin (pipeStep_present.hRenderLayout, this->hFrameBuffer)
        .renderPass_end();
        
    ctx.cw->end();
}