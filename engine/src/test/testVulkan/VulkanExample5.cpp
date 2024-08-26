#include "VulkanExample5.h"
#include "../gosGeom/gosGeomCamera3.h"


using namespace gos;

//************************************************************************************************************
VulkanExample5::VulkanExample5()
{
    world = NULL;
    line = NULL;
}

//************************************
void VulkanExample5::virtual_explain()
{
    gos::logger::log ("Marching cube\n");
    gos::logger::incIndent();
    gos::logger::log ("LMB accende i nodi\n");
    gos::logger::log ("RMB spegne i nodi\n");
    gos::logger::log ("ENTER crea il contorno\n");
    gos::logger::decIndent();
}


//************************************
void VulkanExample5::virtual_onCleanup() 
{
    delete world;
    delete line;
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
bool VulkanExample5::virtual_onInit ()
{
    //input binding
    inputMap.action_add ("game", "LMB");
    inputMap.action_bindToBtn ("game", "LMB", input::eOrigin::mouse, GOS_BUTTON_MOUSE_LEFT, input::eButtonStatus::pressed);

    inputMap.action_add ("game", "RMB");
    inputMap.action_bindToBtn ("game", "RMB", input::eOrigin::mouse, GOS_BUTTON_MOUSE_RIGHT, input::eButtonStatus::pressed);

    inputMap.action_add ("game", "run.marching.square");
    inputMap.action_bindToBtn ("game", "run.marching.square", input::eOrigin::keyboard, GLFW_KEY_ENTER, input::eButtonStatus::pressed);

    inputMap.action_add ("game","mouse_move");
    inputMap.action_bindToAxleABS ("game", "mouse_move",  input::eOrigin::mouse, input::eAxle::y);
    inputMap.action_bindToAxleABS ("game", "mouse_move",  input::eOrigin::mouse, input::eAxle::x);

    inputMap.action_add ("game","inc_dec_ScaleXZ");
    inputMap.action_bindToAxleREL ("game", "inc_dec_ScaleXZ",  input::eOrigin::mouse, input::eAxle::z, input::eAxleDirection::both);


    //creo una sfera
    {
        gos::shape::VtxMap vtxMap;
        vtxMap.begin()
            .addPos3 (offsetof(Vertex,pos))
            .addNorm3 (offsetof(Vertex,norm))
        .end();

        gos::shape::Writer writer;
        writer.setup (vtxMap, vertexList, sizeof(Vertex), NUM_MAX_VERTEX, indexList, NUM_MAX_INDEX);
        const f32 radius = 1.0f;
        gos::shape::buildSphere (vec3f(0,0,0), vec3f(radius, radius, radius), 16, 6, &writer, &sphereInfo);
        //gos::shape::buildCylinder (vec3f(0,0,0), 0.8f, 6, 15, 3, true, true, &writer, &sphereInfo);
        //gos::shape::buildCube24 (vec3f(0,0,0), vec3f(1,1,1), &writer, &info);
    }    

    //vtx buffer (stream 0)
    if (!gpu->vertexBuffer_create (sizeof(Vertex) * sphereInfo.numVertex, eVIBufferMode::onGPU, &vtxBufferHandle))
    {
        gos::logger::err ("VulkanApp::createVertexIndexStageBuffer() => gpu->vertexBuffer_create() failed\n");
        return false;
    }

    //index buffer
    if (!gpu->indexBuffer_create (sizeof(u16)*sphereInfo.numIndex, eVIBufferMode::onGPU, &idxBufferHandle))
    {
        gos::logger::err ("VulkanApp::createVertexIndexStageBuffer() => gpu->indexBuffer_create() failed\n");
        return false;
    }

    //Creo anche uno staging buffer
    if (!gpu->stagingBuffer_create (1024*16, &stgBufferHandle))
    {
        gos::logger::err ("VulkanApp::createVertexIndexStageBuffer() => gpu->stagingBuffer_create() failed\n");
        return false;
    }    

    //copio i Vtx in vtxBuffer e idx in idxBuffer tramite lo staging buffer
    {
        if (!gpu->stagingBuffer_uploadToGPUBuffer (stgBufferHandle, vertexList, vtxBufferHandle, 0, sizeof(Vertex) * sphereInfo.numVertex))
        {
            gos::logger::err ("VulkanApp::init() => can't upload to VtxBuffer\n");
            return false;
        }


        if (!gpu->stagingBuffer_uploadToGPUBuffer (stgBufferHandle, indexList, idxBufferHandle, 0, sizeof(u16) * sphereInfo.numIndex))
        {
            gos::logger::err ("VulkanApp::init() => can't upload to IdxBuffer\n");
            return false;
        }
    }

    //Vtx declaration
    GPUVtxDeclHandle vtxDeclHandle;
    gpu->vtxDecl_createNew (&vtxDeclHandle)
        .addStream(eVtxStreamInputRate::perVertex)
            .addLayout (0, offsetof(Vertex, pos), eDataFormat::_3f32)       //position
            .addLayout (1, offsetof(Vertex, norm), eDataFormat::_3f32)      //normal
        .addStream (eVtxStreamInputRate::perInstance)
            .addLayout (2, offsetof(sPerInstanceData, pos), eDataFormat::_3f32)       //position
            .addLayout (3, offsetof(sPerInstanceData, color), eDataFormat::_3f32)
            .addLayout (4, offsetof(sPerInstanceData, scale), eDataFormat::_3f32)
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
    fs::addAlias ("@shader", "shader/example5", eAliasPathMode::relativeToAppFolder);
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
    gpu->descrSetLayout_createNew (&descrSetLayoutHandle)
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
            .stencil_enable(false)
        .end() //depth stencil
        .setCullMode (eCullMode::CCW)
        .setDrawPrimitive (eDrawPrimitive::trisList)
        .descriptor_add (descrSetLayoutHandle)
        //.setWireframe(true)
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


    line = new Line();
    line->setup (gpu, descrPoolHandle);

    world = new World(gpu);
    world->setup (21, 21);
    
    u32 r=1;
    world->set_ON_OFF (10,r);
    r++; world->set_ON_OFF (10,r); world->set_ON_OFF (9,r); world->set_ON_OFF (11,r);
    r++; world->set_ON_OFF (10,r); world->set_ON_OFF (9,r); world->set_ON_OFF (11,r); world->set_ON_OFF (8,r); world->set_ON_OFF (12,r);
    r++; world->set_ON_OFF (10,r); world->set_ON_OFF (9,r); world->set_ON_OFF (11,r); world->set_ON_OFF (8,r); world->set_ON_OFF (12,r); world->set_ON_OFF (7,r); world->set_ON_OFF (13,r);
    r++; world->set_ON_OFF (10,r); world->set_ON_OFF (9,r); world->set_ON_OFF (11,r); world->set_ON_OFF (8,r); world->set_ON_OFF (12,r);
    r++; world->set_ON_OFF (10,r); world->set_ON_OFF (9,r); world->set_ON_OFF (11,r);
    r++; world->set_ON_OFF (10,r);
    world->updateInstanceVB (stgBufferHandle);

    priv_runMarchingSquare();

    
    return true;
}

//************************************
bool VulkanExample5::priv_recordCommandBuffer (gpu::CmdBufferWriter &cw)
{
    world->updateInstanceVB (stgBufferHandle);

    //upload di UBO su GPU
    ubo.camView = cam.getMatV();
    ubo.camProj = cam.getMatP();
    //ubo.lightDir.set (-0.4f, -1, 0.2f, 0);
    ubo.lightDir.set (0, -1, 0, 0);
    ubo.lightDir.normalize();
    gpu->uniformBuffer_mapCopyUnmap (uboHandle, 0, sizeof(sUniformBufferObject), &ubo);            

    gos::gpu::DescrSetInstanceWriter descrWriter;
    descrWriter.begin (gpu, descrSetInstancerHandle)
        .updateUniformBuffer (0, uboHandle)
        .end();


    cw.setViewport (gpu->viewport_getDefault())
        .bindPipeline (pipelineHandle)
        .bindDescriptorSet (descrSetInstancerHandle)
        .setClearColor (0, gos::ColorHDR(0, 0.1f, 0.3f))
        .setDepthBufferColor(1, 0)
        .renderPass_begin (renderLayoutHandle, frameBufferHandle)
            .bindVtxBuffers(vtxBufferHandle, world->hVBInstance)
            .bindIdxBufferU16(idxBufferHandle)
            //.drawIndexed (NUM_INDEX, 1, 0, 0, 0)
            .drawIndexed (sphereInfo.numIndex, world->getNumInstances(), 0, 0, 0)
        .renderPass_end();
    return true;
}

//**********************************
void VulkanExample5::priv_setSphere_ON_OFF (i16 mouseX, i16 mouseY, bool b)
{
    u16 x, y;
    if (world->mouseToGrid (gpu, cam, mouseX, mouseY, &x, &y))
        world->set_ON_OFF (x, y, b);
}

//**********************************
void VulkanExample5::priv_drawGrid ()
{
    line->setColor (gos::vec3f(0.3f, 0.3f, 0.3f));

    const f32 x1 = -((world->getDimX()/2) * World::SPACE);
    const f32 x2 = x1 + world->getDimX() * World::SPACE;
    const f32 z1 = (world->getDimY()/2) * World::SPACE;
    const f32 z2 = z1 - world->getDimY() * World::SPACE;

    f32 zz = z1;
    for (u32 z=0; z<world->getDimY(); z++)
    {
        line->addLine (gos::vec3f(x1,0,zz), gos::vec3f(x2,0,zz));
        zz -= World::SPACE;
    }

    f32 xx = x1;
    for (u32 x=0; x<world->getDimX(); x++)
    {
        line->addLine (gos::vec3f(xx,0,z1), gos::vec3f(xx,0,z2));
        xx += World::SPACE;
    }

}

//**********************************
void VulkanExample5::virtual_onInputEvent (u32 actionID, i16 value)
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

    case COMPILE_TIME_STR_CRC32("game.mouse_move"):
    case COMPILE_TIME_STR_CRC32("game.LMB"):
    case COMPILE_TIME_STR_CRC32("game.RMB"):
        if (inputMap.resolve_getMouse().isLMBPressed())
            priv_setSphere_ON_OFF (inputMap.resolve_getMouse().x, inputMap.resolve_getMouse().y, true);
        else if (inputMap.resolve_getMouse().isRMBPressed())
            priv_setSphere_ON_OFF (inputMap.resolve_getMouse().x, inputMap.resolve_getMouse().y, false);
        break;


    case COMPILE_TIME_STR_CRC32("game.run.marching.square"):
        priv_runMarchingSquare();
        break;

    case COMPILE_TIME_STR_CRC32("game.inc_dec_ScaleXZ"):
        {
            u16 x, y;
            if (world->mouseToGrid (gpu, cam, inputMap.resolve_getMouse().x, inputMap.resolve_getMouse().y, &x, &y))
            {
                if (inputMap.resolve_getBtnModifier().isSHIFT())
                {
                    if (value > 0)
                        world->inc_scaleZ (x,y);
                    else
                        world->dec_scaleZ (x,y);
                }
                else
                {
                    if (value > 0)
                        world->inc_scaleX (x,y);
                    else
                        world->dec_scaleX (x,y);
                }                
            }
        }
        break;
    }
}

//************************************
void VulkanExample5::priv_runMarchingSquare()
{
    line->begin();

    MarchingSquare msq;
    //msq.algo1 (*world, *line);
    //msq.algo2 (*world, *line);
    msq.algo3 (*world, *line);

    priv_drawGrid();
    line->end();
}

//************************************
void VulkanExample5::virtual_onRun()
{
    fpsMegaTimer.setPrintReportEvery (10000);

    //camera
    cam.setPerspectiveFovLH (gpu->swapChain_calcAspectRatio(),  math::gradToRad(45), 0.1f, 50.0f);
    //cam.pos.identity(); cam.pos.warp (0, 0, -19); cam.pos.lookAt (vec3f(0,0,0));
    cam.pos.identity(); cam.pos.warp (0, 30, 0); cam.pos.rotateMeAboutMyX (-math::PIMEZZI);
    
    cam.markUpdated();

    movement.bind (&cam.pos);
    priv_mainLoop();
}

//**********************************
void VulkanExample5::priv_doCPUStuff ()
{
    fpsMegaTimer.onFrameBegin(FPSTIMER_CPU);

    handleInput();

    //gestione del movimento
    const u64 timeNow_msec = gos::getTimeSinceStart_msec();
    movement.update(timeNow_msec);
    cam.markUpdated();

    fpsMegaTimer.onFrameEnd(FPSTIMER_CPU);
    fpsMegaTimer.printReport();
}


//**********************************
void VulkanExample5::priv_mainLoop()
{
    GPUMainLoop gpuLoop;
    gpuLoop.setup (gpu, &fpsMegaTimer);

    //command buffer 
    GPUCmdBufferHandle  cmdBufferHandle;
    gpu->cmdBuffer_create (eGPUQueueType::gfx, &cmdBufferHandle);

    gos::gpu::CmdBufferWriter cw;

    //main loop
    while (bQuitApp == false)
    {
        priv_doCPUStuff ();


        gpuLoop.run ();
        if (gpuLoop.swapchainRecreated())
            cam.changeAspectRatioPerspectiveFovLH (gpu->swapChain_calcAspectRatio());
        if (gpuLoop.canSubmitGFXJob())
        {
            cw.begin (gpu, cmdBufferHandle);
                priv_recordCommandBuffer (cw);
                line->recordCommandBuffer (cw, stgBufferHandle, cam);
            cw.end();
            gpuLoop.submitGFXJob (cmdBufferHandle);
        }

    }

    //aspetto che GPU abbia finito tutto cio' che ha in coda
    gpu->waitIdle();

    //free
    gpu->deleteResource (cmdBufferHandle);
    gpuLoop.unsetup();
}


