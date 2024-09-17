#ifndef _Scene_h_
#define _Scene_h_
#include "gosGPU.h"
#include "../gos/memory/gosAllocatorHeap.h"
#include "../gos/gosHandle.h"
#include "../gosShape/gosShape.h"
#include "../gosGeom/gosGeomCamera3.h"



typedef gos::HandleT<16,10,4, 0,2>	ModelH;		//2^16=65536 => num totale di oggetti, divisi in chunk da 2^10=1024

struct Shape
{
    GPUVtxBufferHandle  hVB;
    GPUIDxBufferHandle  hIB;
    u32                 fistVtx;
    u32                 firstIdx;
    u16                 numVtx
    u16                 numTris;
};

struct Model
{
    ShapeList<u32>   shapeList;          //lista di handle a shape
    gos::mat4x4f     localTransfor;      //ogni shape ha la sua local transform rispetto alla posizione di root
};

/**
 * @brief Scene
 *  
 */
class Scene
{
public:
            Scene();
            ~Scene();

    //============= gestione modelli
    //Model e' una collezione di shape
    bool    model_addFrom_glTF (const char *filename, ModelH *out_handle);
    bool    model_add (const gos::ShapeList &shapeList, ModelH *out_handle);


    bool    shape_add (const gos::Shape *shape, ShapeH *out_handle);

    //============= gestione instance
    bool    instance_add (const ModelH &hModel, const MaterialH &hMaterial, const gos::geom::Pos3 &worldPos);



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


    MaterialList<MaterialH, Material>   materialList;
};



#endif //_Scene_h_
