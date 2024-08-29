#include "VulkanExample6.h"
#include "../gosShape/gosShapeImport.h"


using namespace gos;


//************************************
VulkanExample6::VulkanExample6()
{
}

//************************************
void VulkanExample6::virtual_explain()
{
    gos::logger::log ("import da .dae\n");
    gos::logger::log (eTextColor::white, "TAB = toggle mouse mode\n");
}


//************************************
void VulkanExample6::virtual_onCleanup() 
{
    shape::shapeFree (gos::getSysHeapAllocator(), &myShape);
    gpu->deleteResource (idxBufferHandle);
    gpu->deleteResource (stgBufferHandle);
    gpu->deleteResource (vtxBufferHandle);
    gpu->deleteResource (vtxShaderHandle);
    gpu->deleteResource (fragShaderHandle);
    gpu->deleteResource (pipelineHandle);
    gpu->deleteResource (renderLayoutHandle);
    gpu->deleteResource (frameBufferHandle);
    gpu->deleteResource (uboHandle);
    gpu->deleteResource (descrSetInstancerHandle);
    gpu->deleteResource (descrSetLayoutHandle);
    gpu->deleteResource (descrPoolHandle);
}    


//************************************
bool VulkanExample6::virtual_onInit ()
{
    //importazione .dae
    //gos::shape::importFromCollada ("shader/example6/esempio.dae", gos::getSysHeapAllocator(), &myShape);
    gos::shape::importFromCollada ("shader/example6/omino2.dae", gos::getSysHeapAllocator(), &myShape);


    //creo vtx/idx/staging buffer
    if (!createVertexIndexStageBuffer())
    {
        gos::logger::err ("VulkanApp::init() => can't create buffers\n");
        return false;
    }

    //copio i Vtx in vtxBuffer e idx in idxBuffer tramite lo staging array
    {
        if (!gpu->stagingBuffer_uploadToGPUBuffer (stgBufferHandle, myShape.vtxBuffer, vtxBufferHandle, 0, sizeof(Vertex) * myShape.numVtx))
        {
            gos::logger::err ("VulkanApp::init() => can't upload to VtxBuffer\n");
            return false;
        }

        if (!gpu->stagingBuffer_uploadToGPUBuffer (stgBufferHandle, myShape.idxBuffer, idxBufferHandle, 0, sizeof(u16) * myShape.numIdx))
        {
            gos::logger::err ("VulkanApp::init() => can't upload to IdxBuffer\n");
            return false;
        }
    }




    //Vtx declaration
    GPUVtxDeclHandle vtxDeclHandle;
    gpu->vtxDecl_createNew (&vtxDeclHandle)
        .addStream(eVtxStreamInputRate::perVertex)
        .addLayout (0, offsetof(Vertex, pos), eDataFormat::_3f32)        //position
        .addLayout (1, offsetof(Vertex, normal), eDataFormat::_3f32)   //color
        .end();
    if (vtxDeclHandle.isInvalid())
    {
        gos::logger::err ("VulkanApp::init() => can't create vtxDeclHandle\n");
        return false;
    }


    //creo il render pass
    gpu->renderLayout_createNew (&renderLayoutHandle)
        .requireRendertarget (eRenderTargetUsage::presentation, gpu->swapChain_getImageFormat(), true)
        .requireDepthStencil (gpu->depthStencil_getDefaultFormat(), false, true)
        .addSubpass_GFX()
            .useRenderTarget(0)
            .useDepthStencil()
        .end()
    .end();
    if (renderLayoutHandle.isInvalid())
    {
        gos::logger::err ("VulkanApp::init() => can't create renderTaskLayout\n");
        return false;
    }

    //frame buffers
    gpu->frameBuffer_createNew (renderLayoutHandle, &frameBufferHandle)
        .bindRenderTarget (gpu->renderTarget_getDefault())
        .bindDepthStencil (gpu->depthStencil_getDefault())
        .end();
    if (frameBufferHandle.isInvalid())
    {
        gos::logger::err ("VulkanApp::init() => can't create frameBufferHandle\n");
        return false;
    }        


    //carico gli shader
    fs::addAlias ("@shader", "shader/example6", eAliasPathMode::relativeToAppFolder);
    if (!gpu->vtxshader_createFromFile ("@shader/shader.vert.spv", "main", &vtxShaderHandle))
    {
        gos::logger::err ("VulkanApp::init() => can't create vert shader\n");
        return false;
    }
    if (!gpu->fragshader_createFromFile ("@shader/shader.frag.spv", "main", &fragShaderHandle))
    {
        gos::logger::err ("VulkanApp::init() => can't create frag shader\n");
        return false;
    }

    //Creo il descriptorSet layout  con un solo UNIFORM BUFFER per il VTX SHADER
    gpu->descrSetLayout_createNew(&descrSetLayoutHandle)
        .add (VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
        .end();
    if (descrSetLayoutHandle.isInvalid())
    {
        gos::logger::err ("VulkanApp::init() => can't create descriptor set\n");
        return false;
    }

    //creo la pipeline
    gpu->pipeline_createNew (renderLayoutHandle, &pipelineHandle)
        .addShader (vtxShaderHandle)
        .addShader (fragShaderHandle)
        .setVtxDecl (vtxDeclHandle)
        .depthStencil()
            .zbuffer_enable(true)
            .zbuffer_enableWrite(true)
            .zbuffer_setFn (eZFunc::LESS)
        .end() //depth stencil
        .setCullMode (eCullMode::CCW)
        .setDrawPrimitive (eDrawPrimitive::trisList)
        .descriptor_add (descrSetLayoutHandle)
        .end ();

    if (pipelineHandle.isInvalid())
    {
        gos::logger::err ("VulkanApp::init() => can't create pipeline\n");
        return false;
    }

    //non mi serve piu'
    gpu->deleteResource (vtxDeclHandle);


    //creo un buffer per UBO
    if (!gpu->uniformBuffer_create (sizeof(sUniformBufferObject), &uboHandle))
    {
        gos::logger::err ("VulkanApp::init() => GPU::uniformBuffer_create\n");
        return false;
    }


    //creo un descriptor pool
    gpu->descrPool_createNew (&descrPoolHandle)
        .setMaxNumDescriptorSet(4)
        .addPool_uniformBuffer()
        .end();
    if (descrPoolHandle.isInvalid())
    {
        gos::logger::err ("VulkanApp::init() => can't create descriptor pool\n");
        return false;
    }

    //alloco una istanza del descriptorSet
    if (!gpu->descrSetInstance_createNew (descrPoolHandle, descrSetLayoutHandle, &descrSetInstancerHandle))
    {
        gos::logger::err ("VulkanApp::init() => can't create descriptorSet instance\n");
        return false;
    }


    return true;
}    


//************************************
bool VulkanExample6::createVertexIndexStageBuffer()
{
    const u32 sizeInByte = sizeof(Vertex) * myShape.numVtx;
    if (!gpu->vertexBuffer_create (sizeInByte, eVIBufferMode::onGPU, &vtxBufferHandle))
    {
        gos::logger::err ("VulkanApp::createVertexIndexStageBuffer() => gpu->vertexBuffer_create() failed\n");
        return false;
    }

    //INDEX BUFFER
    if (!gpu->indexBuffer_create (sizeof(u16)*myShape.numIdx, eVIBufferMode::onGPU, &idxBufferHandle))
    {
        gos::logger::err ("VulkanApp::createVertexIndexStageBuffer() => gpu->indexBuffer_create() failed\n");
        return false;
    }

    //Creo anche uno staging buffer
    if (!gpu->stagingBuffer_create (sizeInByte, &stgBufferHandle))
    {
        gos::logger::err ("VulkanApp::createVertexIndexStageBuffer() => gpu->stagingBuffer_create() failed\n");
        return false;
    }

    return true;
}


//************************************
bool VulkanExample6::recordCommandBuffer (GPUCmdBufferHandle &cmdBufferHandle)
{
    //aggiorno UBO
    ubo.objWorld.identity();
    ubo.camView = cam.getMatV();
    ubo.camProj = cam.getMatP();

    //ubo.lightDir.set (-1, -0.3f, 0, 0);
    ubo.lightDir.set (0, -0.5f, 1, 0);
    ubo.lightDir.normalize();
    gpu->uniformBuffer_mapCopyUnmap (uboHandle, 0, sizeof(sUniformBufferObject), &ubo);

    gos::gpu::DescrSetInstanceWriter descrWriter;
    descrWriter.begin (gpu, descrSetInstancerHandle)
        .updateUniformBuffer (0, uboHandle)
        .end();


    gos::gpu::CmdBufferWriter cw;
    return cw.begin (gpu, cmdBufferHandle)
        .setViewport (gpu->viewport_getDefault())
        .bindPipeline (pipelineHandle)
        .bindDescriptorSet (descrSetInstancerHandle)
        .setClearColor (0, gos::ColorHDR(0, 0.1f, 0.3f))
        .setDepthBufferColor(1, 0)
        .renderPass_begin (renderLayoutHandle, frameBufferHandle)
            .bindVtxBuffer(vtxBufferHandle)
            .bindIdxBufferU16(idxBufferHandle)
            .drawIndexed (myShape.numIdx, 1, 0, 0, 0)
        .renderPass_end()
        .end();
}

/************************************
 * renderizza inviando command buffer a GPU e poi aspettando che questa
 * abbia finito il suo lavoro
 */
void VulkanExample6::virtual_onRun()
{
    cam.setPerspectiveFovLH(gpu->swapChain_calcAspectRatio(),  math::gradToRad(45), 0.1f, 50.0f);
    cam.pos.identity();
    cam.pos.warp (0, 0, -19);
    cam.pos.lookAt (vec3f(0,0,0));
    cam.markUpdated();

    movement.bind (&cam.pos);
    mainLoop();
}

//**********************************
void VulkanExample6::virtual_onInputEvent (u32 actionID, i16 value)
{
    switch (actionID)
    {
    default:
        break;

    case COMPILE_TIME_STR_CRC32("game.move_forward"):           movement.moveForward ((value == 1));break;
    case COMPILE_TIME_STR_CRC32("game.move_backward"):          movement.moveBackward ((value == 1));    break;
    case COMPILE_TIME_STR_CRC32("game.strafe_left"):            movement.strafeLeft ((value == 1));    break;
    case COMPILE_TIME_STR_CRC32("game.strafe_right"):           movement.strafeRight ((value == 1));    break;
    case COMPILE_TIME_STR_CRC32("game.strafe_up"):              movement.strafeUp ((value == 1));    break;
    case COMPILE_TIME_STR_CRC32("game.strafe_down"):            movement.strafeDown ((value == 1));    break;
    case COMPILE_TIME_STR_CRC32("game.rotateY"):                movement.rotateY ((value<0)); break;
    case COMPILE_TIME_STR_CRC32("game.rotateX"):                movement.rotateX ((value<0)); break;
    
    }
}

//**********************************
void VulkanExample6::doCPUStuff ()
{
    fpsMegaTimer.onFrameBegin(FPSTIMER_CPU);

    handleInput();

    //do some stuff
    i32 tot = 0;
    for (u32 i=0; i<1000; i++)
    {
        const f32 r1 = gos::random01() * 100000.0f;
        const f32 r2 = gos::random01() * 100000.0f;
        
        if (sqrtf (r1 * r1) - r2*r2 < 0)
            tot++;
        else
            tot--;
    }
    if (tot < 0)
        printf ("A\n");


    //gestione del movimento
    const u64 timeNow_msec = gos::getTimeSinceStart_msec();
    movement.update(timeNow_msec);
    cam.markUpdated();

    fpsMegaTimer.onFrameEnd(FPSTIMER_CPU);
    fpsMegaTimer.printReport();
}


//**********************************
void VulkanExample6::mainLoop()
{
    GPUMainLoop gpuLoop;
    gpuLoop.setup (gpu, &fpsMegaTimer);

    //command buffer 
    GPUCmdBufferHandle  cmdBufferHandle;
    gpu->cmdBuffer_create (eGPUQueueType::gfx, &cmdBufferHandle);

    //main loop
    while (bQuitApp == false)
    {
        doCPUStuff ();


        gpuLoop.run ();
        if (gpuLoop.swapchainRecreated())
            cam.changeAspectRatioPerspectiveFovLH (gpu->swapChain_calcAspectRatio());
        if (gpuLoop.canSubmitGFXJob())
        {
            recordCommandBuffer (cmdBufferHandle);
            gpuLoop.submitGFXJob (cmdBufferHandle);
        }

    }

    //aspetto che GPU abbia finito tutto cio' che ha in coda
    gpu->waitIdle();

    //free
    gpu->deleteResource (cmdBufferHandle);
    gpuLoop.unsetup();
}

