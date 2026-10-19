#include "MemManager.h"
#include "../gos/gosBit.h"

using namespace gos;
using namespace gos::res2;


//***********************************
void MemManager::setup (gos::Allocator *allocator)
{
	assert (NULL == localAllocator);
	localAllocator = allocator;

	blob = GOSALLOCT(u8*, localAllocator, BLOB__SIZE);
	tracker.setup (BLOB__SIZE, SLOT_SIZE, localAllocator);
}

//***********************************
void MemManager::unsetup()
{
	if (NULL == localAllocator)
		return;

	assert (0 == tracker.get_num_allocated_bytes());
	GOSFREE(localAllocator, blob);
	tracker.unsetup();
	localAllocator = NULL;
}

//***********************************
ResDescr* MemManager::priv__reserve (eResType res_type, u32 size_to_alloc, ResHandle *out_handle)
{
	assert (NULL != out_handle);
	out_handle->set_invalid();

	//alloco memoria
	FreespaceTracker::AllocInfo alloc_info;
	if (!tracker.alloc (size_to_alloc, &alloc_info))
	{
		DBGBREAK;
		return NULL;
	}

	//creo l'handle
	const u32 start_slot = alloc_info.offset / SLOT_SIZE;
	const u32 num_slot_needed = alloc_info.size / SLOT_SIZE;
	
	assert (start_slot * SLOT_SIZE == alloc_info.offset);
	assert (num_slot_needed * SLOT_SIZE == alloc_info.size);
	
	out_handle->make_handle (guard++, start_slot, num_slot_needed, (u8)res_type);
	
	//ritorno il descrittore
	ResDescr *ret = reinterpret_cast<ResDescr*>( &blob[alloc_info.offset] );
	ret->reset();
	ret->res_handle._handle = out_handle->_handle;
	ret->refCount = 1;
	return ret;
}


//***********************************
void* MemManager::priv__is_valid_handle (const ResHandle handle, FreespaceTracker::AllocInfo *out_alloc_info) const
{
	assert (handle.is_valid());
	assert (NULL != out_alloc_info);
	
	out_alloc_info->offset = handle.get_start_slot() * SLOT_SIZE;
	out_alloc_info->size = handle.get_num_slot() * SLOT_SIZE;
	assert (out_alloc_info->offset < BLOB__SIZE);

	void *ret = &blob[out_alloc_info->offset];
	const u32 *blob = reinterpret_cast<const u32*>( ret );
	if (blob[0] != handle._handle)
	{
		DBGBREAK;
		return NULL;
	}
	return ret;
}

//***********************************
void MemManager::release (ResHandle &handle)
{
	if (handle.is_invalid())
		return;

	FreespaceTracker::AllocInfo alloc_info;
	void *pt = priv__is_valid_handle (handle, &alloc_info);
	if (NULL != pt)
	{
		tracker.free(alloc_info);
		u32 *p32 = (u32*)pt;
		p32[0] = u32MAX;
	}

	handle.set_invalid();
}

//***********************************
const ResDescr*	MemManager::query (ResHandle res_handle) const
{
	ResDescr *descr = priv__get(res_handle);
	if (NULL == descr)
		return NULL;
	return (const ResDescr*)descr;
}

//***********************************
ResDescr* MemManager::priv__get (ResHandle res_handle) const
{
	if (res_handle.is_invalid())
	{
		DBGBREAK;
		return NULL;
	}

	FreespaceTracker::AllocInfo alloc_info;
	void *pt = priv__is_valid_handle (res_handle, &alloc_info);
	if (NULL == pt)
	{
		DBGBREAK;
		return NULL;
	}
	return (ResDescr*)pt;
}

