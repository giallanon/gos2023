#include "gosEngineResList.h"
#include "../gos/gosBit.h"

using namespace gos;
using namespace gos::res;


//***********************************
void List::setup (gos::Allocator *allocatorIN, u8 res_typeIN, u32 num_max_resourceIN, u16 num_res_per_pageIN, u32 sizeof_a_single_resIN)
{
	assert (NULL == allocator);
	assert (GOS_IS_POWER_OF_TWO(num_max_resourceIN));
	assert (GOS_IS_POWER_OF_TWO(num_res_per_pageIN));
	assert (num_res_per_pageIN <= Handle::MAX_NUM_INDEX);
	
	allocator = allocatorIN;
	res_type = res_typeIN;
	num_res_per_page = num_res_per_pageIN;
	
	num_max_pages = (u8) (num_max_resourceIN / num_res_per_pageIN);
	if (num_max_pages < 1)
		num_max_pages = 1;
	assert (num_max_pages<= Handle::MAX_NUM_PAGE);

	real_size_of_a_record = GOS_ALIGN_NUMBER_TO_POWER_OF_TWO(sizeof(sRecord) + sizeof_a_single_resIN, 8);

	pages = GOSALLOCT(sPage*, allocator, sizeof(sPage) * num_max_pages);
	memset (pages, 0, sizeof(sPage) * num_max_pages);
	priv_alloc_page (0);	
}

//***********************************
void List::unsetup()
{
	if (NULL == allocator)
		return;

#ifdef _DEBUG
	//verifico che tutti gli handle siano stati rilasciati
	for (u32 i=0; i<num_max_pages; i++)
	{
		if (NULL == pages[i].blob)
			continue;

		u32 num_free = 0;
		u32 free_index = pages[i].first_free;
		while (free_index != u16MAX)
		{
			sRecord *rec = reinterpret_cast<sRecord*> (&pages[i].blob[free_index * real_size_of_a_record]);
			free_index = rec->next_free;
			num_free++;
		}
		assert (num_free == num_res_per_page);

	}
#endif

	for (u32 i=0; i<num_max_pages; i++)
	{
		if (NULL != pages[i].blob)
			priv_free_page (i);
	}

	GOSFREE(allocator, pages);
	allocator = NULL;
}

//***********************************
void List::priv_free_page (u32 page_index)
{
	assert (page_index < num_max_pages);
	assert (NULL != pages[page_index].blob);

	GOSFREE(allocator, pages[page_index].blob);
	memset (&pages[page_index], 0, sizeof(sPage));
}

//***********************************
void List::priv_alloc_page (u32 page_index)
{
	assert (page_index < num_max_pages);
	assert (NULL == pages[page_index].blob);

	pages[page_index].blob = GOSALLOCT(u8*, allocator, real_size_of_a_record * num_res_per_page);
	pages[page_index].cur_allocated = 0;
	pages[page_index].first_free = 0;

	sRecord *rec = NULL;
	u32 ct = 0;
	for (u32 i=0; i<num_res_per_page; i++)
	{
		rec = reinterpret_cast<sRecord*>( &pages[page_index].blob[ct] );
		ct += real_size_of_a_record;

		rec->cur_counter = 1;
		rec->next_free = (u16)(i+1);
	}
	rec->next_free = u16MAX;
}


//***********************************
void* List::reserve (Handle *out_handle)
{
	assert (NULL != out_handle);

	for (u8 i=0; i<num_max_pages; i++)
	{
		if (NULL == pages[i].blob)
			continue;
		if (pages[i].first_free == u16MAX)
			continue;

		return priv_do_reserve_from_page(i, out_handle);
	}

	for (u8 i=0; i<num_max_pages; i++)
	{
		if (NULL == pages[i].blob)
		{
			priv_alloc_page(i);
			return priv_do_reserve_from_page(i, out_handle);
		}
	}

	DBGBREAK;
	return NULL;
}

//***********************************
void* List::priv_do_reserve_from_page (u32 page_index, Handle *out_handle)
{
	assert (page_index < num_max_pages);

	sPage *p = &pages[page_index];
	p->cur_allocated++;

	const u32 index = p->first_free;
	sRecord *rec = reinterpret_cast<sRecord*>( &p->blob[real_size_of_a_record * index] );
	p->first_free = rec->next_free;

	const u16 counter = rec->cur_counter;

	out_handle->set_value_TYPE (res_type);
	out_handle->set_value_COUNTER (counter);
	out_handle->set_value_PAGE (page_index);
	out_handle->set_value_INDEX (index);
	return &p->blob[real_size_of_a_record * index + sizeof(sRecord)];
}

//***********************************
void List::release (Handle handle)
{
	if (handle.isInvalid())
		return;

	const u32 index = handle.get_value_INDEX();
	const u32 counter = handle.get_value_COUNTER();
	const u32 page_index = handle.get_value_PAGE();

	assert (handle.get_value_TYPE() == res_type);
	assert (index < num_res_per_page);
	assert (page_index < num_max_pages);

	sPage *p = &pages[page_index];
	assert (NULL != p->blob);
	assert (p->cur_allocated > 0);

	const u32 ct = index * real_size_of_a_record;
	sRecord *rec = reinterpret_cast<sRecord*>( &p->blob[ct] );
	if (rec->cur_counter != counter)
		return;

	p->cur_allocated--;
	rec->cur_counter++;
	rec->next_free = p->first_free;
	p->first_free = index;
}

//***********************************
void* List::get_data (Handle handle)
{
	if (handle.isInvalid())
		return NULL;

	const u32 index = handle.get_value_INDEX();
	const u32 counter = handle.get_value_COUNTER();
	const u32 page_index = handle.get_value_PAGE();

	assert (handle.get_value_TYPE() == res_type);
	assert (index < num_res_per_page);
	assert (page_index < num_max_pages);

	sPage *p = &pages[page_index];
	assert (NULL != p->blob);
	assert (p->cur_allocated > 0);

	const u32 ct = index * real_size_of_a_record;
	sRecord *rec = reinterpret_cast<sRecord*>( &p->blob[ct] );
	if (rec->cur_counter != counter)
		return NULL;

	return &p->blob[ct + sizeof(sRecord)];
	
}