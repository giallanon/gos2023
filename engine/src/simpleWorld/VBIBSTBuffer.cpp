#include "VBIBSTBuffer.h"
#include "../gos/gosUtils.h"

using namespace gos;



//********************************
VBIBSTBuffer::VBIBSTBuffer()
{
    gpu = NULL;
}

//********************************
VBIBSTBuffer::~VBIBSTBuffer()
{
    unsetup();
}

//********************************
void VBIBSTBuffer::unsetup ()
{
    if (NULL == gpu)
        return;
    
    gpu->deleteResource (hVtxBuffer);
    gpu->deleteResource (hIdxBuffer);
    gpu->deleteResource (hStgBuffer);

    gpu = NULL;    
}

//********************************
bool VBIBSTBuffer::setup (gos::GPU *gpuIN, u32 sizeOfAVertexIN)
{
    gpu = gpuIN;
    sizeOfAVertex = sizeOfAVertexIN;
    nextFreeVtx = nextFreeIdx = 0;

    //vtx buffer (stream 0)
    if (!gpu->vertexBuffer_create (sizeOfAVertex * VTXBUFFER_MAX_NUM_VTX, eVIBufferMode::onGPU, &hVtxBuffer))
    {
        gos::logger::err ("VBIBSTBuffer::setup() => gpu->vertexBuffer_create() failed\n");
        return false;
    }

    //index buffer
    if (!gpu->indexBuffer_create (sizeof(u16) * IDXBUFFER_MAX_NUM_IDX, eVIBufferMode::onGPU, &hIdxBuffer))
    {
        gos::logger::err ("VBIBSTBuffer::setup() => gpu->indexBuffer_create() failed\n");
        return false;
    }

    //Creo anche uno staging buffer
    if (!gpu->stagingBuffer_create (STGBUFFER_SIZE, &hStgBuffer))
    {
        gos::logger::err ("VBIBSTBuffer::setup() => gpu->stagingBuffer_create() failed\n");
        return false;
    }

    return true;
}

//********************************
bool VBIBSTBuffer::upload (const gos::Shape *shape, tpp::sBoundShapeInfo *out_info)
{
    assert (NULL != gpu);
    assert (NULL != shape);
    assert (NULL != out_info);
    assert (shape->numVtx > 0);
    assert (shape->numIdx > 0);
    assert (sizeOfAVertex == gos::shape::calcSizeOfAVertex (shape->vtxLayout));


    if (nextFreeVtx + shape->numVtx > VTXBUFFER_MAX_NUM_VTX)
    {
        gos::logger::err ("VBIBSTBuffer::upload() => not enough space in VB. Current count=%d, max=%d, requested=%d\n", nextFreeVtx, VTXBUFFER_MAX_NUM_VTX, shape->numVtx);
        return false;
    }

    if (nextFreeIdx + shape->numIdx > IDXBUFFER_MAX_NUM_IDX)
    {
        gos::logger::err ("VBIBSTBuffer::upload() => not enough space in IB. Current count=%d, max=%d, requested=%d\n", nextFreeIdx, IDXBUFFER_MAX_NUM_IDX, shape->numIdx);
        return false;
    }


    out_info->numIdx = shape->numIdx;

    //copio i Vtx in vtxBuffer e idx in idxBuffer tramite lo staging buffer
    if (!gpu->stagingBuffer_uploadToGPUBuffer (hStgBuffer, shape->vtxBuffer, hVtxBuffer, nextFreeVtx * sizeOfAVertex, shape->numVtx * sizeOfAVertex))
    {
        gos::logger::err ("VBIBSTBuffer::upload() => can't upload to VtxBuffer\n");
        return false;
    }
    out_info->hVtxBuffer = hVtxBuffer;
    out_info->startVtx = nextFreeVtx;
    nextFreeVtx += shape->numVtx;


    if (!gpu->stagingBuffer_uploadToGPUBuffer (hStgBuffer, shape->idxBuffer, hIdxBuffer, nextFreeIdx * sizeof(u16), shape->numIdx * sizeof(u16)))
    {
        gos::logger::err ("VBIBSTBuffer::upload() => can't upload to IdxBuffer\n");
        return false;
    }
    out_info->hIdxBuffer = hIdxBuffer;
    out_info->startIdx = nextFreeIdx;
    nextFreeIdx += shape->numIdx;


    return true;
}