#include "gosEngine_dynTextureArray.h"


using namespace gos;
using namespace gos::engine;


//*************************************
DynamicTextureArray::DynamicTextureArray()
{
    num_max_texture = 0;
}

//*************************************
void DynamicTextureArray::setup (gos::Allocator *allocator, u32 num_max_textureIN)
{
    num_max_texture = num_max_textureIN;
    bitmask.setup (allocator, num_max_texture);
    hashMap.setup (allocator, num_max_texture);

    bitmask.zero();
}

//*************************************
void DynamicTextureArray::unsetup()
{
    bitmask.unsetup(hashMap.getAllocator());
    hashMap.unsetup();
}

//*************************************
u32 DynamicTextureArray::addIfNotExitst (GPUTextureHandle texHandle, bool *out_canBeNULL_wasNew)
{
    HashMap<GPUTextureHandle, u32>::Position pos;
    u32 index;
    if (hashMap.findWithPos (texHandle, &index, &pos))
    {
        if (NULL != out_canBeNULL_wasNew)
            *out_canBeNULL_wasNew= false;
        return index;
    }

    //cerco il primo index libero
    if (bitmask.findAndSetFirstFreeBit(&index))
    {
        hashMap.insertInPosition (pos, index);

        if (NULL != out_canBeNULL_wasNew)
            *out_canBeNULL_wasNew = true;
        return index;
    }

    DBGBREAK;
    if (NULL != out_canBeNULL_wasNew)
        *out_canBeNULL_wasNew= false;
    return u32MAX;
}

//*************************************
void DynamicTextureArray::remove (GPUTextureHandle texHandle)
{
    u32 index;
    if (find(texHandle, &index))
    {
        hashMap.remove (texHandle);
        bitmask.clear (index);
    }
}


//*************************************
bool DynamicTextureArray::find (GPUTextureHandle texHandle, u32 *out_index) const
{
    return hashMap.find (texHandle, out_index);
}

