#include "VKExample1.h"
#include "dataTypes/gosTimer.h"

using namespace gos;


//************************************
void GLFW_key_callback (GLFWwindow* window, int key, UNUSED_PARAM(int scancode), int action, int mods)
{
    VulkanExample1 *app = reinterpret_cast<VulkanExample1*>(glfwGetWindowUserPointer(window));
    if (key == GLFW_KEY_ENTER && action == GLFW_RELEASE)
    {
        if ((mods & GLFW_MOD_ALT) != 0)
            app->toggleFullscreen();    //ALT + ENTER
    }
}




//************************************
VulkanExample1::VulkanExample1()
{
    gpu = NULL;
    vkRenderPassHandle = VK_NULL_HANDLE; 

    for (u8 i=0;i<SWAPCHAIN_NUM_MAX_IMAGES;i++)
        frameBufferHandleList[i] = VK_NULL_HANDLE;
}

//************************************
void VulkanExample1::cleanup() 
{
    //pipe
    pipeline.cleanUp();

    //shader
    gpu->shader_delete (vtxShaderHandle);
    gpu->shader_delete (fragShaderHandle);

    //render pass
    if (VK_NULL_HANDLE != vkRenderPassHandle)
    {
        vkDestroyRenderPass (gpu->REMOVE_getVkDevice(), vkRenderPassHandle, nullptr);
        vkRenderPassHandle = VK_NULL_HANDLE;
    }

    //frame buffer
    priv_destroyFrameBuffers (gpu);
}    

//************************************
void VulkanExample1::priv_destroyFrameBuffers (gos::GPU *gpuIN)
{
    for (u8 i=0;i<SWAPCHAIN_NUM_MAX_IMAGES;i++)
    {
        if (VK_NULL_HANDLE != frameBufferHandleList[i])
        {
            vkDestroyFramebuffer(gpuIN->REMOVE_getVkDevice(), frameBufferHandleList[i], nullptr);
            frameBufferHandleList[i] = VK_NULL_HANDLE;
        }
    }
}


//************************************
bool VulkanExample1::init(gos::GPU *gpuIN)
{
    gpu = gpuIN;

    glfwSetWindowUserPointer (gpu->getWindow(), this);
    glfwSetKeyCallback (gpu->getWindow(), GLFW_key_callback);

    //creo il render pass
    if (!priv_createRenderPass(gpu))
    {
        gos::logger::err ("can't create render pass\n");
        return false;
    }

    //frame buffers
    priv_recreateFrameBuffers(gpu, vkRenderPassHandle);


    //carico gli shader
    if (!gpu->vtxshader_createFromFile ("shader/shader1.vert.spv", "main", &vtxShaderHandle))
    {
        gos::logger::err ("can't create vert shader\n");
        return false;
    }
    if (!gpu->fragshader_createFromFile ("shader/shader1.frag.spv", "main", &fragShaderHandle))
    {
        gos::logger::err ("can't create frag shader\n");
        return false;
    }

    //creo la pipeline
    bool b = pipeline.begin (gpu)
        .addShader (vtxShaderHandle)
        .addShader (fragShaderHandle)
        .setVtxDecl (GPUVtxDeclHandle::INVALID())
        .end (vkRenderPassHandle);
        
    if (!b)
    {
        gos::logger::err ("can't create pipeline\n");
        return false;
    }

    //esempio di vtxDecl
    /*
    GPUVtxDeclHandle h;
    gpu->vtxDecl_createNew(&h)
        .addStream(eVtxStreamInputRate::perVertex)
        .addDescriptor (0, 0, eDataFormat::_3f32)        //position
        .addDescriptor (12, 1, eDataFormat::_3u32)      //color
        .end();
    assert (h.isValid());
    gpu->vtxDecl_delete (h);
    */

    return true;
}    

/************************************
 * input:   swapchain (per conoscere il formato dell'immagine)
 */
bool VulkanExample1::priv_createRenderPass (gos::GPU *gpuIN)
{
    gos::logger::log ("VulkanExample1::priv_createRenderPass: ");
    bool ret = false;

    //color buffer attachment
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = gpuIN->swapChain_getImageFormat();
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;       //pulisci il buffer all'inizio del render pass
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;     //mantiene le info scritte in questo buffer alla fine del pass
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;        //stencil (dont care)
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;              //non mi interessa lo stato del buffer all'inizio, tanto lo pulisco
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;          //il formato finale deve essere prensentabile

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    //in questo esempio, voglio 1 solo "subpass" che opera sul color buffer
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = NULL;

    //dipendenza di questo subpass da altri subpass (in questo caso non ce ne sono)
    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;    //indica il subpassa before questo
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;    //aspetto che swapchain abbia letto tutto quanto
    dependency.srcAccessMask = 0;    
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    //creazione del render pass
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;    

    VkResult result = vkCreateRenderPass (gpuIN->REMOVE_getVkDevice(), &renderPassInfo, nullptr, &vkRenderPassHandle);
    if (VK_SUCCESS == result)
    {
        ret = true;
        gos::logger::log (eTextColor::green, "OK\n");
    }
    else
    {
        gos::logger::err ("vkCreateRenderPass() error: %s\n", string_VkResult(result)); 
    }  
        
    return ret;
}

/*************************************
 * input:  swapchain e vkRenderPass
 * output: frameBufferHandleList[]   => un frame buffer per ogni image della swapchain
 */
bool VulkanExample1::priv_recreateFrameBuffers (gos::GPU *gpuIN, const VkRenderPass vkRenderPassHandleIN)
{
    bool ret = true;
    gos::logger::log ("VulkanExample1::priv_recreateFrameBuffers()\n");
    gos::logger::incIndent();

    priv_destroyFrameBuffers(gpuIN);

    for (u8 i = 0; i < gpuIN->swapChain_getImageCount(); i++) 
    {
        VkImageView imageViewList[2] = { gpuIN->swapChain_getImageViewHandle(i) , 0};

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = vkRenderPassHandleIN;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = imageViewList;
        framebufferInfo.width = gpuIN->swapChain_getWidth();
        framebufferInfo.height = gpuIN->swapChain_getHeight();
        framebufferInfo.layers = 1;

        const VkResult result = vkCreateFramebuffer(gpuIN->REMOVE_getVkDevice(), &framebufferInfo, nullptr, &frameBufferHandleList[i]);
        if (VK_SUCCESS != result)
        {
            gos::logger::err ("vkCreateFramebuffer() => %s\n", string_VkResult(result));
            ret = false;
            break;
        }
    }

    gos::logger::decIndent();
    return ret;
}

//************************************
bool VulkanExample1::recordCommandBuffer (gos::GPU *gpuIN, const VkRenderPass &vkRenderPassHandle, const VkFramebuffer &vkFrameBufferHandle, 
                                            const VkPipeline &vkPipelineHandle, VkCommandBuffer *out_commandBuffer)
{
    assert (out_commandBuffer);

    //uso la viewport di default di GPU che e' sempre grande tanto quanto la main window
    const gos::gpu::Viewport *viewport = gpuIN->viewport_getDefault();

    VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0; // Optional
        beginInfo.pInheritanceInfo = nullptr; // Optional

    VkResult result = vkBeginCommandBuffer (*out_commandBuffer, &beginInfo);
    if (VK_SUCCESS != result)
    {
        gos::logger::err ("recordCommandBuffer() => vkBeginCommandBuffer() => %s\n", string_VkResult(result));
        return false;
    }


    VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = vkRenderPassHandle;
        renderPassInfo.framebuffer = vkFrameBufferHandle;
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = { viewport->getW(), viewport->getH() };
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;    

    vkCmdBeginRenderPass (*out_commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline (*out_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkPipelineHandle);

    //setto la viewport
    VkViewport vkViewport {0.0f, 0.0f, viewport->getW_f32(), viewport->getH_f32(), 0.0f, 1.0f };
    vkCmdSetViewport(*out_commandBuffer, 0, 1, &vkViewport);

    VkRect2D scissor { 0, 0, viewport->getW(), viewport->getH() };
    vkCmdSetScissor (*out_commandBuffer, 0, 1, &scissor);

    //draw primitive
    vkCmdDraw(*out_commandBuffer, 3, 1, 0, 0);

    //fine del render pass
    vkCmdEndRenderPass (*out_commandBuffer);

    //fine del command buffer
    result = vkEndCommandBuffer (*out_commandBuffer);
    if (VK_SUCCESS != result)
    {
        gos::logger::err ("recordCommandBuffer() => vkEndCommandBuffer() => %s\n", string_VkResult(result));
        return false;
    }    
    
    return true;
}

/************************************
 * renderizza inviando command buffer a GPU e poi aspettando che questa
 * abbia finito il suo lavoro
 */
void VulkanExample1::mainLoop_waitEveryFrame()
{
    VkCommandBuffer     vkCommandBuffer;
    gpu->createCommandBuffer (&vkCommandBuffer);

    VkSemaphore         imageAvailableSemaphore;
    VkSemaphore         renderFinishedSemaphore;
    gpu->semaphore_create (&imageAvailableSemaphore);
    gpu->semaphore_create (&renderFinishedSemaphore);

    VkFence             inFlightFence;
    gpu->fence_create (true, &inFlightFence);


    VkResult            result;
    gos::TimerFPS       fpsTimer;
    gos::Timer          cpuWaitTimer;
    gos::Timer          frameTimer;
    gos::Timer          acquireImageTimer;
    bool                bNeedToRecreateSwapChain = false;
    while (!glfwWindowShouldClose (gpu->getWindow()))
    {
        if (bNeedToRecreateSwapChain)
        {
            bNeedToRecreateSwapChain = false;
            gpu->swapChain_recreate();
            priv_recreateFrameBuffers (gpu, this->vkRenderPassHandle);
        }

//printf ("frame begin\n");
        frameTimer.start();
        fpsTimer.onFrameBegin();

        glfwPollEvents();

        //draw frames
        cpuWaitTimer.start();
            gpu->fence_wait (&inFlightFence);
//printf ("  CPU waited GPU fence for %ld us\n", cpuWaitTimer.elapsed_usec());

        //recupero una immagine dalla swap chain, attendo per sempre e indico [imageAvailableSemaphore] come
        //semaforo che GPU deve segnalare quando questa operazione e' ok
        acquireImageTimer.start();
            u32 imageIndex;
            //result = vkAcquireNextImageKHR (vulkan.dev, vulkan.swapChainInfo.vkSwapChain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
            result = gpu->swapChain_acquireNextImage (&imageIndex, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE);
            if (VK_SUCCESS != result)
            {
                if (VK_ERROR_OUT_OF_DATE_KHR == result)
                {
                    gos::logger::log (eTextColor::yellow, "vkAcquireNextImageKHR() => VK_ERROR_OUT_OF_DATE_KHR\n");
                    bNeedToRecreateSwapChain = true;
                    continue;
                }
                else if (VK_SUBOPTIMAL_KHR == result)
                {
                    gos::logger::log (eTextColor::yellow, "vkAcquireNextImageKHR() => VK_SUBOPTIMAL_KHR\n");
                    bNeedToRecreateSwapChain = true;
                }
                else
                {
                    //Errore generico
                    gos::logger::err ("vkAcquireNextImageKHR() => %s\n", string_VkResult(result));
                    continue;
                }
            }
            gpu->fence_reset (&inFlightFence);
//printf ("  CPU waited vkAcquireNextImageKHR %ld us\n", acquireImageTimer.elapsed_usec());
        
        //command buffer che opera su [imageIndex]
        recordCommandBuffer(gpu, vkRenderPassHandle, frameBufferHandleList[imageIndex], pipeline.getVkHandle(), &vkCommandBuffer);

        //submit
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore semaphoresToBeWaitedBeforeStarting[] = { imageAvailableSemaphore };
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = semaphoresToBeWaitedBeforeStarting;
        submitInfo.pWaitDstStageMask = waitStages;

        submitInfo.commandBufferCount = 1;        
        submitInfo.pCommandBuffers = &vkCommandBuffer;

        //semaforo che GPU segnalera' al termine dell'esecuzione del command buffer
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &renderFinishedSemaphore;

        //submitto il batch a GPU e indico che deve segnalare [inFlightFence] quando ha finito 
        result = vkQueueSubmit (gpu->REMOVE_getGfxQHandle(), 1, &submitInfo, inFlightFence);
        if (VK_SUCCESS != result)
        {
            gos::logger::err ("vkQueueSubmit() => %s\n", string_VkResult(result));
        }


        //presentazione
        result = gpu->swapChain_present (&renderFinishedSemaphore, 1, imageIndex);
        if (VK_ERROR_OUT_OF_DATE_KHR == result || VK_SUBOPTIMAL_KHR == result)
            bNeedToRecreateSwapChain = true;

//printf ("  total frame time: %ldus\n", frameTimer.elapsed_usec());


        if (fpsTimer.onFrameEnd())
        {
            const float usec = fpsTimer.getAvgFrameTime_usec();
            const float msec = usec/ 1000.0f;
            printf ("Avg frame time: %.2fms [%.2fus] [fps: %.01f]\n", msec, usec, fpsTimer.getAvgFPS());
        }
    }

    //aspetto che GPU abbia finito tutto cio' che ha in coda
    gpu->waitIdle();

    gpu->semaphore_destroy (imageAvailableSemaphore);
    gpu->semaphore_destroy (renderFinishedSemaphore);
    gpu->fence_destroy (inFlightFence);
}


//************************************
void VulkanExample1::mainLoop()
{
    mainLoop_waitEveryFrame();
}
