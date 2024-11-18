#ifndef _Renderer1_h_
#define _Renderer1_h_
#include "ThePipeline.h"
#include "../gos/gosHandle.h"
#include "../gosShape/gosShape.h"
#include "BitmaskedFixedArray.h"
#include "Model.h"

/**
 * @brief Renderer1
 *  
 */
class Renderer1
{
public:
            Renderer1();
            ~Renderer1();

    bool    setup (ThePipeline *thePipeline);
    bool    recordCommandBuffer (gos::gpu::CmdBufferWriter &cw, gos::geom::Camera3 *cam);

    //============= gestione materiali
    bool    material_create (const GPUTextureHandle &hDiffuseTex, const gos::vec3f &DiffuseCol, u16 *out_index);

    //============= gestione modelli
    bool    shape_add (const tpp::sBoundShapeInfo &shape, u16 *out_index);

    //============= gestione instance
    bool    instance_add (u16 indexOf_shape, u16 indexOf_material, const gos::geom::Pos3 &worldPos);


private:
    static constexpr u32    NUM_MAX_MATERIAL                        = 1024;
    static constexpr u32    SIZEOF_ONE_ELEMENT_IN_MATERIAL_SSBO     = 64;

private:
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

    struct sDescrMaterial
    {
        tpp::sDescriptor            descr;
        GPUStorageBufferHandle      ssboHandle;
    };

private:
    bool    priv_setupVulkan();
    bool    priv_createPipeline();
    bool    priv_createDescriptorMaterial();

private:
    ThePipeline                 *thePipeline;
    gos::GPU                    *gpu;
    gos::Allocator              *localAllocator;
    GPUPipelineHandle           hPipeline;
    GPUShaderHandle             hVtxShader;
    GPUShaderHandle             hFragShader;

    //descr set 2
    sDescrMaterial              descriptorMaterial;

    //push constant
    u8                          pc_objWorldPos;


    BitmaskedFixedArray<Material>   materialList;
    BitmaskedFixedArray<tpp::sBoundShapeInfo>  shapeList;
    gos::FastArray<sInstance>       instanceList;
};



#endif //_Renderer1_h_
