#include "gosEngine_idxBufferMan.h"

using namespace gos;
using namespace gos::engine;

//**************************************** 
IdxBufferMan::IdxBufferMan()
{
	allocator = NULL;
	gpu = NULL;
}

//**************************************** 
void IdxBufferMan::setup (gos::Allocator *allocatorIN, GPU *gpuIN)
{
	allocator = allocatorIN;
	gpu = gpuIN;
	list.setup (allocator, 32);
}

//**************************************** 
void IdxBufferMan::unsetup ()
{
	const u32 n = list.getNElem();
	for (u32 i = 0; i < n; i++)
	{
		gpu->deleteResource (list[i].handle);
		GOSDELETE(allocator, list[i].tracker);
	}
	list.unsetup();

	gpu = NULL;
	allocator = NULL;
}

//**************************************** 
bool IdxBufferMan::reserve (u32 sizeInByte, u32 *out_offset, u32 *out_size, GPUIdxBufferHandle *out_handle)
{
	gos::FreespaceTracker::AllocInfo info;
	u32 choosenIndex = u32MAX;

	const u32 n = list.getNElem();
	for (u32 i = 0; i < n; i++)
	{
		if (list[i].tracker->alloc (sizeInByte, &info))
		{
			choosenIndex = i;
			break;
		}
	}

	//devo allocare un nuovo buffer
	if (u32MAX == choosenIndex)
	{
		priv_allocNewBuffer();
		if (list[n].tracker->alloc (sizeInByte, &info))
			choosenIndex = n;
	}

	if (u32MAX == choosenIndex)
	{
		DBGBREAK;
		return false;
	}

	*out_handle = list(choosenIndex).handle;
	*out_offset = info.offset;
	*out_size = info.size;
	return true;
}

//**************************************** 
void IdxBufferMan::release (GPUIdxBufferHandle handle, u32 offset, u32 size)
{
	const u32 n = list.getNElem();
	for (u32 i = 0; i < n; i++)
	{
		if (list(i).handle == handle)
		{
			gos::FreespaceTracker::AllocInfo info;
			info.offset = offset;
			info.size = size;
			list[i].tracker->free (info);
			return;
		}
	}

	DBGBREAK;
}

//**************************************** 
bool IdxBufferMan::priv_allocNewBuffer ()
{
	static constexpr u32 BUFFER_SIZE = 64 * 1024 * 1024;

	sElem elem;
	if (!gpu->indexBuffer_create (BUFFER_SIZE, eMemAccessMode::onGPU, &elem.handle))
	{
		DBGBREAK;
		return false;
	}

	elem.tracker = GOSNEW(allocator, FreespaceTracker)();
	elem.tracker->setup (BUFFER_SIZE, 32 * 3);
	list.append(elem);
	return true;

}