#ifndef _VulkanExample2_h_
#define _VulkanExample2_h_
#include "VulkanApp.h"


/************************************
 *  VulkanExample2
 */
class VulkanExample2 : public VulkanApp
{
public:
    
                VulkanExample2();

    bool        virtual_onInit ();
    void        virtual_explain();
    void        virtual_onRun();
    void        virtual_onCleanup();

private:
    struct Vertex 
    {
        gos::vec2f  pos;
        gos::vec3f  colorRGB;
    };

private:
    void        moveVertex();
    void        doCPUStuff ();
    bool        recordCommandBuffer (GPUCmdBufferHandle &cmdBufferHandle, gos::gpu::AcquiredSwapchainImg &swapChainImage);

private:
    static const u8     NUM_VERTEX = 3;

private:
    Vertex                  vertexList[NUM_VERTEX];
    u64                     nextTimeMoveVtx_msec;
    f32                     direction;

    GPUVtxBufferHandle          vtxBufferHandle;
    GPUShaderHandle             vtxShaderHandle;
    GPUShaderHandle             fragShaderHandle;
    gos::gpu::pipe2::Pipeline   pipeline;
};


#endif //_VulkanExample2_h_