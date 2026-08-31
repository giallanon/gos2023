#ifndef _land_Chache_h_
#define _land_Chache_h_
#include "gos.h"


namespace land
{
	template <class KEY>
	class CacheLRU
	{
	public:
		CacheLRU()
		{
			num_max_elem = num_elem = 0;
			localAllocator = NULL;
			lru_list = NULL;
			key_list = NULL;
		}

		~CacheLRU()																{ priv__free(); }

		void 	setup (gos::Allocator *allocator, u32 num_elemIN, u32 sizeof_dataSlotIN)
		{
			priv__free();
			localAllocator = allocator;
			num_max_elem = num_elemIN;
			num_elem = 0;
			sizeof_dataSlot = sizeof_dataSlotIN;
			lru_list = GOSALLOCT(u32*, localAllocator, sizeof(u32) * num_max_elem);
			key_list = GOSALLOCT(KEY*, localAllocator, sizeof(KEY) * num_max_elem);
		}

		bool	get_from_cache (u32 timenow_msec, const KEY &key, u32 *out_offset)
		{
			assert (NULL != out_offset);
			for (u32 i=0; i<num_elem; i++)
			{
				if (key_list[i] == key)
				{
					lru_list[i] = timenow_msec;
					*out_offset = sizeof_dataSlot * i;
					return true;
				}
			}
			return false;
		}

		u32		get_a_slot (u32 timenow_msec, const KEY &key)
		{
			u32 worst_index = 0;

			if (num_elem < num_max_elem)
			{
				worst_index = num_elem++;
			}
			else
			{
				u32 worst_lru = lru_list[0];
				for (u32 i=1; i<num_elem; i++)
				{
					if (lru_list[i] < worst_lru)
					{
						worst_index = i;
						worst_lru = lru_list[i];
					}
				}
			}

			lru_list[worst_index] = timenow_msec;
			key_list[worst_index] = key;
			return sizeof_dataSlot * worst_index;
		}


	private:
		void	priv__free()
		{
			if (NULL == localAllocator)
				return;
			GOSFREE_AND_NULL(localAllocator, lru_list);
			GOSFREE_AND_NULL(localAllocator, key_list);
		}


	private:
		gos::Allocator	*localAllocator;
		u32 			*lru_list;
		KEY				*key_list;
		u32 			num_elem;
		u32 			num_max_elem;
		u32 			sizeof_dataSlot;

	};
}

#endif //_land_Chache_h_