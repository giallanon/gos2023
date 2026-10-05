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
								MemManager()							{ localAllocator = NULL; num_pages=0; guard=0; }
								~MemManager()							{ unsetup(); }

			bool 				is_already_setup() const 				{ return (NULL != localAllocator); }

			void 				setup (gos::Allocator *allocator);
			void 				unsetup();

			ResDescr*			reserve (eResType res_type, u32 size_to_alloc, ResHandle *out_handle);
			AssetDescr*			reserve (eAssetType asset_type, u32 size_to_alloc, AssetHandle *out_handle);
			
			void 				release (BaseHandle &handle);
			
			const ResDescr*		get (ResHandle res_handle) const;
			ResDescr*			get (ResHandle res_handle);
			const AssetDescr*	get (AssetHandle asset_handle) const;
			AssetDescr*			get (AssetHandle asset_handle);


								template<class RES_CLASS>
			RES_CLASS*			reserveT (ResHandle *out_handle)	{ RES_CLASS *ret = reinterpret_cast<RES_CLASS*>( reserve(RES_CLASS::type, sizeof(RES_CLASS), out_handle) ); ret->data.reset(); return ret; }

								template<class RES_CLASS>
			RES_CLASS*			getT (ResHandle handle)				{ assert (handle.is_a_resource_of_type(RES_CLASS::type)); return reinterpret_cast<RES_CLASS*>( get(handle) ); }

								template<class RES_CLASS>
			const RES_CLASS*	getT (ResHandle handle) const		{ assert (handle.is_a_resource_of_type(RES_CLASS::type)); return reinterpret_cast<const RES_CLASS*>( get(handle) ); }								

								template<class ASSET_CLASS>
			ASSET_CLASS*		reserveT (AssetHandle *out_handle)	{ ASSET_CLASS *ret = reinterpret_cast<ASSET_CLASS*>( reserve(ASSET_CLASS::type, sizeof(ASSET_CLASS), out_handle) ); ret->data.reset(); return ret; }

								template<class ASSET_CLASS>
			ASSET_CLASS*		getT (AssetHandle handle)			{ assert (handle.is_an_asset_of_type(ASSET_CLASS::type)); return reinterpret_cast<ASSET_CLASS*>( get(handle) ); }

								template<class ASSET_CLASS>
			const ASSET_CLASS*	getT (AssetHandle handle) const		{ assert (handle.is_an_asset_of_type(ASSET_CLASS::type)); return reinterpret_cast<const ASSET_CLASS*>( get(handle) ); }								

		private:
			static constexpr u32		PAGE__SIZE = 128 * 1024 * 1024;
			static constexpr u32		PAGE__NUM_MAX = 32;
			static constexpr u32		SLOT_SIZE = 64;

		private:
			struct Page
			{
				gos::FreespaceTracker	tracker;
				u8						*blob;
			};

		private:
			u32		priv__alloc_page();
			bool 	priv__reserve (u32 size_to_alloc, FreespaceTracker::AllocInfo *out_alloc_info, u32 *out_page_index);
			void* 	priv__is_valid_handle (const BaseHandle handle, u8 *out_page_index, FreespaceTracker::AllocInfo *out_alloc_info) const;

		private:
			gos::Allocator 	*localAllocator;
			Page			page_list[PAGE__NUM_MAX];
			u32				num_pages;
			u8				guard;
		};

	} //namespace res
} //namespace gos

#endif //_gosEngineRes2_MemManager_h_
