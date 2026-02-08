#ifndef _gosEngineResEnumAndDefine_h_
#define _gosEngineResEnumAndDefine_h_
#include "../../gos/gos.h"
#include "../../gosAsset2/gosAsset2EnumAndDefine.h"

namespace gos
{
	class Engine; //fwd

	typedef void (Engine::*FN_afterCreate)(void *res);
	typedef void (Engine::*FN_destroy)(void *res);


	namespace res
	{
		enum class eLoadMode : u8
		{
			asap = 0,
			onDemand = 1
		};

		enum class eStatus : u8
		{
			ready			= 0,
			notLoaded		= 1,		//esiste nell'engine ma non e' stata ancora caricata
			loading			= 2,		//esiste nell'engine e' ed in fase di caricamento
			error 			= 0xff		//errore fatale. Esiste nell'engine ma probabilmente il loader non e' riuscito a caricarla, questo asset e' spacciato per sempre
		};

		enum class eType : u8
		{
			_unused_zero 	= 0,
			vtx_buffer 		= 1,
			idx_buffer 		= 2,
			vtx_shader 		= 3,
			pxl_shader 		= 4,
			shape 			= 5,
			gpu_shape 		= 6,
			texture_2d 		= 7,
			pipeline 		= 8,
			skeleton 		= 9,
			model_3d 		= 10,
			model_instance 	= 11,
			material 		= 12,

			NUM_MAX 		= 13	//questo deve essre uguale all'ultimo valore +1
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


		typedef struct HandleT<7,4,8,13> Handle;	//2^13=8192 risorse per pagina, 2^8=256 pagine, counter=2^4  => max 2.097.152 handler

		
		struct Descr; //fwd

		struct HandleChain
		{
			Descr		*res;
			HandleChain	*next;
		};


		/***
		 * @brief	Descr
		 * 			Un descrittore di risorsa
		 */
		struct Descr
		{
		public:
			void 	reset()			{ uid.setInvalid(); status=eStatus::error; refCount = 0; figli=padri=NULL; on_afterCreate=NULL; on_destroy=NULL; }

		public:
			asset2::UID			uid;		//se invalido, vuol dire che la risorsa e' stata creata 'a mano' e non e' un asset presente su disco
			Handle				handle;
			res::eStatus		status;		//stato della risorsa dal punto di vista dell'engine
			u8					_pad0;
			u8					_pad1;
			u8					_pad2;
			i32					refCount;

			HandleChain			*figli;		//lista di handle che sono figli miei
			HandleChain			*padri;		//lista di handle di cui io sono figlio (che vengono notificati ogni volta che io cambio di stato)

			FN_afterCreate		on_afterCreate;
			FN_destroy			on_destroy;
		};



		const char* 	enumToString (res::eLoadMode s);
		const char* 	enumToString (res::eType s);
		const char* 	enumToString (res::eStatus s);

	} //namespace res
} //namespace gos

#endif //_gosEngineResEnumAndDefine_h_
