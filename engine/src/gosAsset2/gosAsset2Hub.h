#ifndef _gosAsset2Hub_h_
#define _gosAsset2Hub_h_
#include "gosAsset2Loader.h"
#include "../gos/memory/gosAllocatorHeap.h"
#include "../gos/gosThreadMsgQ.h"
#include "../gosGPU/gosGPU.h"



namespace gos
{
	namespace asset2
	{
		class Loader; //fwd decl

        /**
        * @brief    asset2::Handle
        * 
        */
        struct Handle
        {
		public:
			Handle()				{ _pt = NULL; }

		public:
            void *_pt;
        };
		
		
		/*******************************
		* @brief	asset2::Hub
		* 
		*/
		class Hub
		{
		public:
			enum class eStatusPublic : u8
			{
				ready		= 0,
				notLoaded	= 1,		//ho allocato spazio in RAM ma non ho mai chiamato il suo loader
				loading		= 2,		//il loader la sta caricando
				unloading	= 3,		//il loader la sta scaricando
				error 		= 0xff		//errore fatale, probabilmente il loader non e' riuscito a caricarla, questo asset e' spacciato per sempre
			};

			enum class eStatusInternal : u8
			{
				loaded		= 0,
				unloaded 	= 1,
				error 		= 0xff		//errore fatale, probabilmente il loader non e' riuscito a caricarla, questo asset e' spacciato per sempre
			};

		public:
					Hub();
					~Hub()																	{ priv_free(); }

			bool    setup (const char *baseFolder, gos::GPU *gpu);
			
			//processa i messaggi in coda ma con un timeout
			void 	update (u64 timenow_msec)												{ priv_update (timenow_msec + 15); }
			
			// funziona come update, solo che non ha un timeout. Fino a che ci sono messaggi in coda li processa
			void 	flush (u64 timenow_msec)												{ priv_update (timenow_msec + 60*1000*1000); }


			// false se <runtimeName> non e' un nome valido
			bool	getHandle (const char *runtimeName, Handle *out, bool bScheduleLoadNow = false);

			// chiedi di unloadare un asset e le sue dipendenze
			void 	unload (const Handle &h)													{ priv_unload(h._pt); }

			//funziona come getAsset<> ma questa looppa per un tot di tempo in attesa che l'asset
			//diventi ready
			template<class TASSET>
			bool 	getAssetWithTimeout (const Handle &h, const TASSET **out, u64 timeout_msec)
			{
				timeout_msec += gos::getTimeSinceStart_msec();
				while (1)
				{
					const u64 timenow_msec = gos::getTimeSinceStart_msec();
					update(timenow_msec);

					if (getAsset<TASSET>(h, out))
						return true;

					if (timenow_msec >= timeout_msec)
						return false;
				}
				return false;
			}


			//se ritorna true, puoi usare la risorsa, altrimenti no, provaci al prossimo giro
			template<class TASSET>
			bool 	getAsset (const Handle &h, const TASSET **out)
			{
				//mi assicuro di chiamare update() di tanto in tanto altrimenti
				//lo stato "external" degli asset non viene mai modificato
				const u64 timenow_msec = gos::getTimeSinceStart_msec();
				if (timenow_msec > lastTimeUpdateWasCalled_msec + 33)
					update(timenow_msec);


				sHeader *header = static_cast<sHeader*>(h._pt);

				*out = NULL;
				switch (header->external_status)
				{
				default:
					DBGBREAK;
					return false;

				case eStatusPublic::ready:
					{
						void *ptToAsset = (void*) (static_cast<const u8*>(h._pt) + sizeof(sHeader));
						*out = static_cast<const TASSET*> (ptToAsset);
					}
					return true;

				case eStatusPublic::notLoaded:
					//dato che mi hanno chiesto l'asset, e questo non era caricato, 
					//schedulo il caricamento ora
					priv_scheduleLoad (h._pt);
					return false;

				case eStatusPublic::loading:
				case eStatusPublic::unloading:
				case eStatusPublic::error:
					return false;
				}
			}



			template<class TASSET>
			bool 	internalUSE_getExistingAssetByUID (const UID &uid, const TASSET **out)
			{
				void *pt = priv_getExistingAssetByUID(uid);
				if (NULL != pt)
				{
#ifdef _DEBUG
					const sHeader *header = static_cast<sHeader*>(pt);
					assert (eStatusInternal::loaded == header->internal_status);
#endif

					void *ptToAsset = (void*) (static_cast<const u8*>(pt) + sizeof(sHeader));
					*out = static_cast<TASSET*>(ptToAsset);
					return true;
				}
				
				DBGBREAK;
				*out = NULL;
				return false;
			}

		private:
			typedef FastHashMap<UID, void*> HashList;

			typedef gos::AllocatorHeap<gos::AllocPolicy_Track_simple, gos::AllocPolicy_Thread_Unsafe>		LocalAllocator;

		private:
			static constexpr u32		THREADMSG_1_DIE		= 0xff;
			static constexpr u32		THREADMSG_1_LOAD	= 0x01;
			static constexpr u32		THREADMSG_1_UNLOAD	= 0x02;

			static constexpr u32		THREADMSG_2_CHANGE_STATUS 	= 0x01;

		private:
			struct sThreadParams
			{
				HThreadMsgR		msgqR;
				HThreadMsgW		msgqW;
				gos::Event		hEvent_started;
				const char		*baseFolder;
				asset2::Loader	*loader;
				gos::Logger		*logger;
			};

			struct sHeader
			{
				eStatusPublic	external_status;	//questa viene manipolata da TheHUB e rifletto lo stato della risorsa visto da "fuori"
				eStatusInternal	internal_status;	//questa e' ad uso interno del thread di loader. Quando ci sono significativi cambi di stato, il thread lo segnala
				u8 			pad2;				//a theHub via msgQ (vedi update) e TheHub aggiorna external_status di conseguenza
				u8 			pad3;

				u16			numHandleUsingThisAsset;
				u16 		numAssetUsingThissAsset;
				
				UID	uid;
			};

		private:
			static i16	ThreadFN_main (void *params);
			static void	ThreadFN_changeStatus (const HThreadMsgW &msgqW, sHeader *header, eStatusPublic newStatus);

		private:
			void 		priv_free ();
			void 		priv_scheduleLoad (void *pt);
			bool 		priv_findOrAddAsset (const UID &uid, void **out_pt);
			void* 		priv_getExistingAssetByUID (const UID &uid);
			void 		priv_unload (void *pt);
			void 		priv_update (u64 timeout_msec);

		private:
			LocalAllocator		*localAllocator;
			gos::Logger			*logger;
            GOSThreadHandle 	hThreadLoader;
            HThreadMsgR     	msgq_1R;
            HThreadMsgW     	msgq_1W;
            HThreadMsgR     	msgq_2R;
            HThreadMsgW     	msgq_2W;
			asset2::Loader		loader;
			u64 				lastTimeUpdateWasCalled_msec;

			HashList			knownAssetsList;	//mappa asset2::uid ad un puntatore che punta alla data-struct specifica dell'asset
			

		friend LoaderInterface;

		};

	} //namespace asset2
}//namespace gos

#endif //_gosAsset2Hub_h_