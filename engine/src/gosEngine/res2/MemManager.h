#ifndef _gosEngineRes2_MemManager_h_
#define _gosEngineRes2_MemManager_h_
#include "enumAndDefine.h"
#include "../gos/gosFreespaceTracker.h"

namespace gos
{
	namespace res2
	{
		/*************************************************
		 * @brief	res2::MemManager
		 *
		 */
		class MemManager
		{
		public:
								MemManager()							{ localAllocator = NULL; guard=0; }
								~MemManager()							{ unsetup(); }

			bool 				is_already_setup() const 				{ return (NULL != localAllocator); }

			void 				setup (gos::Allocator *allocator);
			void 				unsetup();

			void 				release (ResHandle &handle);
			
			const ResDescr*		query (ResHandle res_handle) const;
			ResDescr*			get (ResHandle res_handle) const	{ return priv__get(res_handle); }


								template<class RES_CLASS>
			RES_CLASS*			reserveT (ResHandle *out_handle)	{ RES_CLASS *ret = reinterpret_cast<RES_CLASS*>( priv__reserve(RES_CLASS::get_type(), sizeof(RES_CLASS), out_handle) ); ret->data_reset(); return ret; }

								template<class RES_CLASS>
			RES_CLASS*			getT (ResHandle handle)				{ assert (handle.is_of_type(RES_CLASS::get_type())); return reinterpret_cast<RES_CLASS*>( get(handle) ); }

								template<class RES_CLASS>
			const RES_CLASS*	getT (ResHandle handle) const		{ assert (handle.is_of_type(RES_CLASS::get_type())); return reinterpret_cast<const RES_CLASS*>( get(handle) ); }								


		private:
			static constexpr u32		NUM_HANDLERS = 0x00400000; //2^22 dove 22 = bum bit riservati a "start_slot" in ResHandle
			static constexpr u32		SLOT_SIZE = 64;
			static constexpr u32		MAX_BLOB__SIZE = NUM_HANDLERS * SLOT_SIZE;	//268.435.456  ~250MB
			static constexpr u32		NUM_PAGES = 16;	//4 bit MSB di ResHandle->start_slot
			static constexpr u32		PAGE_SIZE = MAX_BLOB__SIZE / NUM_PAGES;

		private:
			struct Page
			{
				gos::FreespaceTracker	tracker;
				u8						*blob;
			};

		private:
			
			void		priv_alloc_page (u32 index);
			ResDescr*	priv__reserve (eResType res_type, u32 size_to_alloc, ResHandle *out_handle);
			ResDescr* 	priv__get (ResHandle res_handle) const;
			void* 		priv__is_valid_handle (const ResHandle handle, u32 *out_page, FreespaceTracker::AllocInfo *out_alloc_info) const;

		private:
			gos::Allocator 	*localAllocator;
			Page			pages[NUM_PAGES];
			u8				guard;
		};

	} //namespace res
} //namespace gos

#endif //_gosEngineRes2_MemManager_h_
