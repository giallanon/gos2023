#ifndef _gosEngineRes2EnumAndDefine_h_
#define _gosEngineRes2EnumAndDefine_h_
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

		/******************************
		 * @brief	Handle
		 * 			 A bit per "tipo di risorsa"
		 *			 B bit per "counter"
		 *			 C bit per "page"	(32)
		 *			 D bit per "index"	(8192)
		 */
		template<int A, int B, int C, int D>
		struct HandleT
		{
		private:
			static const constexpr u32	MASKSHIFT_0 = D + C + B;
			static const constexpr u32	MASKSHIFT_1 = D + C;
			static const constexpr u32	MASKSHIFT_2 = D;
			static const constexpr u32	MASKSHIFT_3 = 0;

			static_assert (A + B + C + D == 32);
			static_assert (A>0);
			static_assert (B>0);
			static_assert (C>0);
			static_assert (D>0);
			
			static const constexpr u32	MASK_0 = static_cast<u32> (((u64)((0x0000000000000001 << A) - 1) << (u64)MASKSHIFT_0) & 0x00000000FFFFFFFF);
			static const constexpr u32	MASK_1 = static_cast<u32> (((u64)((0x0000000000000001 << B) - 1) << (u64)MASKSHIFT_1) & 0x00000000FFFFFFFF);
			static const constexpr u32	MASK_2 = static_cast<u32> (((u64)((0x0000000000000001 << C) - 1) << (u64)MASKSHIFT_2) & 0x00000000FFFFFFFF);
			static const constexpr u32	MASK_3 = static_cast<u32> (((u64)((0x0000000000000001 << D) - 1) << (u64)MASKSHIFT_3) & 0x00000000FFFFFFFF);

		public:
			static constexpr u32 MAX_NUM_TYPE 		= (u32)(0x0001 << A);
			static constexpr u32 MAX_NUM_COUNTER 	= (u32)(0x0001 << B);
			static constexpr u32 MAX_NUM_PAGE		= (u32)(0x0001 << C);
			static constexpr u32 MAX_NUM_INDEX		= (u32)(0x0001 << D);

			typedef HandleT<A, B, C, D> ThisHandle;

		public:
			static ThisHandle			INVALID()				{ static ThisHandle hINVALID; hINVALID.setInvalid(); return hINVALID; }

		public:
						HandleT()								{ setInvalid(); }

			bool		operator== (const ThisHandle b) const  	{ return (id == b.id); }
			bool		operator!= (const ThisHandle b) const  	{ return (id != b.id); }
			int			compare (const ThisHandle b) const 		{ if (id==b.id) return 0; if (id>b.id) return 1; return -1; }
						
			void		setInvalid()							{ id = u32MAX; }
			bool		isInvalid() const						{ return (id == u32MAX); }
			bool		isValid() const							{ return (id != u32MAX); }

			void		setFromU32 (u32 u)						{ id = u; }
			u32			viewAsU32() const						{ return id; }

			u32			get_value_TYPE() const					{ return ((id & MASK_0) >> MASKSHIFT_0); }
			u32			get_value_COUNTER() const				{ return ((id & MASK_1) >> MASKSHIFT_1); }
			u32			get_value_PAGE() const					{ return ((id & MASK_2) >> MASKSHIFT_2); }
			u32			get_value_INDEX() const					{ return ((id & MASK_3) >> MASKSHIFT_3); }

			void		set_value_TYPE (u32 value)				{ assert(value<MAX_NUM_TYPE); 	id &= ~(MASK_0);  id |= ((value << MASKSHIFT_0) & MASK_0); }
			void		set_value_COUNTER(u32 value)			{ assert(value<MAX_NUM_COUNTER);id &= ~(MASK_1);  id |= ((value << MASKSHIFT_1) & MASK_1); }
			void		set_value_PAGE(u32 value)				{ assert(value<MAX_NUM_PAGE); 	id &= ~(MASK_2);  id |= ((value << MASKSHIFT_2) & MASK_2); }
			void		set_value_INDEX(u32 value)				{ assert(value<MAX_NUM_INDEX); 	id &= ~(MASK_3);  id |= ((value << MASKSHIFT_3) & MASK_3); }

		private:
			u32	id;
		};


		typedef struct HandleT<7,4,9,12> ResHandle;		//2^12=4096 risorse per pagina, 2^9=512 pagine, counter=2^4  => max 2.097.152 handler
		typedef struct HandleT<7,4,8,13> AssetHandle;	//2^13=8192 risorse per pagina, 2^8=256 pagine, counter=2^4  => max 2.097.152 handler

		struct ResDescr; //fwd
		struct AssetDescr; //fwd
		
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
			void 		reset()					{ signatureUID.setInvalid(); res_handle.setInvalid(); refCount = 0; status = eStatus::error; asset_owner_list = NULL; on_destroy=NULL; }
			eResType	get_type() const		{ return static_cast<eResType>(res_handle.get_value_TYPE()); }

		public:
			eStatus				status;				//stato della risorsa dal punto di vista dell'engine  (non cambiare direttamente il valore, usa res__set_status()
			u8					pad0;
			i16					refCount;
			ResHandle			res_handle;
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
			void 	reset()
			{
				assetUID.setInvalid(); asset_handle.setInvalid(); refCount = 0; status=eStatus::error; num_child_not_ready=0; asset_owner_list=NULL; child_list=NULL;
			}

			eAssetType	get_type() const 			{ return static_cast<eAssetType>(asset_handle.get_value_TYPE()); }

		public:
			eStatus				status;					
			u8					num_child_not_ready;	//se ho dei figli, questo mi dice quanti di loro sono in stato != da eReady
			i16					refCount;
			AssetHandle			asset_handle;
			asset2::UID			assetUID;				//se invalido, vuol dire che la risorsa e' stata creata 'a mano' e non e' un asset presente su disco
			AssetHandleChain	*asset_owner_list;		//lista di asset di cui io sono figlio (che vengono notificati ogni volta che io cambio di stato)
			AssetHandleChain	*child_list;			//lista degli asset figli
		};


	} //namespace res
} //namespace gos

#endif //_gosEngineRes2EnumAndDefine_h_
