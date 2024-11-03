#include "DynamicTextureArray.h"
#include "../gos/gosUtils.h"

using namespace gos;



//********************************
DynamicTextureArray::DynamicTextureArray()
{
    localAllocator = NULL;
    numMaxElem = 0;
    gpu = NULL;
}

//********************************
void DynamicTextureArray::unsetup ()
{
    if (NULL == localAllocator)
        return;


    u32 iter;
    list.toStart(&iter);

    const GPUTextureHandle *texHandle;
    while (list.next (&iter, &texHandle))
    {
        GPUTextureHandle h = *texHandle;
        gpu->deleteResource (h);
    }

    list.unsetup();
    hashMap.unsetup();

    localAllocator = NULL;    
}

//********************************
bool DynamicTextureArray::setup (gos::Allocator *allocator, gos::GPU *gpuIN, u16 numMaxElements)
{
    assert (NULL == localAllocator);
    localAllocator = allocator;
    gpu = gpuIN;
    
    numMaxElem = numMaxElements;
    list.setup (localAllocator, numMaxElem);
    hashMap.setup (localAllocator, numMaxElem);
    return true;
}

//********************************
u16 DynamicTextureArray::addIfNotExists (const GPUTextureHandle &hTexture, u16 *out_index)
{
    gos::HashMap<u32,u16>::Position pos;
    if (hashMap.findWithPos (hTexture.viewAsU32(), out_index, &pos))
    {
        //la texture esisteva gia', ritorno 2
        return 2;
    }

    if (list.add (hTexture, out_index))
    {
        //nuovo elemento in array, ritorno 1
        hashMap.insertInPosition (pos, *out_index);
        return 1;
    }

    DBGBREAK;
    return 0;
}

//********************************
void DynamicTextureArray::remove (u16 index, bool bAlsoDeleteTexture)
{
    const GPUTextureHandle *texHandle;
    if (!list.get (index, &texHandle))
        return;

    if (bAlsoDeleteTexture)
    {
        GPUTextureHandle h = *texHandle;
        gpu->deleteResource (h);
    }

    list.remove(index);
    hashMap.remove (index);
}

//********************************
bool DynamicTextureArray::getInfo (u16 index, const GPUTextureHandle *out_hTexture) const
{
    return list.get (index, &out_hTexture);
}