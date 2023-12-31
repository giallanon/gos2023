#include "VKExample1.h"
#include "dataTypes/gosTimer.h"

using namespace gos;


//************************************
void GLFW_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
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
}

//************************************
void VulkanExample1::cleanup() 
{
    pipe1.destroy(gpu);
}    

//************************************
bool VulkanExample1::init(gos::GPU *gpuIN)
{
    gpu = gpuIN;

    glfwSetWindowUserPointer (gpu->getWindow(), this);
    glfwSetKeyCallback (gpu->getWindow(), GLFW_key_callback);

    //creo una pipeline / render pass
    if (!pipe1.create (gpu, GPUVtxDeclHandle::INVALID(), eDrawPrimitive::trisList))
    {
        gos::logger::err ("can't creating pipeline1\n");
        return false;
    }
    

    GPUVtxDeclHandle h;
    gpu->vtxDecl_createNew(&h)
        .addStream(eVtxStreamInputRate::perVertex)
        .addDescriptor (0, 0, eDataFormat::_3f32)        //position
        .addDescriptor (12, 1, eDataFormat::_3u32)      //color
        .end();
    assert (h.isValid());

    gpu->vtxDecl_delete (h);

    return true;
}    

//************************************
bool VulkanExample1::recordCommandBuffer (u32 imageIndex, VkCommandBuffer &in_out_commandBuffer)
{
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0; // Optional
    beginInfo.pInheritanceInfo = nullptr; // Optional

    VkResult result = vkBeginCommandBuffer(in_out_commandBuffer, &beginInfo);
    if (VK_SUCCESS != result)
    {
        gos::logger::err ("recordCommandBuffer() => vkBeginCommandBuffer() => %s\n", string_VkResult(result));
        return false;
    }


    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = pipe1.getRenderPassHandle();
    renderPassInfo.framebuffer = pipe1.getFrameBufferHandle(imageIndex);
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = gpu->swapChain_getImageExten2D();

    VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;    

    vkCmdBeginRenderPass(in_out_commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(in_out_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->pipe1.pipeHandle);


    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(gpu->swapChain_getWidth());
    viewport.height = static_cast<float>(gpu->swapChain_getHeight());
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(in_out_commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = gpu->swapChain_getImageExten2D();
    vkCmdSetScissor(in_out_commandBuffer, 0, 1, &scissor);


    vkCmdDraw(in_out_commandBuffer, 3, 1, 0, 0);

    vkCmdEndRenderPass (in_out_commandBuffer);

    result = vkEndCommandBuffer (in_out_commandBuffer);
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
            pipe1.recreateFrameBuffers(gpu);
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
        recordCommandBuffer(imageIndex, vkCommandBuffer);

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
