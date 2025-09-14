#ifndef _gosAssetHub_h_
#define _gosAssetHub_h_
#include "gosAssetLoader.h"
#include "../gos/memory/gosAllocatorHeap.h"
#include "../gos/gosThreadMsgQ.h"
#include "../gosGPU/gosGPU.h"



namespace gos
{
	namespace asset
	{
		class Loader; //fwd decl

        /**
        * @brief    asset::Handle
        * 
        */
        struct Handle
        {
            void *_pt;
        };
		
		
		/*******************************
		* @brief	asset::Hub
		* 
		*/
		class Hub
		{
		public:
			enum class eStatus : u8
			{
				ready		= 0,
				notLoaded	= 1,		//ho allocato spazio in RAM ma non ho mai chiamato il suo loader
				loading		= 2,		//il loader la sta caricando
				unloading	= 3,		//il loader la sta scaricando
				error 		= 0xff		//errore fatale, probabilmente il loader non e' riuscito a caricarla, questo asset e' spacciato per sempre
			};

		public:
					Hub();
					~Hub()																	{ priv_free(); }

			bool    setup (const char *baseFolder, gos::GPU *gpu);
			void 	update (u64 timenow_msec);

			// false se <runtimeName> non e' un nome valido
			bool	getHandle (const char *runtimeName, Handle *out, bool bScheduleLoadNow = false);

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

				case eStatus::ready:
					{
						void *ptToAsset = (void*) (static_cast<const u8*>(h._pt) + sizeof(sHeader));
						*out = static_cast<const TASSET*> (ptToAsset);
					}
					return true;

				case eStatus::notLoaded:
					//dato che mi hanno chiesto l'asset, e questo non era caricato, 
					//schedulo il caricamento ora
					priv_scheduleLoad (h._pt);
					return false;

				case eStatus::loading:
					return false;

				case eStatus::unloading:
				case eStatus::error:
					return false;
				}
			}



			template<class TASSET>
			bool 	internalUSE_getExistingAssetByUID (const asset::UID &uid, const TASSET **out)
			{
				void *pt = NULL;
				if (knownAssetsList.find(uid, &pt))
				{
#ifdef _DEBUG
					const sHeader *header = static_cast<sHeader*>(pt);
					assert (eStatus::ready == header->internal_status);
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
			typedef HashMap<asset::UID, void*> HashList;

			typedef gos::AllocatorHeap<gos::AllocPolicy_Track_simple, gos::AllocPolicy_Thread_Unsafe>		LocalAllocator;

		private:
			static constexpr u32		THREADMSG_1_DIE		= 0xff;
			static constexpr u32		THREADMSG_1_LOAD	= 0x01;

			static constexpr u32		THREADMSG_2_CHANGE_STATUS 	= 0x01;

		private:
			struct sThreadParams
			{
				HThreadMsgR		msgqR;
				HThreadMsgW		msgqW;
				gos::Event		hEvent_started;
				const char		*baseFolder;
				asset::Loader	*loader;
				gos::Logger		*logger;
			};

			struct sHeader
			{
				eStatus 	external_status;	//questa viene manipolata da TheHUB e rifletto lo stato della risorsa visto da "fuori"
				eStatus 	internal_status;	//questa e' ad uso interno del thread di loader. Quando ci sono significativi cambi di stato, il thread lo segnala
				u8 			pad2;				//a theHub via msgQ (vedi update) e TheHub aggiorna external_status di conseguenza
				u8 			pad3;

				u16			numHandleUsingThisAsset;
				u16 		numAssetUsingThissAsset;
				
				asset::UID	uid;
			};

		private:
			static i16	ThreadFN_main (void *params);
			static void	ThreadFN_changeStatus (const HThreadMsgW &msgqW, sHeader *header, eStatus newStatus);

		private:
			void 		priv_free ();
			void 		priv_scheduleLoad (void *pt);
			bool 		priv_findOrAddAsset (const asset::UID &uid, void **out_pt);

		private:
			LocalAllocator		*localAllocator;
			gos::Logger			*logger;
            GOSThreadHandle 	hThreadLoader;
            HThreadMsgR     	msgq_1R;
            HThreadMsgW     	msgq_1W;
            HThreadMsgR     	msgq_2R;
            HThreadMsgW     	msgq_2W;
			asset::Loader		loader;
			u64 				lastTimeUpdateWasCalled_msec;

			asset::FastUIDList  fastUIDList;
			HashList			knownAssetsList;	//mappa asset::uid ad un puntatore che punta alla data-struct specifica dell'asset
			

		friend LoaderInterface;

		};

	} //namespace asset
}//namespace gos

#endif //_gosAssetHub_h_