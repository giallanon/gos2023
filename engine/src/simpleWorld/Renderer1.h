#ifndef _Renderer1_h_
#define _Renderer1_h_
#include "gosGPU.h"
#include "../gos/memory/gosAllocatorHeap.h"
#include "../gos/gosHandle.h"
#include "../gosShape/gosShape.h"
#include "../gosGeom/gosGeomCamera3.h"
#include "DynamicTextureArray.h"
#include "BitmaskedFixedArray.h"
#include "VBIBSTBuffer.h"
#include "Model.h"

/**
 * @brief Renderer1
 *  
 */
class Renderer1
{
private:
    typedef gos::AllocatorHeap<gos::AllocPolicy_Track_simple, gos::AllocPolicy_Thread_Unsafe>		LocalAllocator;

public:
            Renderer1();
            ~Renderer1();

    bool    setup (gos::GPU *gpu);
    bool    recordCommandBuffer (gos::gpu::CmdBufferWriter &cw, gos::geom::Camera3 *cam);

    //============= gestione materiali
    bool    material_create (const GPUTextureHandle &hDiffuseTex, const gos::vec3f &DiffuseCol, u16 *out_index);

    //============= gestione modelli
    bool    shape_add (const VBIBSTBuffer::sUploadInfo &shape, u16 *out_index);

    //============= gestione instance
    bool    instance_add (u16 indexOf_shape, u16 indexOf_material, const gos::geom::Pos3 &worldPos);


private:
    static constexpr u32    NUM_MAX_TEXTURE                         = 1024;
    static constexpr u32    NUM_MAX_MATERIAL                        = 1024;
    static constexpr u32    SIZEOF_ONE_ELEMENT_IN_MATERIAL_SSBO     = 64;

private:
    struct sDescrSet0_UBO
    {
        gos::mat4x4f    camVP;
        gos::vec4f      lightDir;
    };


    struct Material
    {
        gos::vec4f  colorDiffuse;
        u32         indexOf_texDiffuse;
    };
   
    struct sInstance
    {
        u16     indexOf_material;
        u16     indexOf_shape;
        gos::geom::Pos3 worldPos;
    };
    
private:
    bool    priv_setupVulkan();
    bool    priv_createPipeline();



private:
    gos::GPU                *gpu;
    LocalAllocator          *localAllocator;

    GPURenderLayoutHandle   hRenderLayout;
    GPUFrameBufferHandle    hFrameBuffer;
    GPUPipelineHandle       hPipeline;
    GPUShaderHandle         hVtxShader;
    GPUShaderHandle         hFragShader;
    u8                      pc_objWorldPos;


    GPUDescrPoolHandle          hDescrPool;

    GPUDescrSetLayoutHandle     hDescrSetLayout_0;
    GPUDescrSetInstanceHandle   hDescrSetInstance_0;
    sDescrSet0_UBO              descrset0_ubo;
    GPUUniformBufferHandle      hDescrset0_ubo;

    GPUDescrSetLayoutHandle     hDescrSetLayout_1;
    GPUDescrSetInstanceHandle   hDescrSetInstance_1;

    GPUDescrSetLayoutHandle     hDescrSetLayout_2;
    GPUDescrSetInstanceHandle   hDescrSetInstance_2;
    GPUStorageBufferHandle      hDescrset2_ssbo;

    GPUSamplerHandle            hSampler_diffuse;


    DynamicTextureArray             textureList;
    BitmaskedFixedArray<Material>   materialList;
    BitmaskedFixedArray<VBIBSTBuffer::sUploadInfo>  shapeList;
    gos::FastArray<sInstance>       instanceList;
};



#endif //_Renderer1_h_
