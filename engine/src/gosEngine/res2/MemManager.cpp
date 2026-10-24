#include "MemManager.h"
#include "../gos/gosBit.h"

using namespace gos;
using namespace gos::res2;


//***********************************
void MemManager::setup (gos::Allocator *allocator)
{
	assert (NULL == localAllocator);
	localAllocator = allocator;

	for (u32 i=0; i<NUM_PAGES; i++)
	{
		pages[i].blob = NULL;
	}

	priv_alloc_page(0);
}

//***********************************
void MemManager::unsetup()
{
	if (NULL == localAllocator)
		return;

	for (u32 i=0; i<NUM_PAGES; i++)
	{
		if (NULL == pages[i].blob)
			continue;

		assert (0 == pages[i].tracker.get_num_allocated_bytes());
		GOSFREE(localAllocator, pages[i].blob);
		pages[i].tracker.unsetup();
	}

	localAllocator = NULL;
}

//***********************************
void MemManager::priv_alloc_page (u32 index)
{
	assert (index < NUM_PAGES);
	assert (NULL == pages[index].blob);
	pages[index].blob = GOSALLOCT(u8*, localAllocator, PAGE_SIZE);
	pages[index].tracker.setup (PAGE_SIZE, SLOT_SIZE, localAllocator);
}

//***********************************
ResDescr* MemManager::priv__reserve (eResType res_type, u32 size_to_alloc, ResHandle *out_handle)
{
	assert (NULL != out_handle);
	out_handle->set_invalid();

	//alloco memoria
	FreespaceTracker::AllocInfo alloc_info;
	u32 page = 0;
	for (; page < NUM_PAGES; page++)
	{
		if (NULL == pages[page].blob)
			priv_alloc_page(page);

		if (pages[page].tracker.alloc (size_to_alloc, &alloc_info))
		{
			//creo l'handle
			u32 start_slot = alloc_info.offset / SLOT_SIZE;
			assert (start_slot * SLOT_SIZE == alloc_info.offset);

			//negli ultimi 4 bit di start_slot di metto page_index
			assert (0 == (start_slot & 0x003C0000) );
			start_slot |= (page << 18);

			const u32 num_slot_needed = alloc_info.size / SLOT_SIZE;
			assert (num_slot_needed * SLOT_SIZE == alloc_info.size);
			
			out_handle->make_handle (guard++, start_slot, num_slot_needed, (u8)res_type);
			
			//ritorno il descrittore
			ResDescr *ret = reinterpret_cast<ResDescr*>( &pages[page].blob[alloc_info.offset] );
			ret->reset();
			ret->res_handle._handle = out_handle->_handle;
			ret->refCount = 1;
			return ret;
		}
	}

	DBGBREAK;
	return NULL;
}


//***********************************
void* MemManager::priv__is_valid_handle (const ResHandle handle, u32 *out_page, FreespaceTracker::AllocInfo *out_alloc_info) const
{
	assert (handle.is_valid());
	assert (NULL != out_page);
	assert (NULL != out_alloc_info);
	
	const u32 s = handle.get_start_slot();
	const u32 page = (s >> 18);
	assert (page < NUM_PAGES);

	*out_page = page;
	out_alloc_info->offset = (s & 0x0003FFFF) * SLOT_SIZE;
	out_alloc_info->size = handle.get_num_slot() * SLOT_SIZE;
	assert (out_alloc_info->offset < PAGE_SIZE);

	void *ret = &pages[page].blob[out_alloc_info->offset];
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

	u32 page;
	FreespaceTracker::AllocInfo alloc_info;
	void *pt = priv__is_valid_handle (handle, &page, &alloc_info);
	if (NULL != pt)
	{
		pages[page].tracker.free(alloc_info);
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

	u32 page;
	FreespaceTracker::AllocInfo alloc_info;
	void *pt = priv__is_valid_handle (res_handle, &page, &alloc_info);
	if (NULL == pt)
	{
		DBGBREAK;
		return NULL;
	}
	return (ResDescr*)pt;
}

