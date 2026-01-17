#ifndef _SimpleLineRenderer_h_
#define _SimpleLineRenderer_h_
#include "../gosGPU/gosGPU.h"
#include "../gosGeom/gosGeomCamera3.h"

/**
 * @brief SimpleLineRenderer
 */
class SimpleLineRenderer
{
public:
            SimpleLineRenderer ();
            ~SimpleLineRenderer();

    bool    setup(gos::GPU *gpu, GPUDescrPoolHandle &descrPoolHandle);
    void    begin();
    void    setColor (const gos::vec3f &color);
    void    addLine (const gos::vec3f &p1, const gos::vec3f &p2);

    u16     addVtx (const gos::vec3f &p);
    void    line (u16 v0, u16 v1);

    void    end();

    //bool    recordCommandBuffer (gos::gpu::CmdBufferWriter2::BeginRend &cw, GPUStgBufferHandle hStgBuffer, gos::geom::Camera3 &cam);
    bool    recordCommandBuffer (gos::gpu::CmdBufferWriter2 &cw, VkImageView rt, GPUStgBufferHandle hStgBuffer, gos::geom::Camera3 &cam);

private:
    struct sVertex
    {
        gos::vec3f  pos;
        gos::vec3f  col;
    };

    struct sUniformBufferObject 
    {
        gos::mat4x4f camView;
        gos::mat4x4f camProj;
    };

private:
    gos::GPU                    *gpu;
    gos::Allocator              *localAllocator;
    gos::FastArray<sVertex>     vtxList;
    gos::FastArray<u16>         idxList;
    bool                        bNeedUpdate;
    gos::vec3f                  curColor;
    sUniformBufferObject        ubo;

    GPUVtxBufferHandle          hVtxBuffer;
    GPUIdxBufferHandle          hIdxBuffer;

    GPUShaderHandle             hVtxShader;
    GPUShaderHandle             hFragShader;
    GPUPipelineHandle           pipelineHandle;

    GPUUniformBufferHandle      hUBO;
    GPUDescrSetInstanceHandle   hDescrSetInstance;
};



#endif //_SimpleLineRenderer_h_