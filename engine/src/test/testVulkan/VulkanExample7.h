#ifndef _VulkanExample7_h_
#define _VulkanExample7_h_
#include "VulkanApp.h"


/************************************
 *  VulkanExample7
 * 
 *  Un semplice triangolo!
 */
class VulkanExample7 : public VulkanApp
{
public:
    
                VulkanExample7()                    { }

    bool        virtual_onInit ();
    void        virtual_onRun();
    void        virtual_onCleanup();
    void        virtual_explain();

private:    
    bool        recordCommandBuffer (GPUCmdBufferHandle &cmdBufferHandle);

private:
    GPURenderTargetHandle   rt1;
    GPUPipelineHandle       pipelineHandle;
    GPUShaderHandle         vtxShaderHandle;
    GPUShaderHandle         fragShaderHandle;
    GPURenderLayoutHandle   renderLayoutHandle;
    GPUFrameBufferHandle    frameBufferHandle;
};


#endif //_VulkanExample7_h_