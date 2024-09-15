#ifndef _Renderer1_h_
#define _Renderer1_h_
#include "gosGPU.h"
#include "../gos/memory/gosAllocatorHeap.h"
#include "../gos/gosHandle.h"
#include "../gosShape/gosShape.h"
#include "../gosGeom/gosGeomCamera3.h"


/**
 * @brief Renderer1
 *  
 */
class Renderer1
{
private:
    typedef gos::AllocatorHeap<gos::AllocPolicy_Track_simple, gos::AllocPolicy_Thread_Unsafe>		LocalAllocator;

public:
    typedef gos::HandleT<16,10,5, 0,1>	hModel;		//2^16=65536 => num totale di oggetti, divisi in chunk da 2^10=1024

public:
            Renderer1();
            ~Renderer1();

    bool    setup (gos::GPU *gpu);
    bool    recordCommandBuffer (gos::gpu::CmdBufferWriter &cw, gos::geom::Camera3 *cam);

    bool    addModelFrom_glTF (const char *filename, hModel *out);
    bool    addModel (const gos::ShapeList &shapeList, hModel *out);
    bool    addInstance (const hModel &hModel, const gos::geom::Pos3 &pos);

private:
    static constexpr u32    VTXBUFFER_MAX_NUM_VTX   = 4096;
    static constexpr u32    IDXBUFFER_MAX_NUM_IDX   = 4096*4;
    static constexpr u32    STGBUFFER_SIZE          = 16*1024;

private:
    struct sVertex
    {
        gos::vec3f  pos;
        gos::vec3f  norm;
        gos::vec2f  tutv0;
    };

    struct sDescrSet0_UBO
    {
        gos::mat4x4f    camVP;
        gos::vec4f      lightDir;
    };

    struct sMaterialData
    {
        gos::vec3f  color;
        u32         textureIndex;
    };

    struct sMaterial
    {
        GPUUniformBufferHandle      hUBO;
        sMaterialData               data;
    };

private:
    bool    priv_setupVulkan();
    bool    priv_createPipeline();
    bool    priv_createVBIB();
    bool    priv_createSfera();
    bool    priv_createCubo();

private:
    gos::GPU                *gpu;
    LocalAllocator          *localAllocator;
    gos::shape::VtxLayout   vtxLayout;

    GPURenderLayoutHandle   hRenderLayout;
    GPUFrameBufferHandle    hFrameBuffer;
    GPUPipelineHandle       hPipeline;
    GPUShaderHandle         hVtxShader;
    GPUShaderHandle         hFragShader;
    GPUVtxBufferHandle      hVtxBuffer;
    GPUIdxBufferHandle      hIdxBuffer;
    GPUStgBufferHandle      hStgBuffer;
    u8                      pc_objWorldPos;


    GPUDescrPoolHandle      hDescrPool;

    GPUDescrSetLayoutHandle     hDescrSetLayout_0;
    GPUDescrSetInstanceHandle   hDescrSetInstance_0;
    sDescrSet0_UBO              descrset0_ubo;
    GPUUniformBufferHandle      hDescrset0_ubo;

    GPUDescrSetLayoutHandle     hDescrSetLayout_1;
    GPUDescrSetInstanceHandle   hDescrSetInstance_1;

    GPUDescrSetLayoutHandle     hDescrSetLayout_2;

    sMaterial                   material1;
    sMaterial                   material2;

    GPUSamplerHandle            hSampler_diffuse;
    GPUTextureHandle            hTex_checker;
    GPUTextureHandle            hTex_stone003;
    gos::shape::Shape           shapeSfera;

};



#endif //_Renderer1_h_
