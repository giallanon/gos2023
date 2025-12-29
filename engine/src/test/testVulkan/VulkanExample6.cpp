#include "VulkanExample6.h"
#include "../gosShape/gosShapeImport.h"
#include "../gos/gosFIFOFixedSize.h"
#include "gosAsset2Loader.h"

using namespace gos;


//************************************
VulkanExample6::VulkanExample6()
{
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

    //unload degli asset
    {
        theHub.unload (assetPipe);
        theHub.unload (assetPipe2);
        theHub.flush(gos::getTimeSinceStart_msec());
    }


    gpu->deleteResource (idxBufferHandle);
    gpu->deleteResource (stgBufferHandle);
    gpu->deleteResource (vtxBufferHandle);
    gpu->deleteResource (uboHandle);
    gpu->deleteResource (descrSetInstancerHandle);
    
    gpu->deleteResource (descrPoolHandle);

    gpu->deleteResource(rt1);
    gpu->deleteResource(rt2);
    gpu->deleteResource(rt3);
    gpu->deleteResource (zbufferHandle);
}    

//************************************
bool VulkanExample6::priv_loadModel()
{
    gos::VtxLayout vtxLayot;
    shape::VtxLayoutWriter writer(&vtxLayot);
    writer.begin()
        .addPos3(offsetof(Vertex, pos))
        .addTexCoord(offsetof(Vertex, tutv0))
        .addNorm3(offsetof(Vertex, normal))
    .end();

    //if (!gos::shape::importFrom_glTF ("@common_assets/model3d/cubo-normal.mapped/cubo.glb", vtxLayot, gos::getSysHeapAllocator(), shapeList)) return false;
    if (!gos::shape::importFrom_glTF ("@common_assets/model3d/omino/omino.glb", vtxLayot, gos::getSysHeapAllocator(), shapeList)) return false;
    //if (!gos::shape::importFrom_glTF ("@common_assets/model3d/albero/albero.glb", vtxLayot, gos::getSysHeapAllocator(), shapeList)) return false;
    //if (!gos::shape::importFrom_glTF ("@common_assets/model3d/esempio2.glb", vtxLayot, gos::getSysHeapAllocator(), shapeList)) return false;
    
    //if (!gos::shape::importFrom_glTF ("@common_assets/model3d/sponza/sponza.glb", vtxLayot, gos::getSysHeapAllocator(), shapeList)) return false;
    //if (!gos::shape::importFrom_glTF ("/home/giallanon/Desktop/info/Blender/modelli/models_from_glTF_repo/Sponza/glTF/Sponza.glb", vtxLayot, gos::getSysHeapAllocator(), shapeList)) return false;
    //if (!gos::shape::importFrom_glTF ("/home/giallanon/Desktop/info/Blender/modelli/models_from_glTF_repo/DamagedHelmet/glTF/DamagedHelmet.glb", vtxLayot, gos::getSysHeapAllocator(), shapeList)) return false;
    //if (!gos::shape::importFrom_glTF ("/home/giallanon/Desktop/info/Blender/modelli/models_from_glTF_repo/Duck/glTF-Binary/Duck.glb", vtxLayot, gos::getSysHeapAllocator(), shapeList)) return false;
    //if (!gos::shape::importFrom_glTF ("/home/giallanon/Desktop/info/Blender/modelli/models_from_glTF_repo/BrainStem/glTF-Binary/BrainStem.glb", vtxLayot, gos::getSysHeapAllocator(), shapeList)) return false;
    


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
    return true;
}

//************************************
bool VulkanExample6::virtual_onInit ()
{
    //builder per ricompilare gli asset se necessario
    {
        gos::asset2::Builder builder(gpu);
        if (!builder.build("shader/example6/assets", true))
            return false;
    }

    //theHub
    if (!theHub.setup ("shader/example6/assets", gpu))
        return false;
    theHub.getHandle("pipe_1", &assetPipe);
    theHub.getHandle("pipe_2", &assetPipe2);


    //importazione modello
    priv_loadModel();
    

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
    if (!gpu->uniformBuffer_create (sizeof(sUniformBufferObject), eMemAccessMode::shared_cpuW_autoSync, &uboHandle))
    {
        gos::logger::err ("VulkanApp::init() => GPU::uniformBuffer_create\n");
        return false;
    }

    //zbuffer
    if (!gpu->zbuffer_create ("0-", "0-", eImageFormat::_DEPTH_BEST, &zbufferHandle))
    {
        gos::logger::err ("VulkanApp::init() => GPU::zbuffer_create\n");
        return false;
    }


    //creazione pipeline
    const asset2::Asset_pipe *pipe;
    theHub.getAssetWithTimeout (assetPipe, &pipe, 5000);
    
    //risorse di rendering
    const eImageFormat IMG_FORMAT = eImageFormat::U8_RGBA;
    if (!gpu->renderTarget_create ("0-", "0-", IMG_FORMAT, &rt1))
        return false;

    if (!gpu->renderTarget_create ("0-", "0-", eImageFormat::U8_RGBA, &rt2))
        return false;

    if (!gpu->renderTarget_create ("0-", "0-", eImageFormat::U8_RGBA, &rt3))
        return false;      



    //alloco una istanza del descriptorSet
    if (!gpu->descrSetInstance_create (descrPoolHandle, pipe->handle_pipe, 0, &descrSetInstancerHandle))
    {
        gos::logger::err ("VulkanApp::init() => can't create descriptorSet instance\n");
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

    if (!gpu->vertexBuffer_create (totNumVtx * sizeof(Vertex), eMemAccessMode::onGPU, &vtxBufferHandle))
    {
        gos::logger::err ("VulkanApp::createVertexIndexStageBuffer() => gpu->vertexBuffer_create() failed\n");
        return false;
    }

    //INDEX BUFFER
    if (!gpu->indexBuffer_create (totNumIdx * sizeof(u16), eMemAccessMode::onGPU, &idxBufferHandle))
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
    const asset2::Asset_pipe *pipe;
    if (!theHub.getAsset(assetPipe, &pipe))
        return false;


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

    
    
    gos::gpu::pipe2::CmdBufferWriter2 cw;
    cw
        .begin (gpu, cmdBufferHandle)
        .setViewport (gpu->viewport_getDefault());

    priv_recordCommandBuffer_v2(cw, swapChainImage, pipe);

    return cw.end();                 
}


//************************************
bool VulkanExample6::priv_recordCommandBuffer_v2 (gos::gpu::pipe2::CmdBufferWriter2 &cw, VkImage swapChainImage, const asset2::Asset_pipe *pipe)
{
    cw
        .imageTransition (rt1, eImageLayout::undefined, eImageLayout::color_attachment_optimal)
        .imageTransition (rt2, eImageLayout::undefined, eImageLayout::color_attachment_optimal)
        .imageTransition (zbufferHandle, eImageLayout::undefined, eImageLayout::depth_attachment_optimal);        

    auto &rr = cw.beginRender();
    rr
        .withRenderArea (rt1)
        .withRT (rt1, eAttachmentLoadOp::clear, eAttachmentStoreOp::dont_care, gos::ColorHDR(0, 0.1f, 0.1f))
        .withRT (rt2, eAttachmentLoadOp::clear, eAttachmentStoreOp::dont_care, gos::ColorHDR(1.0f, 0, 0))
        .withZB (zbufferHandle, eAttachmentLoadOp::clear, eAttachmentStoreOp::dont_care)    

        .bindPipeline (pipe->handle_pipe)
        .bindDescriptorSet (descrSetInstancerHandle, 0)
        .bindVtxBuffer(vtxBufferHandle)
        .bindIdxBufferU16(idxBufferHandle);

    //draw
    u32 firstIndex = 0;
    u32 firstVtx = 0;
    for (u32 i=0; i<shapeList.getNElem(); i++)
    {
        const gos::Shape *myShape = &shapeList(i);

        rr.drawIndexed (myShape->numIdx, 1, firstIndex, firstVtx, 0);

        firstIndex += myShape->numIdx;
        firstVtx += myShape->numVtx;
    }
    rr.endRender();


    //copio RT1/RT2 nella immagine di swapchain corrente a turno
    if (gos::getTimeSinceStart_msec() > nextTimeSwapRT_msec)
    {
        nextTimeSwapRT_msec = static_cast<u32>(gos::getTimeSinceStart_msec() + 2000);
        if (rt1 == rtToShow)
            rtToShow = rt2;
        else
            rtToShow = rt1;
    }
    cw  
        .imageTransition (rtToShow, eImageLayout::color_attachment_optimal, eImageLayout::transfer_src)
        .imageTransition (swapChainImage, eImageLayout::undefined, eImageLayout::transfer_dst)
        .copyImageToImage (rtToShow, swapChainImage, gpu->swapChain_getImageExten2D(), gpu->swapChain_getImageExten2D())
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
    gpu::MainLoop2 mainLoop;
    mainLoop.setup (gpu);


    //command buffer 
    GPUCmdBufferHandle  cmdBufferHandle;
    gpu->cmdBuffer_create (eGPUQueueFamily::gfx, &cmdBufferHandle);


    //main loop
    while (bQuitApp == false)
    {
        mainLoop.stat_onCPUFrameBegin();
        theHub.update (gos::getTimeSinceStart_msec());
        doCPUStuff ();
        mainLoop.stat_onCPUFrameEnd();


        mainLoop.run();

        if (gpu->swapChain_wasRecreated())
            cam.changeAspectRatioPerspectiveFovLH (gpu->swapChain_calcAspectRatio());


        //se il job precedente e' stato presentato, posso schedularne uno nuovo
        gpu::SwapchainImg swapchainImg;
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
