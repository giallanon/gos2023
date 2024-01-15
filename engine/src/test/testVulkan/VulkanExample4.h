#ifndef _VulkanExample4_h_
#define _VulkanExample4_h_
#include "VulkanApp.h"


/************************************
 *  VulkanExample4
 */
class VulkanExample4 : public VulkanApp
{
public:
    
                VulkanExample4();

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

    struct sUniformBufferObject 
    {
        gos::mat4x4f world;
        gos::mat4x4f view;
        gos::mat4x4f proj;
    };


private:
    static bool createDescriptorSetLayout (gos::GPU *gpu, GPUDescrLayoutHandle *out);


private:
    bool        createVertexIndexStageBuffer();
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
    sUniformBufferObject    ubo;

    u64                     nextTimeRotate_msec;
    f32                     rotation_grad;
    

    GPUVtxBufferHandle      vtxBufferHandle;
    GPUIdxBufferHandle      idxBufferHandle;
    GPUStgBufferHandle      stgBufferHandle;

    GPUPipelineHandle       pipelineHandle;
    GPUShaderHandle         vtxShaderHandle;
    GPUShaderHandle         fragShaderHandle;
    GPURenderLayoutHandle   renderLayoutHandle;
    GPUFrameBufferHandle    frameBufferHandle;

    GPUDescrLayoutHandle    descrLayoutHandle;
    GPUUniformBufferHandle  uboHandle;
};


#endif //_VulkanExample4_h_