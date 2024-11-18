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


    gpu->deleteResource (hRenderLayout);
    gpu->deleteResource (hRenderLayoutClearBuffer);
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


    //creo il render layout
    gpu->renderLayout_createNew (&hRenderLayoutClearBuffer)
        .requireRendertarget (gpu->swapChain_getImageFormat(), eRenderTargetUsage::dont_care, eRenderTargetUsage::storage_color_attachment_optimal, true)
        .requireZBuffer (gpu->depthStencil_getDefaultFormat(), eZBufferUsage::dont_care, eZBufferUsage::depthOnly_RW, true)
        .addSubpass_GFX()
            .useRenderTarget(0)
            .useDepthStencil()
        .end()
    .end();
    if (hRenderLayoutClearBuffer.isInvalid())
    {
        gos::logger::err ("ThePipeline::setup() => can't create renderTaskLayout\n");
        return false;
    }

    gpu->renderLayout_createNew (&hRenderLayout)
        .requireRendertarget (gpu->swapChain_getImageFormat(), eRenderTargetUsage::storage_color_attachment_optimal, eRenderTargetUsage::presentation, false)
        .requireZBuffer (gpu->depthStencil_getDefaultFormat(), eZBufferUsage::depthOnly_RW, eZBufferUsage::dont_care, false)
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
void ThePipeline::descritproScene_update (gos::geom::Camera3 *cam)
{
    gos::vec3f lightDir (-0.2f, -0.6f, 0.2f);
    //gos::vec3f lightDir (0, -1, 0);
    lightDir.normalize();

    const f32 ambientLightIntensity = 0.1f;

    descriptorScene.sceneData.camVP = cam->getMatVP();
    descriptorScene.sceneData.lightDir.set (lightDir, ambientLightIntensity);
    gpu->writeAndSync (descriptorScene.uboHandle, 0, &descriptorScene.sceneData, sizeof(descriptorScene.sceneData));            
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

