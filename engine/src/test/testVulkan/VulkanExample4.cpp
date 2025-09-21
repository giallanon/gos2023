#include "VulkanExample4.h"
#include "../gosShape/gosShapePrefabs.h"
#include "../gosImage/gosImageBuilder.h"

using namespace gos;


//************************************
VulkanExample4::VulkanExample4()
{
    anim.reset();
}

//************************************
void VulkanExample4::virtual_explain()
{
    gos::logger::log ("Esperimenti con Uniform buffer, gestione dell'input da KB/mouse, movimento camera in 3d\n");
    gos::logger::log (eTextColor::white, "TAB = toggle mouse mode\n");
}


//************************************
void VulkanExample4::virtual_onCleanup() 
{
    shape::shapeFree (gos::getSysHeapAllocator(), &myShape);
    gpu->deleteResource (idxBufferHandle);
    gpu->deleteResource (stgBufferHandle);
    gpu->deleteResource (vtxBufferHandle);

    gpu->deleteResource (vtxShaderHandle);
    gpu->deleteResource (fragShaderHandle);
    gpu->deleteResource (pipelineHandle);

    gpu->deleteResource (uboHandle);
    gpu->deleteResource (descrSetInstancerHandle);
    gpu->deleteResource (descrPoolHandle);
    gpu->deleteResource (texHandle);

}    


//************************************
bool VulkanExample4::virtual_onInit ()
{
    //creo un cubo
    {
        gos::VtxLayout vtxLayout;
        gos::shape::VtxLayoutWriter vtxLayoutW (&vtxLayout);
        vtxLayoutW.begin()
            .addPos3 (offsetof(Vertex,pos))
            .addColor3 (offsetof(Vertex,colorRGB))
            .addNorm3 (offsetof(Vertex,normal))
            .addTexCoord (offsetof(Vertex,tutv0))
        .end();

        myShape.reset();
        gos::shape::buildCube24 (vec3f(0,0,0), vec3f(4,2,3), vtxLayout, gos::getSysHeapAllocator(), &myShape);
        Vertex *vertexList = reinterpret_cast<Vertex*>(myShape.vtxBuffer);

        //front face (green)
        vertexList[0].colorRGB.set (0, 1, 0);
        vertexList[1].colorRGB = vertexList[2].colorRGB = vertexList[3].colorRGB = vertexList[0].colorRGB;

        //back face (red)
        vertexList[4].colorRGB.set (1, 0, 0);
        vertexList[5].colorRGB = vertexList[6].colorRGB = vertexList[7].colorRGB = vertexList[4].colorRGB;

        ///right face (blue)
        vertexList[8].colorRGB.set (0, 0, 1);
        vertexList[9].colorRGB = vertexList[10].colorRGB = vertexList[11].colorRGB = vertexList[8].colorRGB;

        //left face (white)
        vertexList[12].colorRGB.set (1, 1, 1);
        vertexList[13].colorRGB = vertexList[14].colorRGB = vertexList[15].colorRGB = vertexList[12].colorRGB;

        //top face (yellow)
        vertexList[16].colorRGB.set (1, 1, 0);
        vertexList[17].colorRGB = vertexList[18].colorRGB = vertexList[19].colorRGB = vertexList[16].colorRGB;

        //bottom face (azzurro)
        vertexList[20].colorRGB.set (0, 1, 1);
        vertexList[21].colorRGB = vertexList[22].colorRGB = vertexList[23].colorRGB = vertexList[20].colorRGB;
    }


    //carico una texture
    {
        gos::Image im;
/*        gos::image::Builder builder;

        builder.begin (gos::getScrapAllocator(), &im)
            .beginTexture (eImageFormat::U8_RGBA_sRGB, 512, 512, 2)
            .setMipMapDataFromFile (0, "texture/faccia512x512.jpg")
            .setMipMapDataFromFile (1, "texture/faccia256x256.png")
            .endTexture()
        .end();
        if (builder.anyError())
        {
            gos::logger::err ("VulkanApp::init() => can't build image'\n");
            return false;
        }
        image::save (im, "texture/faccia_2mipmap.gosimage");
*/
        image::load (gos::getScrapAllocator(), "texture/faccia_2mipmap.gosimage", &im);
        gpu->texture_create2D (&im, 0, &texHandle);
        image::free (gos::getScrapAllocator(), im);
    }

    //creo il sampler
    gpu->sampler_create (gpu::SamplerDesc(), &samplerHandle);
    


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



    //carico gli shader
    fs::addAlias ("@shader", "shader/example4", eAliasPathMode::relativeToAppFolder);
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
            .add (0, offsetof(Vertex, pos), eDataFormat::_3f32)        //position
            .add (1, offsetof(Vertex, colorRGB), eDataFormat::_3f32)   //color
            .add (2, offsetof(Vertex, normal), eDataFormat::_3f32)    //color
            .add (3, offsetof(Vertex, tutv0), eDataFormat::_2f32)     //texCoord
            .endVtxStream()
        .descriptorset_add()
            .add (0, eGPUDescriptrorType::UNIFORM_BUFFER, 1, eGPUDescriptrorUsageFlag::vtx_shader)
            .add (1, eGPUDescriptrorType::COMBINED_IMAGE_SAMPLER, 1, eGPUDescriptrorUsageFlag::pxl_shader)
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
        .addPool_combinedTextureAndSampler()
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


    return true;
}    


//************************************
bool VulkanExample4::createVertexIndexStageBuffer()
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

//**********************************
void VulkanExample4::virtual_onInputEvent (u32 actionID, i16 value, const gos::input::MouseStatus &mouseStatus, const gos::input::sButtonModifier &btnModifier)
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
void VulkanExample4::doCPUStuff ()
{
    handleInput();

    //prepare frame
    {
        const u64 timeNow_msec = gos::getTimeSinceStart_msec();
        if (timeNow_msec >= anim.nextTimeRotate_msec)
        {
            mat4x4f matT;
            mat4x4f matR;

            anim.nextTimeRotate_msec = timeNow_msec + 16;
            anim.rotation_grad+=1.0f;
            //anim.zPos += anim.zInc;
            if (anim.zPos >= 10 || anim.zPos < 0)
                anim.zInc = -anim.zInc;
            
            matR.buildRotationAboutY (math::gradToRad(anim.rotation_grad));
            matT.buildTranslation (0,0,anim.zPos);
            ubo.objWorld = matT * matR;
//            ubo.world.identity();


            //camera
            ubo.camView = cam.getMatV();
            ubo.camProj = cam.getMatP();
            //ubo.lightDir.set (-1, -0.3f, 0, 0);
            ubo.lightDir.set (-1, -0, 0, 0);
            ubo.lightDir.normalize();


            /*vec4f vIN[4];
            vIN[0].set (0,0,0,1);
            vIN[1].set (0,0,1,1);
            vIN[2].set (0,0,10,1);
            vIN[3].set (0,0,100,1);
            vec4f vOUT[4];
            for (u32 i = 0; i < 4; i++)
            {
                vOUT[i] = math::vecTransform (ubo.proj, vIN[i]);
            }
            vOUT[0].w = 1;
*/

        }
        gpu->writeAndSync (uboHandle, 0, &ubo, sizeof(sUniformBufferObject));
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


    //gestione del movimento
    const u64 timeNow_msec = gos::getTimeSinceStart_msec();
    movement.update(timeNow_msec);
    cam.markUpdated();
}

//************************************
bool VulkanExample4::recordCommandBuffer (GPUCmdBufferHandle &cmdBufferHandle, gpu::SwapchainImg &swapChainImage)
{
    //aggiorno UBO
    static u8 bind_once = 0;
    if (0 == bind_once)
    {
        bind_once = 1;

        gos::gpu::DescrSetInstanceWriter descrWriter;
        descrWriter.begin (gpu, descrSetInstancerHandle)
            .bindUniformBuffer (0, uboHandle)
            .bindCombinedTextureAndSampler (1, texHandle, samplerHandle)
            .end();
    }

        
    GPUDepthStencilHandle hZB = gpu->depthStencil_getDefault();
    gos::gpu::pipe2::CmdBufferWriter2 cw;
    cw
        .begin (gpu, cmdBufferHandle)
        .setViewport (gpu->viewport_getDefault())
        .imageTransition (swapChainImage.image, eImageLayout::undefined, eImageLayout::color_attachment_optimal)
        .imageTransition (hZB, eImageLayout::undefined, eImageLayout::depth_attachment_optimal)
        .beginRender()
            .withRenderArea (gpu->swapChain_getWidth(), gpu->swapChain_getHeight())
            .withRT (swapChainImage.imageView, eAttachmentLoadOp::clear, eAttachmentStoreOp::dont_care, gos::ColorHDR(0, 0.1f, 0.3f))
            .withZB (hZB, eAttachmentLoadOp::clear, eAttachmentStoreOp::dont_care, 1.0f, 0)
            .bindPipeline (pipelineHandle)
            .bindDescriptorSet(descrSetInstancerHandle, 0)
            .bindVtxBuffer(vtxBufferHandle)
            .bindIdxBufferU16(idxBufferHandle)
            .drawIndexed (myShape.numIdx, 1, 0, 0, 0)            
            .endRender()
        .imageTransition (swapChainImage.image, eImageLayout::color_attachment_optimal, eImageLayout::presentation)
        .end();

        
    return true;
}



/************************************
 * renderizza inviando command buffer a GPU e poi aspettando che questa
 * abbia finito il suo lavoro
 */
void VulkanExample4::virtual_onRun()
{
    cam.setPerspectiveFovLH(gpu->swapChain_calcAspectRatio(),  math::gradToRad(45), 0.1f, 50.0f);
    cam.pos.identity();
    cam.pos.warp (0, 0, -19);
    cam.pos.lookAt (vec3f(0,0,0));
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
        doCPUStuff();
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