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
    bool        recordCommandBuffer (GPUCmdBufferHandle &cmdBufferHandle, gos::gpu::AcquiredSwapchainImg &swapChainImage);

private:
    GPUShaderHandle             vtxShaderHandle;
    GPUShaderHandle             fragShaderHandle;
    gos::gpu::pipe2::Pipeline   pipeline;
};


#endif //_VulkanExample1_h_