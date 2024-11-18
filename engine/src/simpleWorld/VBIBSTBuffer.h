#ifndef _VBIBSTBuffer_h_
#define _VBIBSTBuffer_h_
#include "ThePipelineEnumAndDefine.h"

/**
 * @brief VBIBSTBuffer
 * Crea un VB,un IB e uno staging buffer nel quale memorizzare
 * delle shape per essere renderizzate
 */
class VBIBSTBuffer
{
public:
            VBIBSTBuffer();
            ~VBIBSTBuffer();

    bool    setup (gos::GPU *gpu, u32 sizeOfAVertex);
    void    unsetup ();

    bool    upload (const gos::Shape *shape, tpp::sBoundShapeInfo *out_info);

private:
    static constexpr u32    VTXBUFFER_MAX_NUM_VTX   = 8*1024*1024;
    static constexpr u32    IDXBUFFER_MAX_NUM_IDX   = VTXBUFFER_MAX_NUM_VTX*3;
    static constexpr u32    STGBUFFER_SIZE          = VTXBUFFER_MAX_NUM_VTX/2;

private:
    gos::GPU                *gpu;
    u32                     sizeOfAVertex;
    u32                     nextFreeVtx;
    u32                     nextFreeIdx;
    GPUVtxBufferHandle      hVtxBuffer;
    GPUIdxBufferHandle      hIdxBuffer;
    GPUStgBufferHandle      hStgBuffer;

};



#endif //__VBIBSTBuffer_h_