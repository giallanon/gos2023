#include "VulkanExample6.h"
#include "../gosShape/gosShapeImport.h"
#include "../gos/gosFIFOFixedSize.h"

using namespace gos;


//************************************
VulkanExample6::VulkanExample6()
{
    bUse_pipe_v2 = true;
    shapeList.setup (gos::getSysHeapAllocator(), 64);
    nextTimeSwapRT_msec = 0;
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

    gpu->deleteResource(rt1);
    gpu->deleteResource(rt2);
    gpu->deleteResource(rt3);
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
    if (!gpu->vtxshader_createFromFile ("@shader/shader2.vert.spv", "main", &vtxShaderHandle))
    {
        gos::logger::err ("VulkanApp::init() => can't create vert shader\n");
        return false;
    }
    if (!gpu->fragshader_createFromFile ("@shader/shader2.frag.spv", "main", &fragShaderHandle))
    {
        gos::logger::err ("VulkanApp::init() => can't create frag shader\n");
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

    //creo un buffer per UBO
    if (!gpu->uniformBuffer_create (sizeof(sUniformBufferObject), eVIBufferMode::shared_cpuW_autoSync, &uboHandle))
    {
        gos::logger::err ("VulkanApp::init() => GPU::uniformBuffer_create\n");
        return false;
    }


    //creazione pipeline
    if (bUse_pipe_v2)
    {
        if (!priv_setupPipeline_v2(vtxShaderHandle, fragShaderHandle))
            return false;
    }
    else
    {
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
        
        if (!priv_setupPipeline_v1(vtxDeclHandle))  
            return false;

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
bool VulkanExample6::priv_setupPipeline_v2 (GPUShaderHandle vtxShaderHandle, GPUShaderHandle fragShaderHandle)
{
    const eImageFormat IMG_FORMAT = eImageFormat::U8_RGBA;
    //risorse di rendering
    if (!gpu->renderTarget_create ("0-", "0-", IMG_FORMAT, &rt1))
        return false;

    if (!gpu->renderTarget_create ("0-", "0-", eImageFormat::U8_RGBA, &rt2))
        return false;

    if (!gpu->renderTarget_create ("0-", "0-", eImageFormat::U8_RGBA, &rt3))
        return false;        

    //pipeline def
    gpu::pipe2::Pipeline_def def;
    def.reset();
    

    def.vtxStream_add(eVtxStreamInputRate::perVertex)
        .add (0, offsetof(Vertex, pos), eDataFormat::_3f32)
        .add (1, offsetof(Vertex, tutv0), eDataFormat::_2f32)
        .add (2, offsetof(Vertex, normal), eDataFormat::_3f32);

    
    def.descriptorset_add()
        .add (0, eGPUDescriptrorType::UNIFORM_BUFFER, 1, eGPUDescriptrorUsageFlag::vtx_shader);


    def.add_rt (IMG_FORMAT);
    def.add_rt (eImageFormat::U8_RGBA);

    def.shader_add (vtxShaderHandle);
    def.shader_add (fragShaderHandle);

    gpu::pipe2::Pipeline pipeline;
    if (!gpu->pipeline_v2_createNew (def, &pipeline))
        return false;
    pipelineHandle = pipeline.pipeline_handle;
    descrSetLayoutHandle = pipeline.descrset_handle_defList[0];


    return true;
    /*
    gpu::Framebuffer_def fbd;
    {
        fbd.reset();
        fbd.add (gpu->swapChain_getImageFormat(), eImageLayout::undefined, eImageLayout::presentation, eAttachmentLoadOp::clear, eAttachmentStoreOp::store);
        fbd.add (gpu->depthStencil_getDefaultFormat(), eImageLayout::undefined, eImageLayout::depth_shader_readonly, eAttachmentLoadOp::clear, eAttachmentStoreOp::dont_care);
    }*/
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
bool VulkanExample6::recordCommandBuffer (GPUCmdBufferHandle &cmdBufferHandle, VkImage swapChainImage)
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
    cw.begin (gpu, cmdBufferHandle);

    if (bUse_pipe_v2)
    {
        priv_recordCommandBuffer_v2(cw, swapChainImage);
    }
    else
    {
        priv_recordCommandBuffer_v1(cw);
    }
    

    return cw.end();                 
}

//************************************
bool VulkanExample6::priv_recordCommandBuffer_v1 (gos::gpu::CmdBufferWriter &cw)
{
    cw
        .setViewport (gpu->viewport_getDefault())
        .setClearColor (0, gos::ColorHDR(0, 0.1f, 0.3f))
        .setDepthBufferColor(1, 0)
        .renderPass_begin (renderPassHandle, frameBufferHandle)
            .bindVtxBuffer(vtxBufferHandle)
            .bindIdxBufferU16(idxBufferHandle)

            .bindPipeline (pipelineHandle)
            .bindDescriptorSet (descrSetInstancerHandle, 0);

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
    return true;       
}

//************************************
bool VulkanExample6::priv_recordCommandBuffer_v2 (gos::gpu::CmdBufferWriter &cw, VkImage swapChainImage)
{
    VkCommandBuffer cmd = cw.debug_getHandle();
    

    //const gpu::RenderTarget     *hRT1_info = gpu->getInfo (gpu->renderTarget_getDefault());
    const gpu::DepthStencil     *zBuffer_info = gpu->getInfo (gpu->depthStencil_getDefault());
    const gpu::RenderTarget     *rt1_info = gpu->getInfo (rt1);
    const gpu::RenderTarget     *rt2_info = gpu->getInfo (rt2);
    
    //.requireRendertarget (gpu->swapChain_getImageFormat(), eImageLayout::undefined, eImageLayout::presentation, eAttachmentLoadOp::clear, eAttachmentStoreOp::store)
    VkRenderingAttachmentInfo   attachInfoList[2];
    memset (attachInfoList, 0, sizeof(attachInfoList));
        attachInfoList[0].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        attachInfoList[0].imageView = rt1_info->view;
        attachInfoList[0].imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
        attachInfoList[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachInfoList[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachInfoList[0].clearValue.color.float32[0] = 0; //r
        attachInfoList[0].clearValue.color.float32[1] = 0.1f; //g
        attachInfoList[0].clearValue.color.float32[2] = 0.1f; //b
        attachInfoList[0].clearValue.color.float32[3] = 1.0f; //a

        attachInfoList[1].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        attachInfoList[1].imageView = rt2_info->view;
        attachInfoList[1].imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
        attachInfoList[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachInfoList[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachInfoList[1].clearValue.color.float32[0] = 1; //r
        attachInfoList[1].clearValue.color.float32[1] = 0; //g
        attachInfoList[1].clearValue.color.float32[2] = 0; //b
        attachInfoList[1].clearValue.color.float32[3] = 1; //a        


    //.requireZBuffer (gpu->depthStencil_getDefaultFormat(), eImageLayout::undefined, eImageLayout::depth_shader_readonly, eAttachmentLoadOp::clear, eAttachmentStoreOp::dont_care)
    VkRenderingAttachmentInfo   zBufferAttach;
    memset (&zBufferAttach, 0, sizeof(zBufferAttach));
        zBufferAttach.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        zBufferAttach.imageView = zBuffer_info->view;
        zBufferAttach.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL_KHR;
        zBufferAttach.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        zBufferAttach.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        zBufferAttach.clearValue.depthStencil.depth = 1;



    VkRenderingInfo renderingInfo;
    memset (&renderingInfo, 0, sizeof(renderingInfo));
        renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
        renderingInfo.renderArea.extent.width = rt1_info->resolvedW;
        renderingInfo.renderArea.extent.height = rt1_info->resolvedH;
        renderingInfo.layerCount = 1;
        renderingInfo.colorAttachmentCount = 2;
        renderingInfo.pColorAttachments = attachInfoList;
        renderingInfo.pDepthAttachment = &zBufferAttach;
    

    cw.setViewport (gpu->viewport_getDefault());


    //cw.imageTransition (gpu->swapChain_getCurImage(), eImageLayout::undefined, eImageLayout::color_attachment_optimal);
    cw.imageTransition (rt1_info->image, eImageLayout::undefined, eImageLayout::color_attachment_optimal);
    cw.imageTransition (rt2_info->image, eImageLayout::undefined, eImageLayout::color_attachment_optimal);
    cw.imageTransition (zBuffer_info->image, eImageLayout::undefined, eImageLayout::depth_attachment_optimal);
    vkCmdBeginRendering (cmd, &renderingInfo);
        //.setClearColor (0, gos::ColorHDR(0, 0.1f, 0.3f))
        //.setDepthBufferColor(1, 0)
        //.renderPass_begin (renderPassHandle, frameBufferHandle)
    cw.bindPipeline (pipelineHandle);
    cw.bindDescriptorSet (descrSetInstancerHandle, 0);
    cw.bindVtxBuffer(vtxBufferHandle);
    cw.bindIdxBufferU16(idxBufferHandle);



    //draw
    u32 firstIndex = 0;
    u32 firstVtx = 0;
    for (u32 i=0; i<shapeList.getNElem(); i++)
    {
        const gos::Shape *myShape = &shapeList(i);
        
        //cw.drawIndexed (myShape->numIdx, 1, firstIndex, firstVtx, 0);
        vkCmdDrawIndexed (cmd, myShape->numIdx, 1, firstIndex, firstVtx, 0);

        firstIndex += myShape->numIdx;
        firstVtx += myShape->numVtx;
    }
    
    //end
    vkCmdEndRendering (cmd);

    //cw.imageTransition (gpu->swapChain_getCurImage(), eImageLayout::color_attachment_optimal, eImageLayout::presentation);
    
    //copio RT1 nella immagine di swapchain corrente
    if (gos::getTimeSinceStart_msec() > nextTimeSwapRT_msec)
    {
        nextTimeSwapRT_msec = static_cast<u32>(gos::getTimeSinceStart_msec() + 2000);
        if (rt1_info == rtToShow)
            rtToShow = rt2_info;
        else
            rtToShow = rt1_info;
    }
    cw  
        .imageTransition (rtToShow->image, eImageLayout::color_attachment_optimal, eImageLayout::transfer_src)
        .imageTransition (swapChainImage, eImageLayout::undefined, eImageLayout::transfer_dst)
        .copyImageToImage (rtToShow->image, swapChainImage, gpu->swapChain_getImageExten2D(), gpu->swapChain_getImageExten2D())
        .imageTransition (swapChainImage, eImageLayout::transfer_dst, eImageLayout::presentation);

    return true;
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
    //case COMPILE_TIME_STR_CRC32("rotateY"):                movement.mouseRotateY (value); break;
    //case COMPILE_TIME_STR_CRC32("rotateX"):                movement.mouseRotateX (value); break;
    
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
    //priv_mainLoop1();
    //priv_mainLoop2();
    priv_mainLoop3();
}

//**********************************
void VulkanExample6::priv_mainLoop1()
{
    gpu::MainLoop gpuLoop;
    gpuLoop.setup (gpu);
    gpuLoop.stat_setPrintReportEvery (10000);


    //command buffer 
    GPUCmdBufferHandle  cmdBufferHandle;
    gpu->cmdBuffer_create (eGPUQueueType::gfx, &cmdBufferHandle);

    gpuLoop.stat_setPrintReportEvery (1000);

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
            recordCommandBuffer (cmdBufferHandle, VK_NULL_HANDLE);
            gpuLoop.submitGFXJob (cmdBufferHandle);
        }

    }

    //aspetto che GPU abbia finito tutto cio' che ha in coda
    gpu->waitIdle();

    //free
    gpu->deleteResource (cmdBufferHandle);
    gpuLoop.unsetup();
}

//**********************************
void VulkanExample6::priv_mainLoop2()
{
    gpu::AquireSwapChainImage acquireImage;
    gpu::PresentGFXJob presentJob;
    acquireImage.setup (gpu);
    presentJob.setup (gpu);

    gos::TimerFPS cpuTimer;


    //command buffer 
    GPUCmdBufferHandle  cmdBufferHandle;
    gpu->cmdBuffer_create (eGPUQueueType::gfx, &cmdBufferHandle);


    gos::FIFOFixedSize<gpu::AcquiredSwapchainImg, 4> acquiredList;

    //main loop
    u64 nextTimePrintReport_ms = 0;
    while (bQuitApp == false)
    {
        cpuTimer.onFrameBegin();
        doCPUStuff ();
        cpuTimer.onFrameEnd();

        if (gpu->swapChain_wasRecreated())
        {
            acquiredList.reset();
            cam.changeAspectRatioPerspectiveFovLH (gpu->swapChain_calcAspectRatio());
        }

        //chiedo una immagine alla swapchain, ne accumulo fino a 2
        if (acquiredList.getNElem() < 2)
        {
            if (acquireImage.tryAcquire ())
                acquiredList.push (acquireImage.acquiredImg);
        }

        //se il job precedente e' stato presentato, posso schedularne uno nuovo
        if (presentJob.hasFinished())
        {
            //..ammesso che abvia gia' una swapchain-image disponibile
            if (acquiredList.getNElem() > 0)
            {
                gpu::AcquiredSwapchainImg info;
                acquiredList.pop (&info);

                recordCommandBuffer (cmdBufferHandle, info.image);
                presentJob.submit (cmdBufferHandle, info.index);
            }
        }


        // un po' di statistiche
        const u64 timenow_ms = gos::getTimeSinceStart_msec();
        if (timenow_ms >= nextTimePrintReport_ms)
        {
            nextTimePrintReport_ms = timenow_ms + 1000;

            printf ("cpu: avg %.2fms [fps: %.01f]    gpu: avg %.2fms [fps: %.01f]    acquire: avg %.2fms [fps: %.01f]\n",
                cpuTimer.getAvgFrameTime_ms(), cpuTimer.getAvgFPS(),
                presentJob.timerFPS.getAvgFrameTime_ms(), presentJob.timerFPS.getAvgFPS(),
                acquireImage.timerFPS.getAvgFrameTime_ms(), acquireImage.timerFPS.getAvgFPS());
        }
    }

    //aspetto che GPU abbia finito tutto cio' che ha in coda
    gpu->waitIdle();

    //free
    gpu->deleteResource (cmdBufferHandle);
}

//**********************************
void VulkanExample6::priv_mainLoop3()
{
    gpu::MainLoop2 mainLoop;
    mainLoop.setup (gpu);


    //command buffer 
    GPUCmdBufferHandle  cmdBufferHandle;
    gpu->cmdBuffer_create (eGPUQueueType::gfx, &cmdBufferHandle);


    //main loop
    while (bQuitApp == false)
    {
        mainLoop.stat_onCPUFrameBegin();
        doCPUStuff ();
        mainLoop.stat_onCPUFrameEnd();


        mainLoop.run();

        if (gpu->swapChain_wasRecreated())
            cam.changeAspectRatioPerspectiveFovLH (gpu->swapChain_calcAspectRatio());


        //se il job precedente e' stato presentato, posso schedularne uno nuovo
        gpu::AcquiredSwapchainImg swapchainImg;
        if (mainLoop.gfxJob_canSubmit(&swapchainImg))
        {
            recordCommandBuffer (cmdBufferHandle, swapchainImg.image);
            mainLoop.gfxJob_submitAndPresent (cmdBufferHandle, swapchainImg);
        }
    }

    //aspetto che GPU abbia finito tutto cio' che ha in coda
    gpu->waitIdle();

    //free
    gpu->deleteResource (cmdBufferHandle);
}
