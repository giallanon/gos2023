#include "MemManager.h"
#include "../gos/gosBit.h"

using namespace gos;
using namespace gos::res2;


//***********************************
void MemManager::setup (gos::Allocator *allocator)
{
	assert (NULL == localAllocator);
	localAllocator = allocator;

	num_pages = 0;
	for (u32 i=0; i<PAGE__NUM_MAX; i++)
		page_list[i].blob = NULL;

	priv__alloc_page();		
}

//***********************************
void MemManager::unsetup()
{
	if (NULL == localAllocator)
		return;

	for (u32 i=0; i<num_pages; i++)
	{
#ifdef _DEBUG
		if (page_list[i].tracker.get_num_allocated_bytes() > 0)
		{
			//qualche handle non e' stato rilasciato
			DBGBREAK;
		}
#endif
		GOSFREE(localAllocator, page_list[i].blob);
		page_list[i].tracker.unsetup();
	}

	localAllocator = NULL;
	num_pages = 0;
}

//***********************************
u32 MemManager::priv__alloc_page()
{
	if (num_pages >= PAGE__NUM_MAX)
	{
		DBGBREAK;
		return u32MAX;
	}

	const u32 index = num_pages++;
	Page *page = &page_list[index];
	page->blob = GOSALLOCT(u8*, localAllocator, PAGE__SIZE);
	page->tracker.setup (PAGE__SIZE, SLOT_SIZE, localAllocator);
	return index;
}

//***********************************
ResDescr* MemManager::reserve (eResType res_type, u32 size_to_alloc, ResHandle *out_handle)
{
	assert (NULL != out_handle);
	out_handle->set_invalid();

	//alloco memoria
	FreespaceTracker::AllocInfo alloc_info;
	u32 page_index;
	if (!priv__reserve (size_to_alloc, &alloc_info, &page_index))
		return NULL;

	//creo l'handle
	const u64 num_slot = alloc_info.size / SLOT_SIZE;
	assert (num_slot * SLOT_SIZE == alloc_info.size);
	out_handle->make_handle (guard++, page_index, alloc_info.offset, num_slot, (u8)res_type, true);
	
	//ritorno il descrittore
	ResDescr *ret = reinterpret_cast<ResDescr*>( &page_list[page_index].blob[alloc_info.offset] );
	ret->reset();
	ret->res_handle._handle = out_handle->_handle;
	ret->refCount = 1;
	return ret;
}

//***********************************
AssetDescr*	MemManager::reserve (eAssetType asset_type, u32 size_to_alloc, AssetHandle *out_handle)
{
	assert (NULL != out_handle);
	out_handle->set_invalid();

	//alloco memoria
	FreespaceTracker::AllocInfo alloc_info;
	u32 page_index;
	if (!priv__reserve (size_to_alloc, &alloc_info, &page_index))
		return NULL;

	//creo l'handle
	const u64 num_slot = alloc_info.size / SLOT_SIZE;
	assert (num_slot * SLOT_SIZE == alloc_info.size);
	out_handle->make_handle (guard++, page_index, alloc_info.offset, num_slot, (u8)asset_type, false);
	
	//ritorno il descrittore
	AssetDescr *ret = reinterpret_cast<AssetDescr*>( &page_list[page_index].blob[alloc_info.offset] );
	ret->reset();
	ret->asset_handle._handle = out_handle->_handle;
	ret->refCount = 1;
	return ret;
}

//***********************************
bool MemManager::priv__reserve (u32 size_to_alloc, FreespaceTracker::AllocInfo *out_alloc_info, u32 *out_page_index)
{
	assert (size_to_alloc <= PAGE__SIZE);
	assert (NULL != out_alloc_info);
	assert (NULL != out_page_index);


	for (u32 i=0; i<num_pages; i++)
	{
		if (page_list[i].tracker.alloc (size_to_alloc, out_alloc_info))
		{
			*out_page_index = i;
			return true;
		}
	}

	u32 page_index = priv__alloc_page();
	if (u32MAX == page_index)
	{
		logger::err ("res2::MemManager::priv__reserve() => can't allocate a new page. Num allocated pages: %d\n", num_pages);
		return false;
	}

	if (page_list[page_index].tracker.alloc (size_to_alloc, out_alloc_info))
	{
		*out_page_index = page_index;
		return true;
	}

	logger::err ("res2::MemManager::priv__reserve() => can't reserve a new handle, out of memory. Num allocated pages: %d\n", num_pages);
	return false;
}

//***********************************
void* MemManager::priv__is_valid_handle (const BaseHandle handle, u8 *out_page_index, FreespaceTracker::AllocInfo *out_alloc_info) const
{
	assert (handle.is_valid());
	assert (NULL != out_page_index);
	assert (NULL != out_alloc_info);
	
	const u8 page_index = handle.get_page_index();
	if (page_index >= num_pages)
	{
		DBGBREAK;
		return NULL;
	}

	*out_page_index = page_index;
	out_alloc_info->offset = handle.get_offset();
	out_alloc_info->size = handle.get_num_slot() * SLOT_SIZE;
	assert (out_alloc_info->offset < PAGE__SIZE);

	void *ret = &page_list[page_index].blob[out_alloc_info->offset];
	const u64 *blob = reinterpret_cast<const u64*>( ret );
	if (blob[0] != handle._handle)
	{
		DBGBREAK;
		return NULL;
	}
	return ret;
}

//***********************************
void MemManager::release (BaseHandle &handle)
{
	if (handle.is_invalid())
		return;

	FreespaceTracker::AllocInfo alloc_info;
	u8 page_index;
	void *pt = priv__is_valid_handle (handle, &page_index, &alloc_info);
	if (NULL != pt)
	{
		page_list[page_index].tracker.free(alloc_info);
		u64 *p64 = (u64*)pt;
		p64[0] = u64MAX;
	}


	handle.set_invalid();
}

//***********************************
const ResDescr*	MemManager::get (ResHandle res_handle) const
{
	if (res_handle.is_invalid())
	{
		DBGBREAK;
		return NULL;
	}
	assert (res_handle.is_a_resource());

	FreespaceTracker::AllocInfo alloc_info;
	u8 page_index;
	void *pt = priv__is_valid_handle (res_handle, &page_index, &alloc_info);
	if (NULL == pt)
	{
		DBGBREAK;
		return NULL;
	}
	return (const ResDescr*)pt;
}

ResDescr* MemManager::get (ResHandle res_handle)
{
	if (res_handle.is_invalid())
	{
		DBGBREAK;
		return NULL;
	}
	assert (res_handle.is_a_resource());

	FreespaceTracker::AllocInfo alloc_info;
	u8 page_index;
	void *pt = priv__is_valid_handle (res_handle, &page_index, &alloc_info);
	if (NULL == pt)
	{
		DBGBREAK;
		return NULL;
	}
	return (ResDescr*)pt;
}

//***********************************
const AssetDescr* MemManager::get (AssetHandle asset_handle) const
{
	if (asset_handle.is_invalid())
	{
		DBGBREAK;
		return NULL;
	}
	assert (asset_handle.is_an_asset());

	FreespaceTracker::AllocInfo alloc_info;
	u8 page_index;
	void *pt = priv__is_valid_handle (asset_handle, &page_index, &alloc_info);
	if (NULL == pt)
	{
		DBGBREAK;
		return NULL;
	}
	return (const AssetDescr*)pt;
}

AssetDescr* MemManager::get (AssetHandle asset_handle)
{
	if (asset_handle.is_invalid())
	{
		DBGBREAK;
		return NULL;
	}
	assert (asset_handle.is_an_asset());

	FreespaceTracker::AllocInfo alloc_info;
	u8 page_index;
	void *pt = priv__is_valid_handle (asset_handle, &page_index, &alloc_info);
	if (NULL == pt)
	{
		DBGBREAK;
		return NULL;
	}
	return (AssetDescr*)pt;
}
