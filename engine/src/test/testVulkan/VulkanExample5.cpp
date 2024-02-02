#include "VulkanExample5.h"
#include "../gosGeom/gosGeomCamera3.h"


using namespace gos;


//************************************
VulkanExample5::VulkanExample5()
{
    localAllocator = gos::getSysHeapAllocator();
    vtxList.setup (localAllocator, 256);
    quadList.setup (localAllocator, 256);
}

//************************************
void VulkanExample5::virtual_explain()
{
    gos::logger::log ("Esperimenti con griglia di vtx da morfare\n");
}


//************************************
void VulkanExample5::virtual_onCleanup() 
{
    gpu->deleteResource (idxBufferHandle);
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
void VulkanExample5::priv_buildGrid()
{
    vtxList.reset();
    quadList.reset();
    const f32 startX = -GRID_HALF_SIZE * GRID_SCALA;
    const f32 incrX = GRID_SCALA;
    const f32 startY =  GRID_HALF_SIZE * GRID_SCALA;
    const f32 incrY = GRID_SCALA;

    f32 yy = startY;
    for (u16 y=0; y<GRID_NUM_VTX_PER_ROW; y++)
    {
        f32 xx = startX;
        for (u16 x=0; x<GRID_NUM_VTX_PER_ROW; x++)
        {
            Vertex v;
            v.set (xx, yy);
            vtxList.append(v);

            xx+=incrX;
        }
        yy+=incrY;
    }

    //quad
    for (u16 y = 0; y < (GRID_NUM_VTX_PER_ROW - 1); y++)
    {
        u16 idxTopLeft = y * GRID_NUM_VTX_PER_ROW;

        for (u16 x = 0; x < (GRID_NUM_VTX_PER_ROW-1) ; x++)
        {
            Quad q;
            q.idx[0] = idxTopLeft;
            q.idx[1] = idxTopLeft + 1;
            q.idx[2] = idxTopLeft + 1 + GRID_NUM_VTX_PER_ROW;
            q.idx[3] = idxTopLeft + GRID_NUM_VTX_PER_ROW;
            quadList.append (q);

            idxTopLeft++;
        }
    }

}

//************************************
bool VulkanExample5::virtual_onInit ()
{
    priv_buildGrid();


    //Vtx declaration
    GPUVtxDeclHandle vtxDeclHandle;
    gpu->vtxDecl_createNew (&vtxDeclHandle)
        .addStream(eVtxStreamInputRate::perVertex)
        .addLayout (0, offsetof(Vertex, pos), eDataFormat::_3f32)        //position
        .end();
    if (vtxDeclHandle.isInvalid())
    {
        gos::logger::err ("VulkanApp::init() => can't create vtxDeclHandle\n");
        return false;
    }

    //vtx buffer
    if (!gpu->vertexBuffer_create (sizeof(Vertex)*MAX_VTX_IN_VTX_BUFFER, eVIBufferMode::mappale, &vtxBufferHandle))
    {
        gos::logger::err ("VulkanApp::createVertexIndexStageBuffer() => gpu->vertexBuffer_create() failed\n");
        return false;
    }

    //idx buffer
    if (!gpu->indexBuffer_create (sizeof(u16)*MAX_IDX_IN_IDX_BUFFER, eVIBufferMode::mappale, &idxBufferHandle))
    {
        gos::logger::err ("VulkanApp::createVertexIndexStageBuffer() => gpu->indexBuffer_create() failed\n");
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

    //Creo il descriptorSet layout con un solo UNIFORM BUFFER per il VTX SHADER
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
            .stencil_enable(false)
        .end() //depth stencil
        .setCullMode (eCullMode::NONE)
        .setDrawPrimitive (eDrawPrimitive::lineList)
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
bool VulkanExample5::recordCommandBuffer (GPUCmdBufferHandle &cmdBufferHandle)
{
    //aggiorno UBO
    gos::gpu::DescrSetInstanceWriter descrWriter;
    descrWriter.begin (gpu, descrSetInstancerHandle)
        .updateUniformBuffer (0, uboHandle)
        .end();


    gos::gpu::CmdBufferWriter cw;
    return cw.begin (gpu, cmdBufferHandle)
        .setViewport (gpu->viewport_getDefault())
        .bindPipeline (pipelineHandle)
        .bindDescriptorSet (descrSetInstancerHandle)
        .setClearColor (0, gos::ColorHDR(0, 0.0f, 0.1f))
        .setDepthBufferColor(1, 0)
        .renderPass_begin (renderLayoutHandle, frameBufferHandle)
            .bindVtxBuffer(vtxBufferHandle)
            .bindIdxBufferU16(idxBufferHandle)
            .drawIndexed (NUM_INDEX, 1, 0, 0, 0)
        .renderPass_end()
        .end();
}

/************************************
 * renderizza inviando command buffer a GPU e poi aspettando che questa
 * abbia finito il suo lavoro
 */
void VulkanExample5::virtual_onRun()
{
    mainLoop();
}

//**********************************
void VulkanExample5::doCPUStuff ()
{
    fpsMegaTimer.onFrameBegin(FPSTIMER_CPU);

    glfwPollEvents();

    //prepare frame
    {
        const u64 timeNow_msec = gos::getTimeSinceStart_msec();
        if (timeNow_msec >= anim.nextTimeRotate_msec)
        {
            mat4x4f matT;
            mat4x4f matR;

            anim.nextTimeRotate_msec = timeNow_msec + 16;
            anim.rotation_grad+=1.0f;
//anim.rotation_grad=12.0f;
            anim.zPos += anim.zInc;
            if (anim.zPos >= 10 || anim.zPos < 0)
                anim.zInc = -anim.zInc;
            
            matR.buildRotationAboutY (math::gradToRad(anim.rotation_grad));
            matT.buildTranslation (0,0,anim.zPos);
            ubo.world = matT * matR;
//            ubo.world.identity();


            gos::geom::Camera3 cam;
            cam.setPerspectiveFovLH(gpu->swapChain_calcAspectRatio(),  math::gradToRad(45), 0.1f, 50.0f);
            cam.pos.identity();
            cam.pos.warp (0, 9, -19);
            cam.pos.lookAt (vec3f(0,0,0));
            cam.markUpdated();
            ubo.view = cam.getMatV();
            ubo.proj = cam.getMatP();


            



            vec4f vIN[4];
            vIN[0].set (0,0,0,1);
            vIN[1].set (0,0,1,1);
            vIN[2].set (0,0,10,1);
            vIN[3].set (0,0,100,1);
            vec4f vOUT[4];
            for (u32 i = 0; i < 4; i++)
            {
                //vec4f v (vertexList[i].pos.x, vertexList[i].pos.y, vertexList[i].pos.z, 1);
                //vOUT[i] = math::vecTransform (ubo.proj, v);
                vOUT[i] = math::vecTransform (ubo.proj, vIN[i]);
            }
            vOUT[0].w = 1;


        }
        gpu->uniformBuffer_mapCopyUnmap (uboHandle, 0, sizeof(sUniformBufferObject), &ubo);
    }

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


    fpsMegaTimer.onFrameEnd(FPSTIMER_CPU);
    fpsMegaTimer.printReport();
}


//**********************************
void VulkanExample5::mainLoop()
{
    GPUMainLoop gpuLoop;
    gpuLoop.setup (gpu, &fpsMegaTimer);

    //command buffer 
    GPUCmdBufferHandle  cmdBufferHandle;
    gpu->cmdBuffer_create (eGPUQueueType::gfx, &cmdBufferHandle);

    //main loop
    while (!glfwWindowShouldClose (gpu->getWindow()))
    {
        doCPUStuff ();


        gpuLoop.run ();
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

