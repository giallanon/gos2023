#include "gosProtocolBuffer.h"
#include "../gos.h"

using namespace gos;


//******************************************
ProtocolBuffer::ProtocolBuffer() :
    allocator (NULL)
{
    mem = NULL;
    bFreeMemBlock = 0;
    allocatedSize = 0;
    cursor = 0;
}

//******************************************
void ProtocolBuffer::priv_freeCurBuffer ()
{
    if (bFreeMemBlock)
        GOSFREE(allocator, mem);
}

//******************************************
void ProtocolBuffer::setupWithBase (void *startingBlock, u32 sizeOfStartingBlock, gos::Allocator *backingallocator)
{
    assert (NULL == mem);
    allocator = backingallocator;
    mem = (u8*)startingBlock;
    bFreeMemBlock = 0;
    allocatedSize = sizeOfStartingBlock;
}

//******************************************
void ProtocolBuffer::setup (gos::Allocator *backingallocator, u32 preallocNumBytes)
{
    assert (NULL == mem);
    allocator = backingallocator;

    bFreeMemBlock = 1;

    if (preallocNumBytes)
    {
        mem = GOSALLOCT(u8*,allocator,preallocNumBytes);
        allocatedSize = preallocNumBytes;
    }
    else
    {
        mem = NULL;
        allocatedSize = 0;
    }
}

//******************************************
bool ProtocolBuffer::append (const u8 *src, u32 nBytesTowrite)
{
    if (nBytesTowrite == 0)
        return true;
    if (cursor + nBytesTowrite > allocatedSize)
    {
        if (!growUpTo (cursor + nBytesTowrite))
            return false;
    }
    memcpy (&mem[cursor], src, nBytesTowrite);
    cursor += nBytesTowrite;
    return true;
}

//******************************************
bool ProtocolBuffer::growUpTo (u32 finalSize)
{
    if (allocatedSize >= finalSize)
        return true;
    return growIncremental (finalSize - allocatedSize);
}

//******************************************
bool ProtocolBuffer::growIncremental (u32 howManyBytesToAdd)
{
    if (howManyBytesToAdd == 0)
        return true;
    u8 *newBuffer = GOSALLOCT(u8*, allocator, allocatedSize + howManyBytesToAdd);
    if (NULL == newBuffer)
    {
        DBGBREAK;
        return false;
    }

    if (allocatedSize)
        memcpy (newBuffer, mem, allocatedSize);

    priv_freeCurBuffer();
    mem = newBuffer;
    bFreeMemBlock = 1;
    allocatedSize += howManyBytesToAdd;
    return true;
}

//******************************************
void ProtocolBuffer::shrink (u32 newSize, gos::Allocator *newAllocator)
{
    assert (newSize && newSize <= allocatedSize);

    if (newAllocator == NULL)
        newAllocator = allocator;

    u8	*temp = GOSALLOCT(u8*,newAllocator, newSize);
    memcpy (temp, mem, newSize);

    unsetup ();
    allocatedSize = newSize;
    bFreeMemBlock = 1;
    mem = temp;
    allocator = newAllocator;

    if (cursor >= newSize)
        cursor = newSize;
}

//******************************************
void ProtocolBuffer::removeFirstNByte (u32 nBytesToRemove)
{
    if (nBytesToRemove == 0)
        return;
    if (nBytesToRemove >= allocatedSize)
        nBytesToRemove = allocatedSize;

    const u32 nBytesToMove = allocatedSize - nBytesToRemove;
    if (nBytesToMove)
        memcpy (mem, &mem[nBytesToRemove], nBytesToMove);

    cursor -= nBytesToRemove;
}