#include "gosBufferLinear.h"

using namespace gos;

//******************************************
void BufferLinear::priv_FreeCurBuffer ()
{
	if (bFreeMemBlock)
		GOSFREE(allocator, mem);
}

//******************************************
void BufferLinear::setupWithBase (void *startingBlock, u32 sizeOfStartingBlock, Allocator *backingallocator)
{
	assert (NULL == mem);
	allocator = backingallocator;
	mem = (u8*)startingBlock;
	bFreeMemBlock = 0;
	allocatedSize = sizeOfStartingBlock;
}

//******************************************
void BufferLinear::setup (Allocator *backingallocator, u32 preallocNumBytes)
{
	assert (NULL == mem);
	allocator = backingallocator;
	
	bFreeMemBlock = 1;

	if (preallocNumBytes)
	{
		mem = (u8*)GOSALLOC(allocator, preallocNumBytes);
		allocatedSize = preallocNumBytes;
	}
	else
	{
		mem = NULL;
		allocatedSize = 0;
	}
}

//******************************************
bool BufferLinear::read  (void *dest, u32 offset, u32 nBytesToread) const
{
	if (nBytesToread == 0)
		return true;
	if (offset + nBytesToread > allocatedSize)
		return false;
	memcpy (dest, &mem[offset], nBytesToread);
	return true;
}

//******************************************
bool BufferLinear::write  (const void *src, u32 offset, u32 nBytesTowrite, bool bCangrow)
{
	if (nBytesTowrite == 0)
		return true;
	if (offset + nBytesTowrite > allocatedSize)
	{
		if (!bCangrow)
			return false;
		if (!growUpTo (offset + nBytesTowrite))
			return false;
	}
	memcpy (&mem[offset], src, nBytesTowrite);
	return true;
}

//******************************************
bool BufferLinear::growUpTo (u32 finalSize)
{
	if (allocatedSize >= finalSize)
		return true;
	return growIncremental (finalSize - allocatedSize);
}

//******************************************
bool BufferLinear::growIncremental (u32 howManyBytesToAdd)
{
	if (howManyBytesToAdd == 0)
		return true;
	u8 *newBuffer = (u8*)GOSALLOC(allocator, allocatedSize + howManyBytesToAdd);
	if (NULL == newBuffer)
	{
		DBGBREAK;
		return false;
	}

	if (allocatedSize)
		memcpy (newBuffer, mem, allocatedSize);

	priv_FreeCurBuffer();
	mem = newBuffer;
	bFreeMemBlock = 1;
	allocatedSize += howManyBytesToAdd;
	return true;
}

//******************************************
bool BufferLinear::copyFrom (const BufferLinear &src, u32 srcOffset, u32 nBytesToCopy, u32 dstOffset, bool bCangrow)
{
	if (nBytesToCopy == 0)
		return true;
	assert (srcOffset < src.getTotalSizeAllocated());
	assert (srcOffset + nBytesToCopy <= src.getTotalSizeAllocated());

	assert (dstOffset < allocatedSize);

	if (dstOffset + nBytesToCopy > allocatedSize)
	{
		if (!bCangrow)
			return false;
		if (!growUpTo (dstOffset + nBytesToCopy))
			return false;
	}

	memcpy (&mem[dstOffset], &src.mem[srcOffset], nBytesToCopy);
	return true;
}

//******************************************
void BufferLinear::shrink (u32 newSize, Allocator *newAllocator)
{
	assert (newSize && newSize <= allocatedSize);

	if (newAllocator == NULL)
		newAllocator = allocator;

	u8	*temp = (u8*)GOSALLOC(newAllocator, newSize);
	memcpy (temp, mem, newSize);

	unsetup ();
	allocatedSize = newSize;
	bFreeMemBlock = 1;
	mem = temp;
	allocator = newAllocator;
}