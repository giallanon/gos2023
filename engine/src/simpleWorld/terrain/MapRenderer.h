#ifndef _MapRenderer_h_
#define _MapRenderer_h_
#include "../ThePipeline.h"
#include "TheMap.h"
#include "MarchingSquare.h"

/**
 * @brief MapRenderer
 *  
 */
class MapRenderer
{
public:
            MapRenderer();
            ~MapRenderer();

    bool    setup (ThePipeline *thePipeline, const char *mapFile);
    bool    recordCommandBuffer (gos::gpu::CmdBufferWriter &cw, gos::geom::Camera3 *cam);


private:
    static constexpr u32    PER_INSTANCE_SSBO__SIZEOF_ONE_ELEMENT       = 32;
    static constexpr u32    PER_INSTANCE_SSBO__NUM_MAX_ELEM             = 32*1024*1024;

private:
    struct sPerInstanceData
    {
        gos::vec4f  worldPosAndScale;
        gos::vec4f  colorAndAO;
    };
   
    struct sDescrPerInstance
    {
        tpp::sDescriptor            descr;
        GPUStorageBufferHandle      ssboHandle;
    };
    
    struct sInfoPerLevel
    {
        tpp::sBoundShapeInfo    bondShape;
        u32                     numFullQuad;
        u32                     indexStartInPerInstanceArray;
    };

private:
    bool    priv_createDescriptorPerInstance();
    bool    priv_createPipeline();
    void    priv_buildALevel (TheMap::LayerView &view, MarchingSquare::VertexList3 &tempVtxList, gos::FastArray<u16> &tempIdxList);
    void    priv_createAShape (MarchingSquare::VertexList3 &tempVtxList, gos::FastArray<u16> &tempIdxList, gos::Shape *out_shape) const;

private:
    ThePipeline                 *thePipeline;
    gos::GPU                    *gpu;
    gos::Allocator              *localAllocator;
    GPUPipelineHandle           hPipeline;
    GPUShaderHandle             hVtxShader;
    GPUShaderHandle             hFragShader;

    sDescrPerInstance               descriptorPerInstance;
    gos::FastArray<sInfoPerLevel>   boundShapePerimetroList;
    tpp::sBoundShapeInfo            boundShapeFullQuad;

    //sPerInstanceData debug_perInstanceData[32];
};



#endif //_MapRenderer_h_
