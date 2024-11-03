#include "Renderer1.h"
#include "../gosShape/gosShapeImport.h"

using namespace gos;



//********************************
Renderer1::Renderer1()
{
    gpu = NULL;
    localAllocator = GOSNEW(gos::getSysHeapAllocator(), LocalAllocator)("renderer1");
    localAllocator->setup (1024 * 1024);
}

//********************************
Renderer1::~Renderer1()
{
    if (NULL != gpu)
    {
        gpu->deleteResource (hRenderLayout);
        gpu->deleteResource (hFrameBuffer);
        gpu->deleteResource (hPipeline);
        gpu->deleteResource (hVtxShader);
        gpu->deleteResource (hFragShader);

        gpu->deleteResource (hDescrSetInstance_0);
        gpu->deleteResource (hDescrSetLayout_0);
        gpu->deleteResource (hDescrset0_ubo);

        gpu->deleteResource (hDescrSetInstance_1);
        gpu->deleteResource (hDescrSetLayout_1);

        gpu->deleteResource (hDescrSetInstance_2);
        gpu->deleteResource (hDescrSetLayout_2);
        gpu->deleteResource (hDescrset2_ssbo);

        gpu->deleteResource (hDescrPool);

        textureList.unsetup ();
        materialList.unsetup();

        shapeList.unsetup();
        instanceList.unsetup();

        gpu = NULL;
    }

    if (NULL != localAllocator)
    {
        GOSDELETE(gos::getSysHeapAllocator(), localAllocator);
        localAllocator = NULL;
    }
}

//********************************
bool Renderer1::setup (gos::GPU *gpuIN)
{
    gpu = gpuIN;
    if (!priv_setupVulkan())
        return false;

    materialList.setup (localAllocator, NUM_MAX_MATERIAL);
    textureList.setup (localAllocator, gpu, NUM_MAX_TEXTURE);
    shapeList.setup (localAllocator, 8192);
    instanceList.setup (localAllocator, 1024);

    return true;
}

//********************************
bool Renderer1::priv_setupVulkan()
{
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
        gos::logger::err ("Renderer1::setup() => can't create renderTaskLayout\n");
        return false;
    }

    //frame buffers
    gpu->frameBuffer_createNew (hRenderLayout, &hFrameBuffer)
        .bindRenderTarget (gpu->renderTarget_getDefault())
        .bindDepthStencil (gpu->depthStencil_getDefault())
        .end();
    if (hFrameBuffer.isInvalid())
    {
        gos::logger::err ("Renderer1::setup() => can't create frameBufferHandle\n");
        return false;
    }
    
    //Creo il descriptorSet layout 0
    if (!gpu->descrSetLayout_createStatic (&hDescrSetLayout_0)
        .add_uniformBuffer (VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT) //set 0, binding 0
        .end())
    {
        gos::logger::err ("Renderer1::setup() => can't create descriptor set 0\n");
        return false;
    }

    //Creo il descriptorSet layout 1
    if (!gpu->descrSetLayout_createDynamic (&hDescrSetLayout_1)
        .add_sampler (VK_SHADER_STAGE_FRAGMENT_BIT, 8)                   //set 1, binding 0
        .add_texture (VK_SHADER_STAGE_FRAGMENT_BIT, NUM_MAX_TEXTURE)     //set 1, binding 1
        .end())
    {
        gos::logger::err ("Renderer1::setup() => can't create descriptor set 1\n");
        return false;
    }

    //Creo il descriptorSet layout 2
    if (!gpu->descrSetLayout_createStatic (&hDescrSetLayout_2)
        .add_dynamicStorageBuffer (VK_SHADER_STAGE_FRAGMENT_BIT) //set 2, binding 0
        .end())
    {
        gos::logger::err ("Renderer1::setup() => can't create descriptor set 2\n");
        return false;
    }       

    if (!priv_createPipeline())
    {
        gos::logger::err ("Renderer1::setup() => can't create pipeline\n");
        return false;
    }

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

    //alloco una istanza del descriptorSet
    if (!gpu->descrSetInstance_createNew (hDescrPool, hDescrSetLayout_0, &hDescrSetInstance_0))
    {
        gos::logger::err ("VulkanApp::init() => can't create descriptorSet instance 0\n");
        return false;
    }

    if (!gpu->descrSetInstance_createNew (hDescrPool, hDescrSetLayout_1, &hDescrSetInstance_1))
    {
        gos::logger::err ("VulkanApp::init() => can't create descriptorSet instance 1\n");
        return false;
    }
    
    if (!gpu->descrSetInstance_createNew (hDescrPool, hDescrSetLayout_2, &hDescrSetInstance_2))
    {
        gos::logger::err ("VulkanApp::init() => can't create descriptorSet instance 2\n");
        return false;
    }
    
    //creo un sampler
    gpu->sampler_create (gpu::SamplerDesc(), &hSampler_diffuse);


    //creo un buffer per UBO
    if (!gpu->uniformBuffer_create (sizeof(sDescrSet0_UBO), eVIBufferMode::shared_cpuW_autoSync, &hDescrset0_ubo))
    {
        gos::logger::err ("Renderer1::setup() => GPU::uniformBuffer_create\n");
        return false;
    }

    //creo un buffer per SSBO
    if (!gpu->storageBuffer_create (SIZEOF_ONE_ELEMENT_IN_MATERIAL_SSBO * 1024, eVIBufferMode::shared_cpuW_manualSync, &hDescrset2_ssbo))
    {
        gos::logger::err ("Renderer1::setup() => GPU::storageBuffer_create\n");
        return false;
    }


    //bind dei buffer ai descrittori
    {
        gos::gpu::DescrSetInstanceWriter descrWriter;

        descrWriter.begin (gpu, hDescrSetInstance_0)
            .bindUniformBuffer (0, hDescrset0_ubo)
            .end();

        descrWriter.begin (gpu, hDescrSetInstance_1)
            .bindSamplerInArray (0, hSampler_diffuse, 0)
            .end();

        descrWriter.begin (gpu, hDescrSetInstance_2)
            .bindDynamicStorageBuffer (0, hDescrset2_ssbo, SIZEOF_ONE_ELEMENT_IN_MATERIAL_SSBO)
            .end();
    }
    return true;
}

//********************************
bool Renderer1::priv_createPipeline()
{
    //Vtx declaration
    GPUVtxDeclHandle vtxDeclHandle;
    gpu->vtxDecl_createNew (&vtxDeclHandle)
        .addStream(eVtxStreamInputRate::perVertex)
            .addLayout (0, 0, eDataFormat::_3f32)
            .addLayout (1, 12, eDataFormat::_3f32)
            .addLayout (2, 24, eDataFormat::_2f32)
        .end();
    if (vtxDeclHandle.isInvalid())
    {
        gos::logger::err ("Renderer1::priv_createPipeline() => can't create vtxDeclHandle\n");
        return false;
    }

    //carico gli shader
    if (!gpu->vtxshader_createFromFile ("@shader/phong.vert.spv", "main", &hVtxShader))
    {
        gos::logger::err ("Renderer1::priv_createPipeline() => can't create vert shader\n");
        return false;
    }

    if (!gpu->fragshader_createFromFile ("@shader/phong.frag.spv", "main", &hFragShader))
    {
        gos::logger::err ("Renderer1::priv_createPipeline() => can't create frag shader\n");
        return false;
    }    

    //creo la pipeline
    gpu->pipeline_createNew (hRenderLayout, &hPipeline)
        .addShader (hVtxShader)
        .addShader (hFragShader)
        .setVtxDecl (vtxDeclHandle)
        .depthStencil()
            .zbuffer_enable(true)
            .zbuffer_enableWrite(true)
            .zbuffer_setFn (eZFunc::LESS)
            .stencil_enable(false)
        .end() //depth stencil
        .setCullMode (eCullMode::CCW)
        .setDrawPrimitive (eDrawPrimitive::trisList)
        .descriptor_add (hDescrSetLayout_0)
        .descriptor_add (hDescrSetLayout_1)
        .descriptor_add (hDescrSetLayout_2)
        .pushConstant_add (VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(gos::mat4x4f), &pc_objWorldPos)
        //.setWireframe(true)
    .end ();

    if (hPipeline.isInvalid())
    {
        gos::logger::err ("Renderer1::priv_createPipeline() => can't create pipeline\n");
        return false;
    }

    //non mi serve piu'
    gpu->deleteResource (vtxDeclHandle);


    return true;
}

//************************************
bool Renderer1::recordCommandBuffer (gpu::CmdBufferWriter &cw, gos::geom::Camera3 *cam)
{
    //update descriptor set 0
    {
        gos::vec3f lightDir (-0.2f, -0.6f, 0.2f);
        lightDir.normalize();

        descrset0_ubo.camVP = cam->getMatVP();
        descrset0_ubo.lightDir.set (lightDir, 0.01f);
        gpu->writeAndSync (hDescrset0_ubo, 0, &descrset0_ubo, sizeof(descrset0_ubo));            
    }

    static f32 debug_red = 1.0f;
    static f32 debug_redInc = 0.001f;
    {
        Material *m;
        materialList.get (0, &m);
        m->colorDiffuse.x = debug_red;
        
        debug_red += debug_redInc;
        if (debug_red <0 || debug_red > 1.0f)
            debug_redInc = -debug_redInc;

        gpu::sMappedBuffer mapped;
        gpu->map (hDescrset2_ssbo, SIZEOF_ONE_ELEMENT_IN_MATERIAL_SSBO*1, sizeof(Material), &mapped);
        memcpy (mapped.host_pt, m, sizeof(Material));
        gpu->buffer_manualSync (&mapped, 1);
        gpu->buffer_unmap(mapped);

    }


    
    //rendering
    cw.setViewport (gpu->viewport_getDefault())
        .bindPipeline (hPipeline)
        .setClearColor (0, gos::ColorHDR(0, 0.1f, 0.3f))
        .setDepthBufferColor(1, 0)
        .renderPass_begin (hRenderLayout, hFrameBuffer)
            .bindDescriptorSet (hDescrSetInstance_0, 0)
            .bindDescriptorSet (hDescrSetInstance_1, 1);

    const u32 n = instanceList.getNElem();
    for (u32 i=0; i<n; i++)
    {
        gos::mat4x4f objW;
        instanceList(i).worldPos.getMatrix4x4(&objW);

        //const Material *material;
        //materialList.get (instanceList(i).indexOf_material, &material);
        const u16 materialIndex = instanceList(i).indexOf_material;


        const VBIBSTBuffer::sUploadInfo *shapeInfo;
        shapeList.get (instanceList(i).indexOf_shape, &shapeInfo);

        
        cw  .bindVtxBuffer(shapeInfo->hVtxBuffer)
            .bindIdxBufferU16(shapeInfo->hIdxBuffer)
            .bindDescriptorSet (hDescrSetInstance_2, 2, materialIndex * SIZEOF_ONE_ELEMENT_IN_MATERIAL_SSBO)
            .pushConstant (pc_objWorldPos, &objW, sizeof(objW))
            .drawIndexed (shapeInfo->numIdx, 1, shapeInfo->startIdx, shapeInfo->startVtx, 0);
            
    }
    cw.renderPass_end();


    return true;
}


//************************************
bool Renderer1::material_create (const GPUTextureHandle &hDiffuseTex, const gos::vec3f &diffuseCol, u16 *out_index)
{
    assert (sizeof(Material) <= SIZEOF_ONE_ELEMENT_IN_MATERIAL_SSBO);

    Material material;
    material.colorDiffuse = vec4f(diffuseCol, 0);

    u16 indexOfTexture;
    switch (textureList.addIfNotExists (hDiffuseTex, &indexOfTexture))
    {
    default:
    case 0:
        gos::logger::err ("Renderer1::material_create () => can't add texture\n");
        return false;

    case 1:
        //la texture non esisteva nell'array, e' stata inserita ora per la prima volta
        //Devo aggiornare il descriptor set
        {
            gos::gpu::DescrSetInstanceWriter descrWriter;
            descrWriter.begin (gpu, hDescrSetInstance_1)
                .bindTextureInArray (1, hDiffuseTex, indexOfTexture)
            .end();

        }
        break;

    case 2:
        //la texture era gia' nell'array, non dove fare nulla di speciale
        break;
    }
    material.indexOf_texDiffuse = indexOfTexture;

    u16 index;
    if (!materialList.add (material, &index))
        return false;
    *out_index= index;

    
    //gpu->writeAndSync (hDescrset2_ssbo, offset, &material, sizeof(Material));
    const u32 offset = SIZEOF_ONE_ELEMENT_IN_MATERIAL_SSBO * index;
    gpu::sMappedBuffer map;
    gpu->map (hDescrset2_ssbo, offset, sizeof(Material), &map);
    memcpy (map.host_pt, &material, sizeof(Material));

    gpu->buffer_manualSync (&map, 1);
    gpu->buffer_unmap (map);
    
    return true;
}

//************************************
bool Renderer1::shape_add (const VBIBSTBuffer::sUploadInfo &shape, u16 *out_index)
{
    if (!shapeList.add (shape, out_index))
    {
        gos::logger::err ("Renderer1::shape_add () => can't add shape\n");
        return false;
    }

    return true;    
}

//************************************
bool Renderer1::instance_add (u16 indexOf_shape, u16 indexOf_material, const gos::geom::Pos3 &worldPos)
{
    const u32 n = instanceList.getNElem();
    instanceList[n].indexOf_material = indexOf_material;
    instanceList[n].indexOf_shape = indexOf_shape;
    instanceList[n].worldPos = worldPos;

   return true;
}
