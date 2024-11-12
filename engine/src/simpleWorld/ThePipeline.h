#ifndef _ThePipeline_h_
#define _ThePipeline_h_
#include "gosGPU.h"
#include "../gos/memory/gosAllocatorHeap.h"
#include "DynamicTextureArray.h"

/**
 * @brief ThePipeline
 *  
 */
class ThePipeline
{
private:
    typedef gos::AllocatorHeap<gos::AllocPolicy_Track_simple, gos::AllocPolicy_Thread_Unsafe>		LocalAllocator;

public:
            ThePipeline();
            ~ThePipeline();

    bool    setup (gos::GPU *gpu);
    void    unsetup();

public:
    static constexpr u32    NUM_MAX_TEXTURE                         = 1024;


public:
    gos::GPU                    *gpu;
    LocalAllocator              *localAllocator;
    GPURenderLayoutHandle       hRenderLayout;
    GPUFrameBufferHandle        hFrameBuffer;
    
    GPUSamplerHandle            hSampler_diffuse;

    GPUDescrPoolHandle          hDescrPool;

    DynamicTextureArray         textureList;


};



#endif //_ThePipeline_h_
