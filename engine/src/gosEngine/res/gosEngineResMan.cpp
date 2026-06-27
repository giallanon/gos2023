#include "gosEngineResMan.h"

using namespace gos;
using namespace gos::res;

//*******************************
void Manager::setup (gos::Allocator *allocatorIN)
{
	assert (NULL == allocator);
	allocator = allocatorIN;

	for (u32 i=0; i<NUM_MAX_LIST; i++)
	{
		lists[i] = GOSNEW(allocator, res::List)();
	}
}

//*******************************
void Manager::unsetup()
{
	if (NULL == allocator)
		return;

	for (u32 i=0; i<NUM_MAX_LIST; i++)
	{
		GOSDELETE(allocator, lists[i]);
	}
	allocator = NULL;
}

//*******************************
void Manager::priv_addResType (res::eType type, u32 num_res_per_page, u16 num_pages, u32 sizeof_a_single_res)
{
	const u8 index = (u8)type;
	assert (index < NUM_MAX_LIST);
	assert (index < res::Handle::MAX_NUM_TYPE);

	assert (!lists[index]->is_already_setup());

	lists[index]->setup (allocator, index, num_res_per_page, num_pages, sizeof_a_single_res);
}

//*******************************
void* Manager::raw_reserve (res::eType type, Handle *out_handle)
{
	const u8 index = (u8)type;
	assert (index < NUM_MAX_LIST);
	assert (lists[index]->is_already_setup()); //vuol dire che non hai addato la res_type
	return lists[index]->reserve (out_handle);
}

//*******************************
void Manager::raw_release (Handle handle)
{
	if (handle.isInvalid())
		return;
	
	const u8 index = handle.get_value_TYPE();
	assert (index < NUM_MAX_LIST);
	assert (lists[index]->is_already_setup());
	lists[index]->release (handle);
}

//*******************************
void* Manager::raw_get_data (Handle handle)
{
	if (handle.isInvalid())
		return NULL;
	
	const u8 index = handle.get_value_TYPE();
	assert (index < NUM_MAX_LIST);
	assert (lists[index]->is_already_setup());
	return lists[index]->get_data (handle);
}
