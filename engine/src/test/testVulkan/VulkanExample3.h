#ifndef _VulkanExample3_h_
#define _VulkanExample3_h_
#include "VulkanApp.h"


/************************************
 *  VulkanExample3
 */
class VulkanExample3 : public VulkanApp
{
public:
    
                VulkanExample3();

    bool        virtual_onInit ();
    void        virtual_explain();
    void        virtual_onRun();
    void        virtual_onCleanup();

private:
    struct Vertex 
    {
        gos::vec2f  pos;
        gos::vec3f  colorRGB;

        void set (f32 x, f32 y, f32 r, f32 g, f32 b)    { pos.set(x,y); colorRGB.set(r,g,b); }
    };


private:
    bool        createVertexIndexStageBuffer();
    void        moveVertex();
    bool        recordCommandBuffer (VkCommandBuffer *out_commandBuffer);
    bool        copyIntoVtxBuffer();
    void        doCPUStuff ();
    void        mainLoop();


private:
    static const u8     NUM_VERTEX = 4;
    static const u8     NUM_INDEX = 6;

private:
    Vertex                  vertexList[NUM_VERTEX];
    u16                     indexList[NUM_INDEX];

    u64                     nextTimeMoveVtx_msec;
    f32                     direction;
    void                    *ptToMappedStagingBuffer;

    GPUVtxBufferHandle      vtxBufferHandle;
    GPUIdxBufferHandle      idxBufferHandle;
    GPUStgBufferHandle      stgBufferHandle;

    GPUPipelineHandle       pipelineHandle;
    GPUShaderHandle         vtxShaderHandle;
    GPUShaderHandle         fragShaderHandle;
    GPURenderLayoutHandle   renderLayoutHandle;
    GPUFrameBufferHandle    frameBufferHandle;
};


#endif //_VulkanExample3_h_