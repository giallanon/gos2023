#ifndef _VKExample1_h_
#define _VKExample1_h_
#include "gosGPU.h"


/************************************
 *  VulkanExample
 */
class VulkanExample1
{
public:
    
                VulkanExample1();

    bool        init (gos::GPU *gpu);
    void        mainLoop();
    void        cleanup();

    void        toggleFullscreen()                          { gpu->toggleFullscreen(); }


private:    
    static bool recordCommandBuffer (gos::GPU *gpu, 
                                    const VkRenderPass &vkRenderPassHandle, 
                                    const VkFramebuffer &vkFrameBufferHandle,
                                    const VkPipeline &vkPipelineHandle,
                                    VkCommandBuffer *out_commandBuffer);

private:
    bool        priv_createRenderPass (gos::GPU *gpu);
    bool        priv_recreateFrameBuffers (gos::GPU *gpu, const VkRenderPass vkRenderPassHandle);
    void        priv_destroyFrameBuffers (gos::GPU *gpu);
    void        mainLoop_waitEveryFrame ();

private:
    gos::GPU            *gpu;
    gos::gpu::Pipeline  pipeline;
    GPUShaderHandle     vtxShaderHandle;
    GPUShaderHandle     fragShaderHandle;
    VkRenderPass        vkRenderPassHandle;
    VkFramebuffer       frameBufferHandleList[SWAPCHAIN_NUM_MAX_IMAGES];
};


#endif //_VKExample1_h_