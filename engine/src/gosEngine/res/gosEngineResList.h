#ifndef _gosEngineResList_h_
#define _gosEngineResList_h_
#include "gosEngineResEnumAndDefine.h"

namespace gos
{
	namespace res
	{
		/***
		 * @brief	res::List
		 * 			Una lista paginata di risorse aventi dimensione <sizeof_a_single_res>
		 * 
		 */
		class List
		{
		public:
					List()						{ allocator = NULL; }
					~List()						{ unsetup(); }

			bool 	is_already_setup() const 	{ return (NULL != allocator); }

			void 	setup (gos::Allocator *allocator, u8 res_type, u32 num_max_resource, u16 num_res_per_page, u32 sizeof_a_single_res);
			void 	unsetup();

			void*	reserve (Handle *out_handle);
			void 	release (Handle handle);
			void*	get_data (Handle handle);

		private:
			struct sRecord
			{
				u16	cur_counter;
				u16 next_free;
				u32 pad;
			};

			struct sPage
			{
				u8 		*blob;
				u16 	cur_allocated;
				u16 	first_free;	
			};
		
		private:
			void 	priv_alloc_page (u32 page_index);
			void 	priv_free_page (u32 page_index);
			void*	priv_do_reserve_from_page (u32 page_index, Handle *out_handle);

		private:
			gos::Allocator *allocator;
			u32 			num_res_per_page;
			u32 			real_size_of_a_record;
			u8 				res_type;
			u8 				num_max_pages;
			u8 				_pad0;
			sPage			*pages;
		};

	} //namespace res
} //namespace gos

#endif //_gosEngineResList_h_