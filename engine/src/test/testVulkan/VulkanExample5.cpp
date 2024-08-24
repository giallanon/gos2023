#include "VulkanExample5.h"
#include "../gosGeom/gosGeomCamera3.h"


using namespace gos;

/************************************************************************************************************
 * 
 * WORLD
 * 
 *************************************************************************************************************/
VulkanExample5::World::World (gos::GPU *gpuIN)
{
    localAllocator = gos::getSysHeapAllocator();
    gpu = gpuIN;
    map = NULL;
    dimx = dimy = 0;
}

VulkanExample5::World::~World()
{
    priv_freeMap();
    gpu->deleteResource (hVBInstance);
}

void VulkanExample5::World::priv_freeMap()
{
    if (NULL != map)
    {
        GOSFREE(localAllocator, map);
        map = NULL;
        dimx = dimy = 0;
    }
}

void VulkanExample5::World::setup (u32 gridSizeX, u32 gridSizeY)
{
    priv_freeMap();
    dimx = gridSizeX;
    dimy = gridSizeY;
    map = GOSALLOCT(u8*, localAllocator, dimx*dimy);
    memset (map, 0, dimx*dimy);
}
void VulkanExample5::World::set (u32 x, u32 y, bool b)
{
    if (x >= dimx || y >= dimy)
        return;

    bNeedUpdate = true;
    const u32 ct = y*dimy+x;
    if (b)
        map[ct] = 1;
    else
        map[ct] = 0;
}
void VulkanExample5::World::toggle (u32 x, u32 y)
{
    if (x >= dimx || y >= dimy)
        return;

    bNeedUpdate = true;
    const u32 ct = y*dimx+x;
    if (map[ct] == 1)
        map[ct] = 0;
    else
        map[ct] = 1;
}
void VulkanExample5::World::set (const gos::vec3f &p, bool b)
{
    const i16 x = (i16) (floorf(p.x + 0.5f) + (f32)dimx/2);
    const i16 y = (i16) ((f32)dimy/2 - floorf(p.z + 0.5f));
    if (x >=0 && x < (i16)dimx)
        set ((u32)x, (u32)y, b);
}

void VulkanExample5::World::updateInstanceVB (GPUStgBufferHandle hStgBuffer)
{
    if (!bNeedUpdate)
        return;
    bNeedUpdate = false;
    gpu->deleteResource (hVBInstance);

    const u32 numInstances = dimx*dimy;
    //vtx buffer (stream 1)
    if (!gpu->vertexBuffer_create (sizeof(sPerInstanceData) * numInstances, eVIBufferMode::onGPU, &hVBInstance))
    {
        gos::logger::err ("VulkanExample5::World::updateInstanceVB() => gpu->vertexBuffer_create() failed\n");
        return;
    }    

    sPerInstanceData *perInstanceData = GOSALLOCT(sPerInstanceData*, gos::getScrapAllocator(), sizeof(sPerInstanceData) * numInstances);
    u32 ct = 0;
    const f32 startX = -((dimx/2) * SPACE);
    f32 zz = (dimy/2) * SPACE;

    for (u32 y=0; y<dimy; y++)
    {
        f32 xx = startX;
        for (u32 x=0; x<dimx; x++)
        {
            perInstanceData[ct].pos.set (xx, 0, zz);
            if (map[ct] == 0)
                perInstanceData[ct].color.set (1,0,0);
            else
                perInstanceData[ct].color.set (0,1,0);
            ct++;
            xx += SPACE;
        }
        zz -= SPACE;
    }

    if (!gpu->stagingBuffer_uploadToGPUBuffer (hStgBuffer, perInstanceData, hVBInstance, 0, sizeof(sPerInstanceData) * numInstances))
    {
        gos::logger::err ("VulkanExample5::World::updateInstanceVB() => can't upload to VtxBuffer\n");
    }

    GOSFREE(gos::getScrapAllocator(), perInstanceData);
}

void VulkanExample5::World::computeAndDrawPerimeter (Line &line)
{
    line.begin();

    //marching square
    const f32 HALF_SPACE = SPACE * 0.5f;
    const f32 startX = -((dimx/2) * SPACE) + HALF_SPACE;
    f32 zz = (dimy/2) * SPACE - HALF_SPACE;


    for (u32 y=0; y<dimy-1; y++)
    {
        f32 xx = startX;
        for (u32 x=0; x<dimx-1; x++)
        {
            /*
                       a
                0x08---------0x04
                   |         |
                 d |         | b
                   |         |
                0x01---------0x02
                        c
            */
            u8 mask = 0;
            if (get(x,y) != 0)
                mask |= 0x08;
            if (get(x+1,y) != 0)
                mask |= 0x04;
            if (get(x+1,y+1) != 0)
                mask |= 0x02;
            if (get(x,y+1) != 0)
                mask |= 0x01;

            const vec3f a(xx, 0, zz + HALF_SPACE); 
            const vec3f b(xx + HALF_SPACE, 0, zz);
            const vec3f c(xx, 0, zz - HALF_SPACE);
            const vec3f d(xx - HALF_SPACE, 0, zz);

            switch (mask)
            {
            case 0:     break;
            case 1:     line.addLine (c, d); break;
            case 2:     line.addLine (b, c); break;
            case 3:     line.addLine (b, d); break;
            case 4:     line.addLine (a, b); break;
            case 5:     line.addLine (a, d); line.addLine (b, c); break;
            case 6:     line.addLine (a, c); break;
            case 7:     line.addLine (a, d); break;
            case 8:     line.addLine (a, d); break;
            case 9:     line.addLine (a, c); break;
            case 10:    line.addLine (a, b); line.addLine (c, d); break;
            case 11:    line.addLine (a, b); break;
            case 12:    line.addLine (b, d); break;
            case 13:    line.addLine (b, c); break;
            case 14:    line.addLine (c, d); break;
            case 15:    break;

            default:
                //errore, qui non ci dobbiamo mai arrivare
                DBGBREAK;
                break;
            }

            xx += SPACE;
        }
        zz -= SPACE;
    }

    line.end();
}


/************************************************************************************************************
 * 
 * VULKAN 5
 * 
 *************************************************************************************************************/
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
    gos::logger::log ("LMB accende spegne i nodi\n");
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


    //creo una sfera
    {
        gos::shape::VtxMap vtxMap;
        vtxMap.begin()
            .addPos3 (offsetof(Vertex,pos))
            .addNorm3 (offsetof(Vertex,norm))
        .end();

        gos::shape::Writer writer;
        writer.setup (vtxMap, vertexList, sizeof(Vertex), NUM_MAX_VERTEX, indexList, NUM_MAX_INDEX);
        gos::shape::buildSphere (vec3f(0,0,0), vec3f(0.4f,0.4f,0.4f), 16, 6, &writer, &sphereInfo);
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
    line->begin ();
    line->addLine (gos::vec3f(0,0,0), gos::vec3f(10,0,0));
    line->addLine (gos::vec3f(0,0,0), gos::vec3f(-10,0,0));
    line->addLine (gos::vec3f(0,0,0), gos::vec3f(0,0,10));
    line->addLine (gos::vec3f(0,0,0), gos::vec3f(0,0,-10));
    line->end();

    world = new World(gpu);
    world->setup (21, 21);
    
    u32 r=1;
    world->set (10,r);
    r++; world->set (10,r); world->set (9,r); world->set (11,r);
    r++; world->set (10,r); world->set (9,r); world->set (11,r); world->set (8,r); world->set (12,r);
    r++; world->set (10,r); world->set (9,r); world->set (11,r); world->set (8,r); world->set (12,r); world->set (7,r); world->set (13,r);
    r++; world->set (10,r); world->set (9,r); world->set (11,r); world->set (8,r); world->set (12,r);
    r++; world->set (10,r); world->set (9,r); world->set (11,r);
    r++; world->set (10,r);
    world->updateInstanceVB (stgBufferHandle);



    
    return true;
}

//************************************
bool VulkanExample5::priv_recordCommandBuffer (gpu::CmdBufferWriter &cw)
{
    world->updateInstanceVB (stgBufferHandle);

    //upload di UBO su GPU
    ubo.camView = cam.getMatV();
    ubo.camProj = cam.getMatP();
    ubo.lightDir.set (-0.4f, -1, 0.2f, 0);
    //ubo.lightDir.set (-0, -1, 0, 0);
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
void VulkanExample5::priv_setSphere (const gos::vec2f &mouseXY, bool b)
{
    gos::vec3f dir;
    cam.unproject (gpu->swapChain_getWidth(), gpu->swapChain_getHeight(), &mouseXY, &dir, 1);

    //cam.o.z + dir.z * t = 0
    const f32 t = -cam.pos.o.y / dir.y;
    gos::vec3f pp = cam.pos.o + dir*t;
    //printf ("POINT (%d,%d) to 3d: %.2f %.2f %.2f\n", inputMap.resolve_getMouseX(), inputMap.resolve_getMouseY(), pp.x, pp.y, pp.z);
    world->set (pp, b);
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
            priv_setSphere (gos::vec2f(inputMap.resolve_getMouse().x, inputMap.resolve_getMouse().y), true);
        else if (inputMap.resolve_getMouse().isRMBPressed())
            priv_setSphere (gos::vec2f(inputMap.resolve_getMouse().x, inputMap.resolve_getMouse().y), false);
        break;


    case COMPILE_TIME_STR_CRC32("game.run.marching.square"):
        world->computeAndDrawPerimeter(*line);
        break;
    }
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


