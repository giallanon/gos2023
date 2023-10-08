#include "gosBufferSparse.h"
#include "memory/gosMemory.h"

using namespace gos;


//******************************************
BufferSparse::BufferSparse() : 
	allocator (NULL)
{
	totalAvailableBufferSpace = 0;
	bFreeFirstMemBlock = false;
	bufferList = lastBuffer = NULL;
}

//******************************************
BufferSparse::~BufferSparse()
{
	unsetup();
}

//******************************************
void BufferSparse::unsetup()
{
	if (bufferList && !bFreeFirstMemBlock)
		bufferList = bufferList->next;
	while (bufferList)
	{
		sBuffer *p = bufferList;
		bufferList = bufferList->next;
		GOSFREE(allocator, p);
	}

	allocator = NULL;
	totalAvailableBufferSpace = 0;
	bFreeFirstMemBlock = false;
	bufferList = lastBuffer = NULL;
}

//******************************************
BufferSparse::sBuffer* BufferSparse::priv_setupBufferFromMem (void *mem, u32 sizeOfMem) const
{
	assert (NULL != mem && sizeOfMem >= sizeof(sBuffer));
	
	sBuffer *buf = (sBuffer*)mem;
	buf->sizeInByte = sizeOfMem - sizeof(sBuffer);
	buf->next = NULL;
	return buf;
}

//******************************************
void BufferSparse::setupWithBase (void *startingBlock, u32 sizeOfStartingBlock, Allocator *backingallocator, u32 minSizeOfBlocksWhengrowingIN)
{
	assert (NULL == bufferList && totalAvailableBufferSpace == 0 && minSizeOfBlocksWhengrowingIN > sizeof(sBuffer));
	assert (sizeOfStartingBlock >= sizeof(sBuffer));

	allocator = backingallocator;
	minSizeOfBlocksWhengrowing = minSizeOfBlocksWhengrowingIN;
	bFreeFirstMemBlock = false;
	bufferList = lastBuffer = priv_setupBufferFromMem (startingBlock, sizeOfStartingBlock);
	totalAvailableBufferSpace += bufferList->sizeInByte;
}

//******************************************
void BufferSparse::setup (Allocator *backingallocator, u32 preallocNumBytes, u32 minSizeOfBlocksWhengrowingIN)
{
	assert (NULL == bufferList && totalAvailableBufferSpace == 0 && minSizeOfBlocksWhengrowingIN > sizeof(sBuffer));
	allocator = backingallocator;
	
	minSizeOfBlocksWhengrowing = minSizeOfBlocksWhengrowingIN;
	bFreeFirstMemBlock = true;
	bufferList = lastBuffer = NULL;
	
	if (preallocNumBytes)
		growIncremental (preallocNumBytes);
}

//******************************************
bool BufferSparse::growUpTo (u32 finalSize)
{
	if (totalAvailableBufferSpace >= finalSize)
		return true;
	return growIncremental (finalSize - totalAvailableBufferSpace);
}

//******************************************
bool BufferSparse::growIncremental (u32 howManyBytesToAdd)
{
	if (howManyBytesToAdd == 0)
		return true;
	if (howManyBytesToAdd < minSizeOfBlocksWhengrowing)
		howManyBytesToAdd = minSizeOfBlocksWhengrowing;
	howManyBytesToAdd += sizeof(sBuffer);

	u8 *mem = (u8*)GOSALLOC(allocator, howManyBytesToAdd);
	if (NULL == mem)
	{
		DBGBREAK;
		return false;
	}

	sBuffer *buf = priv_setupBufferFromMem (mem, howManyBytesToAdd);
	totalAvailableBufferSpace += buf->sizeInByte;
	if (NULL == lastBuffer)
		bufferList = lastBuffer = buf;
	else
	{
		lastBuffer->next = buf;
		lastBuffer = buf;
	}
	return true;
}

//******************************************
void* BufferSparse::getBufferAtOffset (u32 offset, u32 nBytesRequested) const
{
	if (nBytesRequested == 0)
		return NULL;
	if (offset + nBytesRequested > getTotalSizeAllocated())
		return NULL;

	sBuffer *p = bufferList;
	while (offset)
	{
		if (offset >= p->sizeInByte)
		{
			offset -= p->sizeInByte;
			p = p->next;
			assert (p);
		}
		else
			break;
	}

	if (offset + nBytesRequested > p->sizeInByte)
	{
		DBGBREAK;
		return NULL;
	}
	return p->getMemPointer(offset);
}

//******************************************
bool BufferSparse::read  (void *destIN, u32 offset, u32 nBytesToread) const
{
	assert (destIN);
	if (nBytesToread == 0)
		return true;
	if (offset + nBytesToread > getTotalSizeAllocated())
		return false;

	sBuffer *p = bufferList;
	while (offset)
	{
		if (offset >= p->sizeInByte)
		{
			offset -= p->sizeInByte;
			p = p->next;
			assert (p);
		}
		else
			break;
	}

	u8 *dest = (u8*)destIN;

	while (nBytesToread)
	{
		u32 n = p->sizeInByte - offset;
		assert (n);
		if (n >= nBytesToread)
		{
			memcpy (dest, p->getMemPointer(offset), nBytesToread);
			return true;
		}

		memcpy (dest, p->getMemPointer(offset), n);
		dest += n;
		nBytesToread -= n;
		offset = 0;
		p = p->next;
		assert (p);
	}
	return false;
}

//******************************************
bool BufferSparse::write  (const void *srcIN, u32 offset, u32 nBytesTowrite, bool bCangrow)
{
	if (nBytesTowrite == 0)
		return true;
	if (offset + nBytesTowrite > getTotalSizeAllocated())
	{
		if (!bCangrow)
			return false;
		if (!growUpTo (offset + nBytesTowrite))
			return false;
	}

	sBuffer *p = bufferList;
	while (offset)
	{
		if (offset >= p->sizeInByte)
		{
			offset -= p->sizeInByte;
			p = p->next;
			assert (p);
		}
		else
			break;
	}

	u8 *src = (u8*)srcIN;

	while (nBytesTowrite)
	{
		u32 n = p->sizeInByte - offset;
		assert (n);
		if (n >= nBytesTowrite)
		{
			memcpy (p->getMemPointer(offset), src, nBytesTowrite);
			return true;
		}

		memcpy (p->getMemPointer(offset), src, n);
		src += n;
		nBytesTowrite -= n;
		offset = 0;
		p = p->next;
		assert (p);
	}
	return false;
}