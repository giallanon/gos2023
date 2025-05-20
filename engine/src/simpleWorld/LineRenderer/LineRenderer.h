#ifndef _LineRenderer_h_
#define _LineRenderer_h_
#include "../ThePipeline.h"

/**
 * @brief LineRenderer
 *  
 */
class LineRenderer
{
public:
            LineRenderer();
            ~LineRenderer();

    bool    setup (ThePipeline *thePipeline);
    bool    recordCommandBuffer (gos::gpu::CmdBufferWriter &cw, gos::geom::Camera3 *cam);

private:
    static constexpr u32    PER_INSTANCE_SSBO__SIZEOF_ONE_ELEMENT       = 16;
    static constexpr u32    PER_INSTANCE_SSBO__NUM_MAX_ELEM             = 1024;

private:
    bool    priv_setupVulkan();
    bool    priv_createPipeline();
    bool    priv_createDescriptor();

private:
    struct sVertex
    {
        gos::vec3f  pos;
    };

    struct sPerInstanceData
    {
        gos::vec4f  pos;
    };

    struct sLineInfo
    {
        f32 width;
    };

private:
    ThePipeline                 *thePipeline;
    gos::GPU                    *gpu;
    gos::Allocator              *localAllocator;
    GPUPipelineHandle           hPipeline;
    GPUShaderHandle             hVtxShader;
    GPUShaderHandle             hFragShader;

    GPUVtxBufferHandle          hVtxBuffer;
    GPUIdxBufferHandle          hIdxBuffer;
    GPUVtxDeclHandle            vtxDeclHandle;
    gos::VtxLayout              vtxLayout; 

    GPUDescrSetLayoutHandle     descr2_layout;
    GPUDescrSetInstanceHandle   descr2_instance;       
    GPUStorageBufferHandle      descr2_ssboHandle;

    u8                          pc_lineInfo;
};



#endif //_LineRenderer_h_
