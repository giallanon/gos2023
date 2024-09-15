#include "Renderer1.h"
#include "../gosShape/gosShapeImport.h"

using namespace gos;



//********************************
Renderer1::Renderer1()
{
    gpu = NULL;
    localAllocator = GOSNEW(gos::getSysHeapAllocator(), LocalAllocator)("renderer1");
    localAllocator->setup (1024 * 1024);

    //definizione del vtx layout
    shape::VtxLayoutWriter writer(&vtxLayout);
    writer.begin()
        .addPos3(offsetof(sVertex, pos))
        .addNorm3(offsetof(sVertex, norm))
        .addTexCoord(offsetof(sVertex, tutv0))
    .end();    
}

//********************************
Renderer1::~Renderer1()
{
    if (NULL != gpu)
    {
        gos::shape::shapeFree (gos::getSysHeapAllocator(), &shapeSfera);

        gpu->deleteResource (hRenderLayout);
        gpu->deleteResource (hFrameBuffer);
        gpu->deleteResource (hPipeline);
        gpu->deleteResource (hVtxShader);
        gpu->deleteResource (hFragShader);
        gpu->deleteResource (hVtxBuffer);
        gpu->deleteResource (hIdxBuffer);
        gpu->deleteResource (hStgBuffer);

        gpu->deleteResource (hDescrSetInstance_0);
        gpu->deleteResource (hDescrSetLayout_0);
        gpu->deleteResource (hDescrset0_ubo);

//gpu->deleteResource (hDescrSetInstance_1);
        gpu->deleteResource (hDescrSetLayout_1);
        gpu->deleteResource (material1.hUBO);
        gpu->deleteResource (material2.hUBO);

        gpu->deleteResource (hDescrPool);

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

    //test 1
    priv_createSfera();


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
    gpu->descrSetLayout_createStatic (&hDescrSetLayout_0)
        .add_uniformBuffer (VK_SHADER_STAGE_VERTEX_BIT) //set 0, binding 0
        .end();
    if (hDescrSetLayout_0.isInvalid())
    {
        gos::logger::err ("Renderer1::setup() => can't create descriptor set\n");
        return false;
    }

    //Creo il descriptorSet layout 1
    gpu->descrSetLayout_createPushable (&hDescrSetLayout_1)
        .add_uniformBuffer (VK_SHADER_STAGE_VERTEX_BIT) //set 1, binding 0
        .end();
    if (hDescrSetLayout_1.isInvalid())
    {
        gos::logger::err ("Renderer1::setup() => can't create descriptor set\n");
        return false;
    }     

    if (!priv_createPipeline())
    {
        gos::logger::err ("Renderer1::setup() => can't create pipeline\n");
        return false;
    }


    //creo un descriptor pool
    gpu->descrPool_createNew (&hDescrPool)
        .setMaxNumDescriptorSet(4)
        .addPool_uniformBuffer()
        .addPool_uniformBuffer()
        .end();
    if (hDescrPool.isInvalid())
    {
        gos::logger::err ("VulkanApp::init() => can't create descriptor pool\n");
        return false;
    }

    //alloco una istanza del descriptorSet
    if (!gpu->descrSetInstance_createNew (hDescrPool, hDescrSetLayout_0, &hDescrSetInstance_0))
    {
        gos::logger::err ("VulkanApp::init() => can't create descriptorSet instance\n");
        return false;
    }

/*    if (!gpu->descrSetInstance_createNew (hDescrPool, hDescrSetLayout_1, &hDescrSetInstance_1))
    {
        gos::logger::err ("VulkanApp::init() => can't create descriptorSet instance\n");
        return false;
    }
*/


    //creo un buffer per UBO
    if (!gpu->uniformBuffer_create (sizeof(sDescrSet0_UBO), &hDescrset0_ubo))
    {
        gos::logger::err ("Renderer1::setup() => GPU::uniformBuffer_create\n");
        return false;
    }

    if (!gpu->uniformBuffer_create (sizeof(material1.data), &material1.hUBO))
    {
        gos::logger::err ("Renderer1::setup() => GPU::uniformBuffer_create\n");
        return false;
    }
    if (!gpu->uniformBuffer_create (sizeof(material2.data), &material2.hUBO))
    {
        gos::logger::err ("Renderer1::setup() => GPU::uniformBuffer_create\n");
        return false;
    }

    return priv_createVBIB();
}

//********************************
bool Renderer1::priv_createPipeline()
{
    //Vtx declaration
    GPUVtxDeclHandle vtxDeclHandle;
    gpu->vtxDecl_createNew (&vtxDeclHandle)
        .addStream(eVtxStreamInputRate::perVertex)
            .addLayout (0, offsetof(sVertex, pos), eDataFormat::_3f32)
            .addLayout (1, offsetof(sVertex, norm), eDataFormat::_3f32)
            .addLayout (2, offsetof(sVertex, tutv0), eDataFormat::_2f32)
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
        .pushConstant_add (eShaderType::vertexShader, 0, sizeof(gos::mat4x4f), &pc_objWorldPos)
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

//********************************
bool Renderer1::priv_createVBIB()
{
    //vtx buffer (stream 0)
    if (!gpu->vertexBuffer_create (sizeof(sVertex) * VTXBUFFER_MAX_NUM_VTX, eVIBufferMode::onGPU, &hVtxBuffer))
    {
        gos::logger::err ("Renderer1::priv_createVBIB() => gpu->vertexBuffer_create() failed\n");
        return false;
    }

    //index buffer
    if (!gpu->indexBuffer_create (sizeof(u16) * IDXBUFFER_MAX_NUM_IDX, eVIBufferMode::onGPU, &hIdxBuffer))
    {
        gos::logger::err ("Renderer1::priv_createVBIB() => gpu->indexBuffer_create() failed\n");
        return false;
    }

    //Creo anche uno staging buffer
    if (!gpu->stagingBuffer_create (STGBUFFER_SIZE, &hStgBuffer))
    {
        gos::logger::err ("Renderer1::priv_createVBIB() => gpu->stagingBuffer_create() failed\n");
        return false;
    }

    return true;
}


//************************************
#include "../gosShape/gosShapePrefabs.h"
bool Renderer1::priv_createSfera()
{
    shapeSfera.reset();

    gos::shape::VtxLayout vtxLayout;
    gos::shape::VtxLayoutWriter vtxLayoutW(&vtxLayout);
    vtxLayoutW.begin()
        .addPos3 (offsetof(sVertex,pos))
        .addNorm3 (offsetof(sVertex,norm))
        .addTexCoord (offsetof(sVertex,tutv0))
    .end();

    const f32 radius = 1.0f;
    gos::shape::buildSphere (vec3f(0,0,0), vec3f(radius, radius, radius), 16, 6, vtxLayout, gos::getSysHeapAllocator(), &shapeSfera);


    //copio i Vtx in vtxBuffer e idx in idxBuffer tramite lo staging buffer
    if (!gpu->stagingBuffer_uploadToGPUBuffer (hStgBuffer, shapeSfera.vtxBuffer, hVtxBuffer, 0, sizeof(sVertex) * shapeSfera.numVtx))
    {
        gos::logger::err ("Renderer1::priv_createSfera() => can't upload to VtxBuffer\n");
        return false;
    }


    if (!gpu->stagingBuffer_uploadToGPUBuffer (hStgBuffer, shapeSfera.idxBuffer, hIdxBuffer, 0, sizeof(u16) * shapeSfera.numIdx))
    {
        gos::logger::err ("Renderer1::priv_createSfera() => can't upload to IdxBuffer\n");
        return false;
    }

    return true;
}   

//************************************
bool Renderer1::recordCommandBuffer (gpu::CmdBufferWriter &cw, gos::geom::Camera3 *cam)
{
    gos::gpu::DescrSetInstanceWriter descrWriter;

    //descriptor set 0
    {
        gos::vec3f lightDir (-0.2f, -0.6f, 0.2f);
        lightDir.normalize();

        descrset0_ubo.camVP = cam->getMatVP();
        descrset0_ubo.lightDir.set (lightDir, 0.01f);
        gpu->uniformBuffer_mapCopyUnmap (hDescrset0_ubo, 0, sizeof(descrset0_ubo), &descrset0_ubo);            

        descrWriter.begin (gpu, hDescrSetInstance_0)
            .bindUniformBuffer (0, hDescrset0_ubo)
            .end();
    }

    //descriptor set 1
    {
        material1.data.color.set (0,1,0);
        gpu->uniformBuffer_mapCopyUnmap (material1.hUBO, 0, sizeof(sMaterialData), &material1.data);            


        material2.data.color.set (0,0,1);
        gpu->uniformBuffer_mapCopyUnmap (material2.hUBO, 0, sizeof(sMaterialData), &material2.data);            

        //descrWriter.begin (gpu, hDescrSetInstance_1)
        //    .updateUniformBuffer (0, hDescrset1_ubo)
        //    .end();
    }

    //world position dell'obj
    gos::mat4x4f objW;
    objW.identity();

    gos::mat4x4f objW2;
    objW2.buildTranslation (3,0,0);
    
    gos::mat4x4f objW3;
    objW3.buildTranslation (5,0,0);

    cw.setViewport (gpu->viewport_getDefault())
        .bindPipeline (hPipeline)
        .setClearColor (0, gos::ColorHDR(0, 0.1f, 0.3f))
        .setDepthBufferColor(1, 0)
        .renderPass_begin (hRenderLayout, hFrameBuffer)
            .bindDescriptorSet (hDescrSetInstance_0, 0)
//.bindDescriptorSet (hDescrSetInstance_1, 1)
            .bindVtxBuffer(hVtxBuffer)
            .bindIdxBufferU16(hIdxBuffer)
            
            .pushDescriptor_begin (1)
                .pushDescriptor_UBO (material1.hUBO, 0)
            .pushDescriptor_end()
            .pushConstant (pc_objWorldPos, &objW, sizeof(objW))
            .drawIndexed (shapeSfera.numIdx, 1, 0, 0, 0)
            
            .pushDescriptor_begin (1)
                .pushDescriptor_UBO (material2.hUBO, 0)
            .pushDescriptor_end()
            .pushConstant (pc_objWorldPos, &objW2, sizeof(objW))
            .drawIndexed (shapeSfera.numIdx, 1, 0, 0, 0)

            .pushConstant (pc_objWorldPos, &objW3, sizeof(objW))
            .drawIndexed (shapeSfera.numIdx, 1, 0, 0, 0)

        .renderPass_end();
    return true;
}

//************************************
bool Renderer1::addModelFrom_glTF (const char *filename, hModel *out)
{
    assert (NULL != out);

    gos::ShapeList shapeList(gos::getScrapAllocator(), 128);
    if (!shape::importFrom_glTF (filename, vtxLayout, localAllocator, shapeList))
    {
        out->setInvalid();
        gos::logger::err ("Renderer1::addModelFrom_glTF(%s) => unable to load model\n", filename);
        return false;
    }

    return addModel (shapeList, out);
}

//************************************
bool Renderer1::addModel (const gos::ShapeList &shapeList, hModel *out)
{
    assert (NULL != out);
    out->setInvalid();

    return false;
}

//************************************
bool Renderer1::addInstance (const hModel &hModel, const gos::geom::Pos3 &pos)
{
    /*
    una instance è composta da
        1 shape                 => vtx buffer / idx buffer
        1 material instance     => texture(s) e parametri vari
        1 world pos
    

    renderer deve sortare per
        vtxShader/idxShader         (aka material)
        vtxBuffer, idexBuffer       (aka shape)
        material instance           (aka parametri specifici del materiale)
        shape world pos             => da sfruttare la drawInstance a parità di material instance

    */
   return false;
}
