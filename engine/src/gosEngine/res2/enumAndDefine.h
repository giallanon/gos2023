#ifndef _gosEngineRes2_enumAndDefine_h_
#define _gosEngineRes2_enumAndDefine_h_
#include "../../gos/gos.h"
#include "../../gosAsset2/gosAsset2EnumAndDefine.h"
#include "../../gos/gosBit.h"

namespace gos
{
	class Engine; //fwd

	namespace res2
	{
		enum class eLoadMode : u8
		{
			asap = 0,
			onDemand = 1
		};

		enum class eStatus : u8
		{
			ready			= 0,
			unloaded		= 1,
			loading			= 2,
			loaded			= 3,
			unloading		= 4,
			error 			= 0xff		//errore fatale. Esiste nell'engine ma probabilmente il loader non e' riuscito a caricarla, questo asset e' spacciato per sempre
		};

		enum class eResType : u8
		{
			_unused_zero 	= 0,
			vtx_buffer 		= 1,
			idx_buffer 		= 2,
			vtx_shader 		= 3,
			pxl_shader 		= 4,
			pipeline 		= 5,
			texture_2d 		= 6,
			shape			= 7,
			gpu_shape 		= 8,
			skeleton 		= 9,
			model_3d 		= 10,
			
			NUM_MAX 		= 11	//questo deve essre uguale all'ultimo valore +1
		};

		enum class eAssetType : u8
		{
			_unused_zero 	= 0,
			vtx_shader 		= 1,
			pxl_shader 		= 2,
			pipeline 		= 3,
			texture_2d 		= 4,
			shape 			= 5,
			skeleton 		= 6,
			materialPBR		= 7,
			model_3d 		= 8,
			model_instance 	= 9,

			NUM_MAX 		= 10	//questo deve essre uguale all'ultimo valore +1
		};


		class MemManager; //fwd
		struct ResDescr; //fwd
		struct AssetDescr; //fwd

		/*********************************
		 * @brief	BaseHandle
		 */
		struct BaseHandle
		{
			void		set_invalid()									{ _handle = u64MAX; }
			bool		is_valid() const								{ return _handle != u64MAX; }
			bool		is_invalid() const								{ return _handle == u64MAX; }

			u64 		viewAsU64() const 								{ return _handle; }
			void 		setFromU64(u64 h)								{ _handle = h; }

		protected:
			void 		make_handle (u8 guard, u32 page_index, u32 offset, u32 num_slot, u8 type, bool is_a_resource);
			u32			get_offset() const								{ return (u32)( _handle & 0x00000000FFFFFFFF ); }
			u8			get_page_index() const							{ return (u8)( (_handle >> 32) & 0x00000000000000FF ); }
			u16			get_num_slot() const							{ return (u16)( (_handle >> 40) & 0x00000000000007FF ); }
			u8			get_type() const								{ return (u8)( (_handle >> 51) & 0x00000000000000FF ); }

			bool		is_a_resource() const							{ return (_handle & 0x8000000000000000) != 0; }
			eResType	get_res_type() const							{ assert(is_a_resource()); return static_cast<eResType>( get_type() ); }
			bool		is_a_resource_of_type(eResType t) const			{ return get_res_type() == t; }

			bool		is_an_asset() const								{ return !is_a_resource(); }
			eAssetType	get_asset_type() const							{ assert(is_an_asset()); return static_cast<eAssetType>( get_type() ); }
			bool		is_an_asset_of_type(eAssetType t) const			{ return get_asset_type() == t; }

		private:
			u64		_handle;

			friend 	MemManager;
		};


		//************************************************
		struct	ResHandle: public BaseHandle
		{
			eResType	get_type() const							{ return get_res_type(); }
			bool		is_a_resource_of_type(eResType t) const		{ return get_type() == t; }
		};

		
		//************************************************
		struct	AssetHandle : public BaseHandle
		{
			eAssetType	get_type() const							{ return get_asset_type(); }
			bool		is_an_asset_of_type(eAssetType t) const		{ return get_type() == t; }
		};


		//*********************************
		struct AssetHandleChain
		{
			AssetDescr			*asset;
			AssetHandleChain	*next;
		};
		
		
		/*******************************************************
		 * @brief	ResDescr
		 * 			Un descrittore di risorsa
		 */
		struct ResDescr
		{
		public:
			void 		reset()					{ signatureUID.setInvalid(); res_handle.set_invalid(); refCount = 0; status = eStatus::error; asset_owner_list = NULL; }
			eResType	get_type() const		{ return res_handle.get_type(); }

		public:
			ResHandle			res_handle;			//IMPORTANTE: deve essere il primo elemento della struct
			eStatus				status;				//stato della risorsa dal punto di vista dell'engine  (non cambiare direttamente il valore, usa res__set_status()
			u8					pad0;
			u16					refCount;
			asset2::UID			signatureUID;		//se invalido, vuol dire che la risorsa e' stata creata 'a mano' e non e' un asset presente su disco
			AssetHandleChain	*asset_owner_list;	//lista di asset di cui io sono figlio (che vengono notificati ogni volta che io cambio di stato)
		};

		/*******************************************************
		 * @brief	AssetDescr
		 * 			Un descrittore di asset
		 */
		struct AssetDescr
		{
		public:
			void 	reset()					{ assetUID.setInvalid(); asset_handle.set_invalid(); refCount = 0; status=eStatus::error; num_child_not_ready=0; asset_owner_list=NULL; child_list=NULL; }
			eAssetType	get_type() const 	{ return asset_handle.get_type(); }

		public:
			AssetHandle			asset_handle;			//IMPORTANTE: deve essere il primo elemento della struct
			eStatus				status;					
			u8					num_child_not_ready;	//se ho dei figli, questo mi dice quanti di loro sono in stato != da eReady
			u16					refCount;
			asset2::UID			assetUID;				//se invalido, vuol dire che la risorsa e' stata creata 'a mano' e non e' un asset presente su disco
			AssetHandleChain	*asset_owner_list;		//lista di asset di cui io sono figlio (che vengono notificati ogni volta che io cambio di stato)
			AssetHandleChain	*child_list;			//lista degli asset figli
		};




	} //namespace res
} //namespace gos

#endif //_gosEngineRes2_enumAndDefine_h_
