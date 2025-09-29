#include "gosGPUVBIBManager.h"
#include "../gosGPU.h"

using namespace gos;
using namespace gos::gpu;

//************************************************
VBIBManager::VBIBManager()
{
    gpu = NULL;
    sizeOfAVertex = 0;
}

//************************************************
VBIBManager::~VBIBManager()
{
    unsetup();
}

//************************************************
void VBIBManager::unsetup()
{
    if (NULL == gpu)
        return;

    gpu->deleteResource (hStageBuffer);

    for (u32 i=0; i<vbList.getNElem(); i++)
        gpu->deleteResource (vbList[i].handle);

    for (u32 i=0; i<ibList.getNElem(); i++)
        gpu->deleteResource (ibList[i].handle);

    gpu = NULL;
}


//************************************************
void VBIBManager::setup (gos::Allocator *allocator, gos::GPU *gpuIN, u32 sizeOfAVertexIN, u32 sizeInByteOfAVtxBuffer, u32 sizeInByteOfAIdxBuffer)
{
    gpu = gpuIN;
    vbList.setup (allocator, 16);
    ibList.setup (allocator, 16);
    sizeOfAVertex = sizeOfAVertexIN;
    numVtxPerBuffer = sizeInByteOfAVtxBuffer / sizeOfAVertexIN;
    numIdxPerBuffer = sizeInByteOfAIdxBuffer / sizeof(u16);

    const u32 size1 = sizeOfAVertex * numVtxPerBuffer;
    const u32 size2 = numIdxPerBuffer * sizeof(u16);
    if (size1 > size2)
        gpu->stagingBuffer_create (size1, &hStageBuffer);
    else
        gpu->stagingBuffer_create (size2, &hStageBuffer);

}


//************************************************
void VBIBManager::add (const void *vtx, u32 numVtx, const u16 *idx, u32 numIndex, 
                        GPUVtxBufferHandle *out_vbHandle, GPUIdxBufferHandle *out_ibHandle, u32 *out_vtxStart, u32 *out_idxStart)
{
    if (numVtx > 0)
    {
        assert (NULL != vtx);
        assert (NULL != out_vbHandle);
        assert (NULL != out_vtxStart);
        assert (numVtx <= numVtxPerBuffer);

        u32 index = u32MAX;
        for (u32 i=0; i<vbList.getNElem(); i++)
        {
            if (vbList(i).numFree >= numVtx)
            {
                index = i;
                break;
            }
        }

        if (u32MAX == index)
        {
            index = vbList.getNElem();
            vbList[index].numFree = numVtxPerBuffer;
            gpu->vertexBuffer_create (numVtxPerBuffer * sizeOfAVertex, eMemAccessMode::onGPU, &vbList[index].handle);
        }

        *out_vtxStart = numVtxPerBuffer - vbList(index).numFree;
        *out_vbHandle = vbList[index].handle;
        priv_copyVB (vtx, numVtx, vbList(index).handle, (*out_vtxStart));
        vbList[index].numFree -= numVtx;
    }


    if (numIndex > 0)
    {
        assert (NULL != vtx);
        assert (NULL != out_ibHandle);
        assert (NULL != out_idxStart);
        assert (numIndex <= numIdxPerBuffer);

        u32 index = u32MAX;
        for (u32 i=0; i<ibList.getNElem(); i++)
        {
            if (ibList(i).numFree >= numIndex)
            {
                index = i;
                break;
            }
        }

        if (u32MAX == index)
        {
            index = ibList.getNElem();
            ibList[index].numFree = numIdxPerBuffer;
            gpu->indexBuffer_create (numIdxPerBuffer * sizeof(u16), eMemAccessMode::onGPU, &ibList[index].handle);
        }

        *out_idxStart = numIdxPerBuffer - ibList(index).numFree;
        *out_ibHandle = ibList[index].handle;
        priv_copyIB (idx, numIndex, ibList(index).handle, (*out_idxStart));
        ibList[index].numFree -= numIndex;
    }
}

//************************************************
void VBIBManager::priv_copyVB (const void *vtx, u32 numVtx, const GPUVtxBufferHandle &handleDST, u32 vtxStart) const
{
    assert (NULL != vtx);
    assert (numVtx > 0);
    gpu->stagingBuffer_uploadToGPUBuffer (hStageBuffer, vtx, handleDST, vtxStart * sizeOfAVertex, numVtx * sizeOfAVertex);
}

//************************************************
void VBIBManager::priv_copyIB (const void *idx, u32 numIdx, const GPUIdxBufferHandle &handleDST, u32 idxStart) const
{
    assert (NULL != idx);
    assert (numIdx > 0);
    gpu->stagingBuffer_uploadToGPUBuffer (hStageBuffer, idx, handleDST, idxStart * sizeof(u16), numIdx * sizeof(u16));
}