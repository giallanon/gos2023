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
    void        virtual_onRun();
    void        virtual_onCleanup();

private:
    struct Vertex 
    {
        gos::vec2f  pos;
        gos::vec3f  colorRGB;
    };

private:    
    static bool recordCommandBuffer (gos::GPU *gpu, 
                                    const GPURenderLayoutHandle &renderLayoutHandle, 
                                    const GPUFrameBufferHandle &frameBufferHandle,
                                    const GPUPipelineHandle &pipelineHandle,
                                    const GPUVtxBufferHandle &vtxBufferHandle,
                                    VkCommandBuffer *out_commandBuffer);

private:
    bool        createVertexBuffer();
    void        moveVertex();

    void        doCPUStuff (gos::TimerFPS &cpuTimer);

    void        mainLoop_3();

private:
    static const u8     NUM_VERTEX = 3;

private:
    Vertex                  vertexList[NUM_VERTEX];
    u64                     nextTimeMoveVtx_msec;
    f32                     direction;
    void                    *ptToMappedMemory;

    GPUVtxBufferHandle      vtxBufferHandle;
    GPUPipelineHandle       pipelineHandle;
    GPUShaderHandle         vtxShaderHandle;
    GPUShaderHandle         fragShaderHandle;
    GPURenderLayoutHandle   renderLayoutHandle;
    GPUFrameBufferHandle    frameBufferHandle;
};


#endif //_VulkanExample2_h_