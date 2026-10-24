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
			unloading		= 4,
			deleting		= 5,
			deleted			= 6,
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
			materialPBR		= 10,
			model_3d 		= 11,
			model_instance 	= 12,
			
			NUM_MAX 		= 13	//questo deve essre uguale all'ultimo valore +1
		};


		class MemManager; //fwd
		struct ResDescr; //fwd

		/*********************************
		 * @brief	ResHandle
		 */
		struct ResHandle
		{
						ResHandle()										{ _handle = u32MAX; }
			void		set_invalid()									{ _handle = u32MAX; }
			bool		is_valid() const								{ return _handle != u32MAX; }
			bool		is_invalid() const								{ return _handle == u32MAX; }

			eResType	get_res_type() const							{ return static_cast<eResType>( get_type() ); }
			bool		is_of_type(eResType t) const					{ return get_res_type() == t; }

			u32 		viewAsU32() const 								{ return _handle; }
			void 		setFromU32(u32 h)								{ _handle = h; }

		protected:
			void 		make_handle (u8 guard, u32 start_slot, u32 num_slot, u8 type);
			u32			get_start_slot() const							{ return ( _handle & 0x003FFFFF ); }
			u16			get_num_slot() const							{ return (u16)( (_handle >> 22) & 0x07 ); }
			u8			get_type() const								{ return (u8)( (_handle >> 25) & 0x0F ); }
			u8			get_gard() const								{ return (u8)( (_handle >> 29) & 0x07 ); }

		private:
			u32			_handle;

			friend 	MemManager;
		};


		//*********************************
		struct ResHandleChain
		{
			 ResDescr		*descr;
			 ResHandleChain	*next;
		};
		
		
		/*******************************************************
		 * @brief	ResDescr
		 * 			Un descrittore di risorsa
		 */
		struct ResDescr
		{
		public:
			void 		reset()					{ UID.setInvalid(); res_handle.set_invalid(); refCount = 0; status = eStatus::error; num_child_not_ready=0; list_padri = list_figli = NULL; }
			eResType	get_type() const		{ return res_handle.get_res_type(); }
			bool 		isAnAsset() const		{ return (UID.isValid() && UID.isAnAsset()); }

		public:
			ResHandle			res_handle;		//IMPORTANTE: deve essere il primo elemento della struct
			eStatus				status;			//stato della risorsa dal punto di vista dell'engine  (non cambiare direttamente il valore, usa res__set_status()
			u8					num_child_not_ready;	//se ho dei figli, questo mi dice quanti di loro sono in stato != da eReady
			u16					refCount;
			asset2::UID			UID;			//se invalido, vuol dire che la risorsa e' stata creata 'a mano' e non e' un asset presente su disco
			ResHandleChain		*list_padri;			
			ResHandleChain		*list_figli;
		};


	} //namespace res


} //namespace gos

#endif //_gosEngineRes2_enumAndDefine_h_
