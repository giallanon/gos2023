#ifndef _ThePipelineEnumAndDefine_h_
#define _ThePipelineEnumAndDefine_h_
#include "gosGPU.h"
#include "../gos/memory/gosAllocatorHeap.h"
#include "../gosGeom/gosGeomCamera3.h"
#include "../gosShape/gosShape.h"


class VBIBSTBuffer;

namespace tpp
{
    struct sDescriptor
    {
        GPUDescrSetLayoutHandle     layout;
        GPUDescrSetInstanceHandle   instance;
    };

    struct sBoundShapeInfo
    {
        GPUVtxBufferHandle      hVtxBuffer;
        GPUIdxBufferHandle      hIdxBuffer;
        u32                     startVtx;
        u32                     startIdx;
        u32                     numIdx;        
    };    

} //namespace tpp


#endif //_ThePipelineEnumAndDefine_h_
