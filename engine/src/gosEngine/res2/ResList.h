#ifndef _gosEngineRes2_ResList_h_
#define _gosEngineRes2_ResList_h_
#include "enumAndDefine.h"
#include "../gos/gosFreespaceTracker.h"

namespace gos
{
	namespace res2
	{
		/*************************************************
		 * @brief	res2::List
		 *
		 */
		class List
		{
		public:
					List()						{ allocator = NULL; }
					~List()						{ unsetup(); }

			bool 	is_already_setup() const 	{ return (NULL != allocator); }

			void 	setup (gos::Allocator *allocator, u32 num_res_per_page, u16 num_pages);
			void 	unsetup();

			void*		reserve (u32 size_to_alloc, ResHandle *out_handle);
			void 		release (ResHandle handle);
			ResDescr*	get_descr (ResHandle handle)				{ return static_cast<ResDescr*>(priv_get_descr(handle)); }
				
						template<class T>
			T*			get_full_resource (ResHandle handle)		{ return static_cast<T*>(priv_get_descr(handle)); }

		private:
			struct sRecord
			{
				u16	cur_counter;
				u16 next_free;
				gos::FreespaceTracker::AllocInfo alloc_info;
			};

			struct sPage
			{
				sRecord					*handle_list;
				void					*blob;
				gos::FreespaceTracker	blob_tracker;
				u16 					cur_allocated;
				u16 					first_free;	
			};
		
		private:
			void 		priv_alloc_page (u32 page_index);
			void 		priv_free_page (u32 page_index);
			void*		priv_do_reserve_from_page (u32 page_index, u32 size_to_alloc, ResHandle *out_handle);
			void*		priv_get_descr (ResHandle handle);

		private:
			gos::Allocator *allocator;
			sPage			*pages;
			u32 			num_res_per_page;
			u16 			num_max_pages;
			u16				size_of_a_slot;
		};

	} //namespace res
} //namespace gos

#endif //_gosEngineRes2_ResList_h_
