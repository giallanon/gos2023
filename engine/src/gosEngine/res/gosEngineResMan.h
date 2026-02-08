#ifndef _gosEngineResMan_h_
#define _gosEngineResMan_h_
#include "gosEngineResList.h"

namespace gos
{
	namespace res
	{
		/***
		 * @brief	res::Manager
		 * 			mantiene un elenco di res::List, una per ogni tipo di risorsa che e' stata 
		 * 			aggiunta usando addResType()
		 */
		class Manager
		{
		public:
					Manager()								{ allocator = NULL; }
					~Manager()								{ unsetup(); }

			void	setup (gos::Allocator *allocator);
			void 	unsetup();

					template<class RES>
			void 	addResType (res::eType type, u32 num_res_per_page, u16 num_pages)
					{
						priv_addResType (type, num_res_per_page, num_pages, sizeof(RES));
					}

					template<class RES, class RES_HANDLE>
			bool	reserve (res::eType type, RES_HANDLE *out_handle, RES **out_resource)
					{
						void *res = raw_reserve (type, &out_handle->res_handle);
						if (NULL == res)
							return false;

						(*out_resource) = reinterpret_cast<RES*> (res);
						(*out_resource)->_descr.reset();
						return true;
					}

					template<class RES>
			void 	release (RES handle)
					{
						raw_release (handle.res_handle);
					}

					template<class RES, class RES_HANDLE>
			bool	get_data (RES_HANDLE handle, RES **out_resource)
					{
						void *res = raw_get_data(handle.res_handle);
						if (NULL == res)
							return false;
						(*out_resource) = reinterpret_cast<RES*> (res);
						return true;
					}


			void*	raw_reserve (res::eType type, Handle *out_handle);
			void*	raw_get_data (Handle handle);
			void 	raw_release (Handle handle);


		private:
			static constexpr u32 NUM_MAX_LIST = (u32)res::eType::NUM_MAX;

		private:
			void 	priv_addResType (res::eType type, u32 num_res_per_page, u16 num_pages, u32 sizeof_a_single_res);

		private:
			gos::Allocator 	*allocator;
			res::List		*lists[NUM_MAX_LIST];
		};
	} //namespace res
} //namespace gos

#endif //_gosEngineResMan_h_