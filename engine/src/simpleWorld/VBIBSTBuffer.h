#ifndef _VBIBSTBuffer_h_
#define _VBIBSTBuffer_h_
#include "gosGPU.h"
#include "../gosShape/gosShape.h"

/**
 * @brief VBIBSTBuffer
 * Crea un VB,un IB e uno staging buffer nel quale memorizzare
 * delle shape per essere renderizzate
 */
class VBIBSTBuffer
{
public:
    struct sUploadInfo
    {
        GPUVtxBufferHandle      hVtxBuffer;
        GPUIdxBufferHandle      hIdxBuffer;
        u32                     startVtx;
        u32                     startIdx;
        u32                     numIdx;        
    };

public:
            VBIBSTBuffer();
            ~VBIBSTBuffer();

    bool    setup (gos::GPU *gpu, u32 sizeOfAVertex);
    void    unsetup ();

    bool    upload (const gos::Shape *shape, sUploadInfo *out_info);

private:
    static constexpr u32    VTXBUFFER_MAX_NUM_VTX   = 1024*1024;
    static constexpr u32    IDXBUFFER_MAX_NUM_IDX   = VTXBUFFER_MAX_NUM_VTX*3;
    static constexpr u32    STGBUFFER_SIZE          = 1024*1024;

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