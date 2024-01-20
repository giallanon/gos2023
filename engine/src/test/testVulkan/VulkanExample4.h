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
        gos::vec3f  pos;
        gos::vec3f  colorRGB;

        void set (f32 x, f32 y, f32 z, f32 r, f32 g, f32 b)    { pos.set(x,y,z); colorRGB.set(r,g,b); }
    };

    struct sUniformBufferObject 
    {
        //glm::mat4 world;
        gos::mat4x4f world;
        gos::mat4x4f view;
        gos::mat4x4f proj;
    };


    struct sAnimation
    {
        u64 nextTimeRotate_msec;
        f32 rotation_grad;
        f32 zPos;
        f32 zInc;

        void reset () { nextTimeRotate_msec = 0; rotation_grad = 0; zPos = 0; zInc = 0.1f; }
    };

private:
    bool        createVertexIndexStageBuffer();
    bool        recordCommandBuffer (VkCommandBuffer *out_commandBuffer);
    bool        copyIntoVtxBuffer();
    void        doCPUStuff ();
    void        mainLoop();


private:
    static const u8     NUM_FACES = 5;
    static const u8     NUM_VERTEX = 4*NUM_FACES;
    static const u8     NUM_INDEX = 6*NUM_FACES;

private:
    Vertex                  vertexList[NUM_VERTEX];
    u16                     indexList[NUM_INDEX];
    sUniformBufferObject    ubo;
    sAnimation              anim;
    

    GPUVtxBufferHandle      vtxBufferHandle;
    GPUIdxBufferHandle      idxBufferHandle;
    GPUStgBufferHandle      stgBufferHandle;

    GPUPipelineHandle       pipelineHandle;
    GPUShaderHandle         vtxShaderHandle;
    GPUShaderHandle         fragShaderHandle;
    GPURenderLayoutHandle   renderLayoutHandle;
    GPUFrameBufferHandle    frameBufferHandle;

    GPUDescrPoolHandle      descrPoolHandle;
    GPUDescrSetLayoutHandle descrSetLayoutHandle;
    GPUDescrSetInstancerHandle descrSetInstancerHandle;
    GPUUniformBufferHandle  uboHandle;
};


#endif //_VulkanExample4_h_