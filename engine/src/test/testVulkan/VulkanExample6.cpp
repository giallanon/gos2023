#include "VulkanExample6.h"
#include "../gosShape/gosShapeImport.h"


using namespace gos;


//************************************
VulkanExample6::VulkanExample6()
{
    shapeList.setup (gos::getSysHeapAllocator(), 64);
}

//************************************
void VulkanExample6::virtual_explain()
{
    gos::logger::log ("import da .glTF\n");
    gos::logger::log (eTextColor::white, "TAB = toggle mouse mode\n");
}


//************************************
void VulkanExample6::virtual_onCleanup() 
{
    for (u32 i=0; i<shapeList.getNElem(); i++)
    {
        shape::shapeFree (gos::getSysHeapAllocator(), &shapeList[i]);
    }
    shapeList.unsetup();

    gpu->deleteResource (idxBufferHandle);
    gpu->deleteResource (stgBufferHandle);
    gpu->deleteResource (vtxBufferHandle);
    gpu->deleteResource (vtxShaderHandle);
    gpu->deleteResource (fragShaderHandle);
    gpu->deleteResource (pipelineHandle);
    gpu->deleteResource (renderPassHandle);
    gpu->deleteResource (frameBufferHandle);
    gpu->deleteResource (uboHandle);
    gpu->deleteResource (descrSetInstancerHandle);
    gpu->deleteResource (descrSetLayoutHandle);
    gpu->deleteResource (descrPoolHandle);
}    


//************************************
bool VulkanExample6::virtual_onInit ()
{
    //importazione modello
    {
        gos::VtxLayout vtxLayot;
        shape::VtxLayoutWriter writer(&vtxLayot);
        writer.begin()
            .addPos3(offsetof(Vertex, pos))
            .addTexCoord(offsetof(Vertex, tutv0))
            .addNorm3(offsetof(Vertex, normal))
        .end();
    
        //gos::shape::importFrom_dae ("shader/example6/esempio.dae", vtxLayot, gos::getSysHeapAllocator(), shapeList);
        //gos::shape::importFrom_dae ("shader/example6/omino/omino2.dae", vtxLayot, gos::getSysHeapAllocator(), shapeList);
        //gos::shape::importFrom_dae ("shader/example6/sponza/sponza.dae", vtxLayot, gos::getSysHeapAllocator(), shapeList);
        
        //if (!gos::shape::importFrom_glTF ("shader/example6/cubo-normal.mapped/cubo.glb", vtxLayot, gos::getSysHeapAllocator(), shapeList)) return false;
        //if (!gos::shape::importFrom_glTF ("shader/example6/omino/omino.glb", vtxLayot, gos::getSysHeapAllocator(), shapeList)) return false;
        //if (!gos::shape::importFrom_glTF ("shader/example6/angolo.glb", vtxLayot, gos::getSysHeapAllocator(), shapeList)) return false;
        //if (!gos::shape::importFrom_glTF ("shader/example6/albero/albero.glb", vtxLayot, gos::getSysHeapAllocator(), shapeList)) return false;
        //if (!gos::shape::importFrom_glTF ("shader/example6/esempio2.glb", vtxLayot, gos::getSysHeapAllocator(), shapeList)) return false;
        
        if (!gos::shape::importFrom_glTF ("shader/example6/sponza/sponza.glb", vtxLayot, gos::getSysHeapAllocator(), shapeList)) return false;
        //if (!gos::shape::importFrom_glTF ("/home/giallanon/Desktop/info/Blender/modelli/models_from_glTF_repo/Sponza/glTF/Sponza.glb", vtxLayot, gos::getSysHeapAllocator(), shapeList)) return false;
        //if (!gos::shape::importFrom_glTF ("/home/giallanon/Desktop/info/Blender/modelli/models_from_glTF_repo/DamagedHelmet/glTF/DamagedHelmet.glb", vtxLayot, gos::getSysHeapAllocator(), shapeList)) return false;
        //if (!gos::shape::importFrom_glTF ("/home/giallanon/Desktop/info/Blender/modelli/models_from_glTF_repo/Duck/glTF-Binary/Duck.glb", vtxLayot, gos::getSysHeapAllocator(), shapeList)) return false;
        //if (!gos::shape::importFrom_glTF ("/home/giallanon/Desktop/info/Blender/modelli/models_from_glTF_repo/BrainStem/glTF-Binary/BrainStem.glb", vtxLayot, gos::getSysHeapAllocator(), shapeList)) return false;
        
    }

    //creo vtx/idx/staging buffer
    if (!createVertexIndexStageBuffer())
    {
        gos::logger::err ("VulkanApp::init() => can't create buffers\n");
        return false;
    }

    //copio i Vtx in vtxBuffer e idx in idxBuffer tramite lo staging array
    {
        u32 vtxBufferSize = 0;
        u32 idxBufferSize = 0;
        for (u32 i=0; i<shapeList.getNElem(); i++)
        {
            const gos::Shape *myShape = &shapeList(i);

            if (!gpu->stagingBuffer_uploadToGPUBuffer (stgBufferHandle, myShape->vtxBuffer, vtxBufferHandle, vtxBufferSize, sizeof(Vertex) * myShape->numVtx))
            {
                gos::logger::err ("VulkanApp::init() => can't upload to VtxBuffer\n");
                return false;
            }
            vtxBufferSize += sizeof(Vertex) * myShape->numVtx;

            if (!gpu->stagingBuffer_uploadToGPUBuffer (stgBufferHandle, myShape->idxBuffer, idxBufferHandle, idxBufferSize, sizeof(u16) * myShape->numIdx))
            {
                gos::logger::err ("VulkanApp::init() => can't upload to IdxBuffer\n");
                return false;
            }
            idxBufferSize += sizeof(u16) * myShape->numIdx;
        }
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

    //Vtx declaration
    GPUVtxDeclHandle vtxDeclHandle;
    gpu->vtxDecl_createNew (&vtxDeclHandle)
        .addStream(eVtxStreamInputRate::perVertex)
        .addLayout (0, offsetof(Vertex, pos), eDataFormat::_3f32)
        .addLayout (1, offsetof(Vertex, tutv0), eDataFormat::_2f32)
        .addLayout (2, offsetof(Vertex, normal), eDataFormat::_3f32)
        .end();
    if (vtxDeclHandle.isInvalid())
    {
        gos::logger::err ("VulkanApp::init() => can't create vtxDeclHandle\n");
        return false;
    }    

    //creazione pipeline
    //if (!priv_setupPipeline_v1(vtxDeclHandle))  return false;
    if (!priv_setupPipeline_v2(vtxDeclHandle))  return false;

    //non mi serve piu'
    gpu->deleteResource (vtxDeclHandle);


    //frame buffers
    gpu->frameBuffer_createNew (renderPassHandle, &frameBufferHandle)
        .bindRenderTarget (gpu->renderTarget_getDefault())
        .bindDepthStencil (gpu->depthStencil_getDefault())
        .end();
    if (frameBufferHandle.isInvalid())
    {
        gos::logger::err ("VulkanApp::init() => can't create frameBufferHandle\n");
        return false;
    }     

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
bool VulkanExample6::priv_setupPipeline_v1(GPUVtxDeclHandle &vtxDeclHandle)
{
    //creo il render pass
    gpu->renderPass_createNew (&renderPassHandle)
        .requireRendertarget (gpu->swapChain_getImageFormat(), eImageLayout::undefined, eImageLayout::presentation, eAttachmentLoadOp::clear, eAttachmentStoreOp::store)
        .requireZBuffer (gpu->depthStencil_getDefaultFormat(), eImageLayout::undefined, eImageLayout::depth_shader_readonly, eAttachmentLoadOp::clear, eAttachmentStoreOp::dont_care)
        .addSubpass_GFX()
            .writeToRenderTarget(0)
            .writeToDepthStencil()
        .end()
    .end();
    if (renderPassHandle.isInvalid())
    {
        gos::logger::err ("VulkanApp::init() => can't create renderTaskLayout\n");
        return false;
    }


    //Creo il descriptorSet layout  con un solo UNIFORM BUFFER per il VTX SHADER
    gpu->descrSetLayout_create(&descrSetLayoutHandle)
        .add_uniformBuffer (VK_SHADER_STAGE_VERTEX_BIT)
        .end();
    if (descrSetLayoutHandle.isInvalid())
    {
        gos::logger::err ("VulkanApp::init() => can't create descriptor set\n");
        return false;
    }

    //creo la pipeline
    gpu->pipeline_createNew (renderPassHandle, &pipelineHandle)
        .addShader (vtxShaderHandle)
        .addShader (fragShaderHandle)
        .setVtxDecl (vtxDeclHandle)
        //.setWireframe(true)
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
    return true;
}

//************************************
bool VulkanExample6::priv_setupPipeline_v2(GPUVtxDeclHandle &vtxDeclHandleIN)
{

    gpu::Framebuffer_def fbd;
    {
        fbd.reset();
        fbd.add (gpu->swapChain_getImageFormat(), eImageLayout::undefined, eImageLayout::presentation, eAttachmentLoadOp::clear, eAttachmentStoreOp::store);
        fbd.add (gpu->depthStencil_getDefaultFormat(), eImageLayout::undefined, eImageLayout::depth_shader_readonly, eAttachmentLoadOp::clear, eAttachmentStoreOp::dont_care);
    }

    gpu::RenderPassDesc rpd;
    {
        rpd.reset();

        rpd.framebuffer_def = &fbd;

        gpu::RenderPassDesc::sSubpass *sub = &rpd.subpassList[0];

        sub->setType_gfx();
        sub->add_rt (0);
        sub->set_zbuffer (1, true, eZFunc::LESS, false, eStencilFunc::NEVER);

        sub->add_descriptorSet (0, 0);
        sub->add_descriptor (0, 0, VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 1);

        sub->vtxDeclHandle = vtxDeclHandleIN;
        sub->vtxShaderHandle = vtxShaderHandle;
        sub->pxlShaderHandle = fragShaderHandle;
    }

    return priv_buildPipe_v2(rpd);
}

//************************************
bool VulkanExample6::priv_buildPipe_v2 (const gpu::RenderPassDesc &rpd)
{
    //creo il render pass
    {
        GPU::RenderPassBuilder& builder = gpu->renderPass_createNew (&renderPassHandle);
        for (u32 i=0; i<rpd.framebuffer_def->numAttachment; i++)
        {
            const gpu::Framebuffer_def::sAttachment *attach = &rpd.framebuffer_def->attachment[i];
            if (image::isFormatWithDepth(attach->fmt))
            {
                builder.requireZBuffer (attach->fmt, attach->initialLayout, attach->finalLayout, attach->loadOp, attach->storeOp);
            }
            else
            {
                builder.requireRendertarget (attach->fmt, attach->initialLayout, attach->finalLayout, attach->loadOp, attach->storeOp);
            }
        }

        for (u32 i=0; i<GOSGPU__NUM_MAX_SUBPASSES; i++)
        {
            const gpu::RenderPassDesc::sSubpass *subpass = &rpd.subpassList[i];

            if (gpu::RenderPassDesc::eSubpassType::unknown == subpass->type)
                break;


            GPU::RenderPassBuilder::SubPassInfo &sp = builder.addSubpass_GFX();
            {
                for (u32 i2=0; i2<subpass->num_rt; i2++)
                    sp.writeToRenderTarget(subpass->rtList[i2]);

                if (0xFF != subpass->zbIndex)
                    sp.writeToDepthStencil();
            }
        }
        builder.end();

        if (renderPassHandle.isInvalid())
        {
            gos::logger::err ("VulkanApp::init() => can't create renderTaskLayout\n");
            return false;
        }
    }


    const gpu::RenderPassDesc::sSubpass *subpass = &rpd.subpassList[0];

    //Creo il descriptorSet layout  con un solo UNIFORM BUFFER per il VTX SHADER
    {
        gpu->descrSetLayout_create(&descrSetLayoutHandle)
            .add_uniformBuffer (VK_SHADER_STAGE_VERTEX_BIT)
            .end();
        if (descrSetLayoutHandle.isInvalid())
        {
            gos::logger::err ("VulkanApp::init() => can't create descriptor set\n");
            return false;
        }
    }

    //creo la pipeline
    gpu->pipeline_createNew (renderPassHandle, &pipelineHandle)
        .addShader (subpass->vtxShaderHandle)
        .addShader (subpass->pxlShaderHandle)
        .setVtxDecl (subpass->vtxDeclHandle)
        //.setWireframe(true)
        .depthStencil()
            .zbuffer_enable((subpass->zbIndex != 0xff))
            .zbuffer_enableWrite(subpass->zbuffer_write)
            .zbuffer_setFn (subpass->zbuffer_cmpFn)
        .end() //depth stencil
        .setCullMode (subpass->cullMode)
        .setDrawPrimitive (subpass->drawPrimitive)
        .descriptor_add (descrSetLayoutHandle)
        .end ();

    if (pipelineHandle.isInvalid())
    {
        gos::logger::err ("VulkanApp::init() => can't create pipeline\n");
        return false;
    }
    return true;
}    


//************************************
bool VulkanExample6::createVertexIndexStageBuffer()
{
    u32 totNumVtx = 0;
    u32 totNumIdx = 0;

    for (u32 i=0; i<shapeList.getNElem(); i++)
    {
        const gos::Shape *myShape = &shapeList(i);
        totNumVtx += myShape->numVtx;
        totNumIdx += myShape->numIdx;
    }

    if (!gpu->vertexBuffer_create (totNumVtx * sizeof(Vertex), eVIBufferMode::onGPU, &vtxBufferHandle))
    {
        gos::logger::err ("VulkanApp::createVertexIndexStageBuffer() => gpu->vertexBuffer_create() failed\n");
        return false;
    }

    //INDEX BUFFER
    if (!gpu->indexBuffer_create (totNumIdx * sizeof(u16), eVIBufferMode::onGPU, &idxBufferHandle))
    {
        gos::logger::err ("VulkanApp::createVertexIndexStageBuffer() => gpu->indexBuffer_create() failed\n");
        return false;
    }

    //Creo anche uno staging buffer
    if (!gpu->stagingBuffer_create (totNumVtx * sizeof(Vertex), &stgBufferHandle))
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
    //ubo.lightDir.set (0, -0.5f, 1, 0);
    ubo.lightDir = vec4f (cam.pos.getAsseZ(), 0);
    ubo.lightDir.normalize();
    gpu->writeAndSync (uboHandle, 0, &ubo, sizeof(sUniformBufferObject));

    gos::gpu::DescrSetInstanceWriter descrWriter;
    descrWriter.begin (gpu, descrSetInstancerHandle)
        .bindUniformBuffer (0, uboHandle)
        .end();


    gos::gpu::CmdBufferWriter cw;
    cw.begin (gpu, cmdBufferHandle)
        .setViewport (gpu->viewport_getDefault())
        .setClearColor (0, gos::ColorHDR(0, 0.1f, 0.3f))
        .setDepthBufferColor(1, 0)
        .renderPass_begin (renderPassHandle, frameBufferHandle)
            .bindVtxBuffer(vtxBufferHandle)
            .bindIdxBufferU16(idxBufferHandle)

            .bindPipeline (pipelineHandle)
            .bindDescriptorSet (descrSetInstancerHandle, 0)
        ;


            u32 firstIndex = 0;
            u32 firstVtx = 0;
            for (u32 i=0; i<shapeList.getNElem(); i++)
            {
                const gos::Shape *myShape = &shapeList(i);
                cw.drawIndexed (myShape->numIdx, 1, firstIndex, firstVtx, 0);

                firstIndex += myShape->numIdx;
                firstVtx += myShape->numVtx;
            }

        cw.renderPass_end();
        return cw.end();        
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
void VulkanExample6::virtual_onInputEvent (u32 actionID, i16 value, const gos::input::MouseStatus &mouseStatus, const gos::input::sButtonModifier &btnModifier)
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
    
    }
}

//**********************************
void VulkanExample6::doCPUStuff ()
{
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
}


//**********************************
void VulkanExample6::mainLoop()
{
    gpu::MainLoop gpuLoop;
    gpuLoop.setup (gpu);

    //command buffer 
    GPUCmdBufferHandle  cmdBufferHandle;
    gpu->cmdBuffer_create (eGPUQueueType::gfx, &cmdBufferHandle);

    //main loop
    while (bQuitApp == false)
    {
        gpuLoop.stat_onCPUFrameBegin();
        doCPUStuff ();
        gpuLoop.stat_onCPUFrameEnd();
        gpuLoop.stat_printReport();


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

