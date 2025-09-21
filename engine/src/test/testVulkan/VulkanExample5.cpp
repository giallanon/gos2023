#include "VulkanExample5.h"
#include "../gosGeom/gosGeomCamera3.h"
#include "../gosShape/gosShapePrefabs.h"

using namespace gos;

//************************************************************************************************************
VulkanExample5::VulkanExample5()
{
    world = NULL;
    line = NULL;

    gpuMSQ2.numIdx = gpuMSQ2.numVtx = 0;
    gpuMSQ2.idxBufferHandle.setInvalid();
    gpuMSQ2.vtxBufferHandle.setInvalid();
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
    shape::shapeFree (gos::getSysHeapAllocator(), &myShape);
    delete world;
    delete line;
    gpu->deleteResource (idxBufferHandle);
    gpu->deleteResource (stgBufferHandle);
    gpu->deleteResource (vtxBufferHandle);

    gpu->deleteResource (vtxShaderHandle);
    gpu->deleteResource (fragShaderHandle);
    gpu->deleteResource (pipelineHandle);

    gpu->deleteResource (uboHandle);
    gpu->deleteResource (descrSetInstancerHandle);
    gpu->deleteResource (descrPoolHandle);
    priv_freeMSQ2();
}    

//************************************
void VulkanExample5::priv_freeMSQ2()
{
    if (gpuMSQ2.idxBufferHandle.isValid())
    {
        gpu->deleteResource (gpuMSQ2.idxBufferHandle);
        gpuMSQ2.idxBufferHandle.setInvalid();
    }

    if (gpuMSQ2.vtxBufferHandle.isValid())
    {
        gpu->deleteResource (gpuMSQ2.vtxBufferHandle);
        gpuMSQ2.vtxBufferHandle.setInvalid();
    }

    gpuMSQ2.numIdx = 0;
    gpuMSQ2.numVtx = 0;
}

//************************************
void VulkanExample5::priv_createSfera()
{
    myShape.reset();

    gos::VtxLayout vtxLayout;
    gos::shape::VtxLayoutWriter vtxLayoutW(&vtxLayout);
    vtxLayoutW.begin()
        .addPos3 (offsetof(Vertex,pos))
        .addNorm3 (offsetof(Vertex,norm))
    .end();

    const f32 radius = 1.0f;
    gos::shape::buildSphere (vec3f(0,0,0), vec3f(radius, radius, radius), 16, 6, vtxLayout, gos::getSysHeapAllocator(), &myShape);
    //gos::shape::buildCylinder (vec3f(0,0,0), 0.8f, 6, 15, 3, true, true, gos::getSysHeapAllocator(), &myShape);
    //gos::shape::buildCube24 (vec3f(0,0,0), vec3f(1,1,1), gos::getSysHeapAllocator(), &myShape);

    shape::shapeSave ("@w/vulkanExample5_shape.gosshape", &myShape);
}   

//************************************
bool VulkanExample5::priv_loadSfera()
{
    return gos::shape::shapeLoad ("@w/vulkanExample5_shape.gosshape", gos::getSysHeapAllocator(), &myShape);
}

//************************************
bool VulkanExample5::virtual_onInit ()
{
    //input binding
    inputCtx.action_add ("LMB");
    inputCtx.action_bindToBtn ("LMB", input::eOrigin::mouse, GOS_BUTTON_MOUSE_LEFT, input::eButtonStatus::pressed);

    inputCtx.action_add ("RMB");
    inputCtx.action_bindToBtn ("RMB", input::eOrigin::mouse, GOS_BUTTON_MOUSE_RIGHT, input::eButtonStatus::pressed);

    inputCtx.action_add ("run.marching.square");
    inputCtx.action_bindToBtn ("run.marching.square", input::eOrigin::keyboard, GLFW_KEY_ENTER, input::eButtonStatus::pressed);

    inputCtx.action_add ("mouse_move");
    inputCtx.action_bindToAxleABS ("mouse_move",  input::eOrigin::mouse, input::eAxle::y);
    inputCtx.action_bindToAxleABS ("mouse_move",  input::eOrigin::mouse, input::eAxle::x);

    inputCtx.action_add ("inc_dec_ScaleXZ");
    inputCtx.action_bindToAxleREL ("inc_dec_ScaleXZ",  input::eOrigin::mouse, input::eAxle::z, input::eAxleDirection::both);


    //creo una sfera
    priv_createSfera();
    /*if (!priv_loadSfera())
    {
        gos::logger::err ("VulkanApp::virtual_onInit() => error loading shape\n");
        return false;
    }*/
 

    //vtx buffer (stream 0)
    if (!gpu->vertexBuffer_create (sizeof(Vertex) * myShape.numVtx, eVIBufferMode::onGPU, &vtxBufferHandle))
    {
        gos::logger::err ("VulkanApp::createVertexIndexStageBuffer() => gpu->vertexBuffer_create() failed\n");
        return false;
    }

    //index buffer
    if (!gpu->indexBuffer_create (sizeof(u16)*myShape.numIdx, eVIBufferMode::onGPU, &idxBufferHandle))
    {
        gos::logger::err ("VulkanApp::createVertexIndexStageBuffer() => gpu->indexBuffer_create() failed\n");
        return false;
    }

    //Creo anche uno staging buffer
    if (!gpu->stagingBuffer_create (1024*64, &stgBufferHandle))
    {
        gos::logger::err ("VulkanApp::createVertexIndexStageBuffer() => gpu->stagingBuffer_create() failed\n");
        return false;
    }    

    //copio i Vtx in vtxBuffer e idx in idxBuffer tramite lo staging buffer
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



    //pipeline
    gpu::pipe2::Pipeline_def def;
    def
        .reset()
        .shader_add (vtxShaderHandle)
        .shader_add (fragShaderHandle)
        .add_rt (eImageFormat::_SAME_AS_CURRENT_SWAPCHAIN)
        .set_zbuffer(eImageFormat::_DEPTH_BEST)
        .vtxStream_add (eVtxStreamInputRate::perVertex)
            .add (0, offsetof(Vertex, pos), eDataFormat::_3f32)       //position
            .add (1, offsetof(Vertex, norm), eDataFormat::_3f32)      //normal
            .endVtxStream()
        .vtxStream_add (eVtxStreamInputRate::perInstance)
            .add (2, offsetof(sPerInstanceData, pos), eDataFormat::_3f32)       //position
            .add (3, offsetof(sPerInstanceData, color), eDataFormat::_3f32)
            .add (4, offsetof(sPerInstanceData, scale), eDataFormat::_3f32)    
            .endVtxStream()
        .descriptorset_add()
            .add (0, eGPUDescriptrorType::UNIFORM_BUFFER, 1, eGPUDescriptrorUsageFlag::vtx_shader)
            .endDescriptorSet();

    if (!gpu->pipeline_createNew (def, &pipelineHandle))
    {
        gos::logger::err ("VulkanApp::init() => can't create pipeline\n");
        return false;
    };    


   
    //creo un buffer per UBO
    if (!gpu->uniformBuffer_create (sizeof(sUniformBufferObject), eVIBufferMode::shared_cpuW_autoSync, &uboHandle))
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
    if (!gpu->pipeline_createDescrSetInstance (pipelineHandle, 0, descrPoolHandle, &descrSetInstancerHandle))
    {
        gos::logger::err ("VulkanApp::init() => can't create descriptorSet instance\n");
        return false;
    }


    line = new SimpleLineRenderer();
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
    line->setColor (gos::vec3f(0.1f, 0.1f, 0.1f));

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
void VulkanExample5::virtual_onInputEvent (u32 actionID, i16 value, const gos::input::MouseStatus &mouseStatus, const gos::input::sButtonModifier &btnModifier)
{
    switch (actionID)
    {
    default:
        break;

    case COMPILE_TIME_STR_CRC32("move_forward"):           movement.moveForward ((value == 1));break;
    case COMPILE_TIME_STR_CRC32("move_backward"):          movement.moveBackward ((value == 1));    break;
    case COMPILE_TIME_STR_CRC32("strafe_left"):            movement.strafeLeft ((value == 1));    break;
    case COMPILE_TIME_STR_CRC32("strafe_right"):           movement.strafeRight ((value == 1));    break;
    case COMPILE_TIME_STR_CRC32("strafe_up"):              movement.strafeUp ((value == 1));    break;
    case COMPILE_TIME_STR_CRC32("strafe_down"):            movement.strafeDown ((value == 1));    break;
    case COMPILE_TIME_STR_CRC32("rotateY"):                movement.rotateY ((value<0)); break;
    case COMPILE_TIME_STR_CRC32("rotateX"):                movement.rotateX ((value<0)); break;

    case COMPILE_TIME_STR_CRC32("mouse_move"):
    case COMPILE_TIME_STR_CRC32("LMB"):
    case COMPILE_TIME_STR_CRC32("RMB"):
        if (mouseStatus.isLMBPressed())
            priv_setSphere_ON_OFF (mouseStatus.x, mouseStatus.y, true);
        else if (mouseStatus.isRMBPressed())
            priv_setSphere_ON_OFF (mouseStatus.x, mouseStatus.y, false);
        break;


    case COMPILE_TIME_STR_CRC32("run.marching.square"):
        priv_runMarchingSquare();
        break;

    case COMPILE_TIME_STR_CRC32("inc_dec_ScaleXZ"):
        {
            u16 x, y;
            if (world->mouseToGrid (gpu, cam, mouseStatus.x, mouseStatus.y, &x, &y))
            {
                if (btnModifier.isSHIFT())
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
    priv_drawGrid();

    //MarchingSquare msq;
    //VkEx5MarchingSquare msq;    msq.algo2 (*world, *line);

    const f32 world_x1 = -((world->getDimX()/2) * World::SPACE);
    const f32 world_z1 = (world->getDimY()/2) * World::SPACE;


    MarchingSquare msq2;
    msq2.run (gos::getSysHeapAllocator(), *world);
    for (u32 i=0; i<msq2.getNumPerimetri(); i++)
    {
        const MarchingSquare::sInfo *info = msq2.getPerimetroByIndex (i);

        //disegno i segmenti
        for (u32 i2=0; i2<info->numVtx-1; i2++)
        {
            const gos::vec2f v1 = info->vtxList[i2].pos;
            const gos::vec2f v2 = info->vtxList[i2+1].pos;

            vec3f vtx1 (world_x1 + v1.x, 0, world_z1 + v1.y);
            vec3f vtx2 (world_x1 + v2.x, 0, world_z1 + v2.y);
            
            line->setColor (vec3f(1,1,1));
            const u16 idx1 = line->addVtx (vtx1);

            line->setColor (vec3f(1,0,0));
            const u16 idx2 = line->addVtx (vtx2);

            line->line (idx1, idx2);
        }

        //disegno le normali
        line->setColor (vec3f(0,1,0));
        for (u32 i2=0; i2<info->numVtx; i2++)
        {
            const gos::vec2f p1 = info->vtxList[i2].pos;
            const gos::vec2f p2 = info->vtxList[i2].pos + info->vtxList[i2].norm * 0.2f;

            const vec3f vtx1 (world_x1 + p1.x, 0, world_z1 + p1.y);
            const vec3f vtx2 (world_x1 + p2.x, 0, world_z1 + p2.y);
            line->addLine (vtx1, vtx2);

        }        
    }
    line->end();

    //estraggo le mesh
    {
        MarchingSquare::VertexList3 vtxList (gos::getScrapAllocator(), 1024);
        gos::FastArray<u16> idxList (gos::getScrapAllocator(), 1024);
        msq2.buildMesh (1.5f, vtxList, idxList);

        priv_freeMSQ2();

        gpuMSQ2.numVtx = vtxList.getNElem();
        gpuMSQ2.numIdx = idxList.getNElem();
        if (gpuMSQ2.numIdx > 0)
        {
            const gos::vec3f origin (world_x1, 0, world_z1);
            Vertex *vtx = GOSALLOCT(Vertex*, gos::getScrapAllocator(), sizeof(Vertex) * gpuMSQ2.numVtx);
            for (u32 i=0; i<gpuMSQ2.numVtx; i++)
            {
                vtx[i].pos = origin + vtxList(i).pos;
                vtx[i].norm = vtxList(i).norm;
            }

            gpu->vertexBuffer_create (sizeof(Vertex) * gpuMSQ2.numVtx, eVIBufferMode::onGPU, &gpuMSQ2.vtxBufferHandle);
            gpu->stagingBuffer_uploadToGPUBuffer (stgBufferHandle, vtx, gpuMSQ2.vtxBufferHandle, 0, sizeof(Vertex) * gpuMSQ2.numVtx);
            GOSFREE(gos::getScrapAllocator(), vtx);

            gpu->indexBuffer_create (sizeof(u16) * gpuMSQ2.numIdx, eVIBufferMode::onGPU, &gpuMSQ2.idxBufferHandle);
            gpu->stagingBuffer_uploadToGPUBuffer (stgBufferHandle, idxList._queryPointer() , gpuMSQ2.idxBufferHandle, 0, sizeof(u16) * gpuMSQ2.numIdx);
        }
        vtxList.unsetup();
        idxList.unsetup();

    }

    
}


//**********************************
void VulkanExample5::priv_doCPUStuff ()
{
    handleInput();

    //gestione del movimento
    const u64 timeNow_msec = gos::getTimeSinceStart_msec();
    movement.update(timeNow_msec);
    cam.markUpdated();
}

//************************************
bool VulkanExample5::recordCommandBuffer (GPUCmdBufferHandle &cmdBufferHandle, gos::gpu::SwapchainImg &swapChainImage)
{
    world->updateInstanceVB (stgBufferHandle);

    //upload di UBO su GPU
    ubo.camView = cam.getMatV();
    ubo.camProj = cam.getMatP();
    //ubo.lightDir.set (-0.4f, -1, 0.2f, 0);
    ubo.lightDir.set (0, -1, 0, 0);
    ubo.lightDir.normalize();
    gpu->writeAndSync (uboHandle, 0, &ubo, sizeof(sUniformBufferObject));            

    gos::gpu::DescrSetInstanceWriter descrWriter;
    descrWriter.begin (gpu, descrSetInstancerHandle)
        .bindUniformBuffer (0, uboHandle)
        .end();


    GPUDepthStencilHandle hZB = gpu->depthStencil_getDefault();
    gos::gpu::pipe2::CmdBufferWriter2 cw;
    cw
        .begin (gpu, cmdBufferHandle)
        .setViewport (gpu->viewport_getDefault())
        .imageTransition (swapChainImage.image, eImageLayout::undefined, eImageLayout::color_attachment_optimal)
        .imageTransition (hZB, eImageLayout::undefined, eImageLayout::depth_attachment_optimal);
    

    auto &rend = cw.beginRender();
    rend
        .withRenderArea (gpu->swapChain_getWidth(), gpu->swapChain_getHeight())
        .withRT (swapChainImage.imageView, eAttachmentLoadOp::clear, eAttachmentStoreOp::dont_care, gos::ColorHDR(0, 0, 0))
        .withZB (hZB, eAttachmentLoadOp::clear, eAttachmentStoreOp::dont_care, 1.0f, 0)
        .bindPipeline (pipelineHandle)
        .bindDescriptorSet (descrSetInstancerHandle, 0)
        .bindVtxBuffers(vtxBufferHandle, world->hVBInstance)
        .bindIdxBufferU16(idxBufferHandle)
        .drawIndexed (myShape.numIdx, world->getNumInstances(), 0, 0, 0);

        if (gpuMSQ2.numIdx)
        {
            rend
                .bindIdxBufferU16(gpuMSQ2.idxBufferHandle)
                .bindVtxBuffer(gpuMSQ2.vtxBufferHandle)
                .drawIndexed (gpuMSQ2.numIdx, 1, 0, 0, 0);
        }
    rend.endRender();

    //line->recordCommandBuffer (rend, stgBufferHandle, cam);
    line->recordCommandBuffer (cw, swapChainImage.imageView, stgBufferHandle, cam);
    

//    rend.endRender();
    cw
        .imageTransition (swapChainImage.image, eImageLayout::color_attachment_optimal, eImageLayout::presentation)
        .end();

    return true;
}

//**********************************
void VulkanExample5::virtual_onRun()
{
    //camera
    cam.setPerspectiveFovLH (gpu->swapChain_calcAspectRatio(),  math::gradToRad(45), 0.1f, 50.0f);
    //cam.pos.identity(); cam.pos.warp (0, 0, -19); cam.pos.lookAt (vec3f(0,0,0));
    cam.pos.identity(); cam.pos.warp (0, 30, 0); cam.pos.rotateMeAboutMyX (math::gradToRad(-90));
    
    cam.markUpdated();

    movement.bind (&cam.pos);
    
    
    
    gpu::MainLoop2 mainLoop;
    mainLoop.setup (gpu);

    //command buffer 
    GPUCmdBufferHandle  cmdBufferHandle;
    gpu->cmdBuffer_create (eGPUQueueType::gfx, &cmdBufferHandle);


    //main loop
    while (bQuitApp == false)
    {
        mainLoop.stat_onCPUFrameBegin();
        priv_doCPUStuff();
        mainLoop.stat_onCPUFrameEnd();


        mainLoop.run();

        if (gpu->swapChain_wasRecreated())
            cam.changeAspectRatioPerspectiveFovLH (gpu->swapChain_calcAspectRatio());

        //se il job precedente e' stato presentato, posso schedularne uno nuovo
        gpu::SwapchainImg swapchainImg;
        if (mainLoop.gfxJob_canSubmit(&swapchainImg))
        {
            recordCommandBuffer (cmdBufferHandle, swapchainImg);
            mainLoop.gfxJob_submitAndPresent (cmdBufferHandle, swapchainImg);
        }
    }

    //aspetto che GPU abbia finito tutto cio' che ha in coda
    gpu->waitIdle();

    //free
    gpu->deleteResource (cmdBufferHandle);
}


