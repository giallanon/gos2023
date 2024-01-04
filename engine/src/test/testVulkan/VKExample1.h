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
                                    const GPURenderLayoutHandle &renderLayoutHandle, 
                                    const VkFramebuffer &vkFrameBufferHandle,
                                    const GPUPipelineHandle &pipelineHandle,
                                    VkCommandBuffer *out_commandBuffer);


private:
    bool        priv_recreateFrameBuffers (gos::GPU *gpu, const GPURenderLayoutHandle &renderLayoutHandle);
    void        priv_destroyFrameBuffers (gos::GPU *gpu);
    void        mainLoop_waitEveryFrame ();
    
    
//    bool priv_createRenderPass (gos::GPU *gpu);

private:
    gos::GPU                *gpu;
    GPUPipelineHandle       pipelineHandle;
    GPUShaderHandle         vtxShaderHandle;
    GPUShaderHandle         fragShaderHandle;
    GPURenderLayoutHandle   renderLayoutHandle;
    
    VkFramebuffer           frameBufferHandleList[SWAPCHAIN_NUM_MAX_IMAGES];
};


#endif //_VKExample1_h_