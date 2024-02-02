#ifndef _VulkanExample5_h_
#define _VulkanExample5_h_
#include "VulkanApp.h"


/************************************
 *  VulkanExample5
 */
class VulkanExample5 : public VulkanApp
{
public:
    
                VulkanExample5();

    bool        virtual_onInit ();
    void        virtual_explain();
    void        virtual_onRun();
    void        virtual_onCleanup();

private:
    static const u32 MAX_VTX_IN_VTX_BUFFER  = 1024;
    static const u32 MAX_IDX_IN_IDX_BUFFER  = 8192;

    static const u16 GRID_HALF_SIZE = 8;
    static const f32 GRID_SCALA = 1.0f;
    static const u16 GRID_NUM_VTX_PER_ROW = (GRID_HALF_SIZE*2) +1;

private:
    struct Vertex 
    {
        gos::vec3f  pos;

        void set (f32 x, f32 y)    { pos.set(x,y,0); }
    };

    struct Quad
    {
        u16 idx[4];
    };

    struct sUniformBufferObject 
    {
        gos::mat4x4f world;
        gos::mat4x4f view;
        gos::mat4x4f proj;
    };


private:
    bool        recordCommandBuffer (GPUCmdBufferHandle &cmdBufferHandle);
    void        doCPUStuff ();
    void        mainLoop();
    void        priv_buildGrid();


private:
    gos::Allocator          *localAllocator;
    gos::FastArray<Vertex>  vtxList;
    gos::FastArray<Quad>    quadList;
    sUniformBufferObject    ubo;
    
    GPUVtxBufferHandle      vtxBufferHandle;
    GPUIdxBufferHandle      idxBufferHandle;
    GPURenderLayoutHandle   renderLayoutHandle;
    GPUFrameBufferHandle    frameBufferHandle;
    GPUShaderHandle         vtxShaderHandle;
    GPUShaderHandle         fragShaderHandle;
    GPUDescrSetLayoutHandle descrSetLayoutHandle;
    GPUPipelineHandle       pipelineHandle;
    GPUUniformBufferHandle  uboHandle;
    GPUDescrPoolHandle      descrPoolHandle;
    GPUDescrSetInstancerHandle descrSetInstancerHandle;
};


#endif //_VulkanExample5_h_