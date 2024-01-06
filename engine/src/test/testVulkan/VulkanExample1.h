#ifndef _VulkanExample1_h_
#define _VulkanExample1_h_
#include "VulkanApp.h"


/************************************
 *  VulkanExample1
 * 
 *  Un semplice triangolo!
 */
class VulkanExample1 : public VulkanApp
{
public:
    
                VulkanExample1()                    { }

    bool        virtual_onInit ();
    void        virtual_onRun();
    void        virtual_onCleanup();
    void        virtual_explain();

private:    
    static bool recordCommandBuffer (gos::GPU *gpu, 
                                    const GPURenderLayoutHandle &renderLayoutHandle, 
                                    const GPUFrameBufferHandle &frameBufferHandle,
                                    const GPUPipelineHandle &pipelineHandle,
                                    VkCommandBuffer *out_commandBuffer);

private:
    GPUPipelineHandle       pipelineHandle;
    GPUShaderHandle         vtxShaderHandle;
    GPUShaderHandle         fragShaderHandle;
    GPURenderLayoutHandle   renderLayoutHandle;
    GPUFrameBufferHandle    frameBufferHandle;
};


#endif //_VulkanExample1_h_