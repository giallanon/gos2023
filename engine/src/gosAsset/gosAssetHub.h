#ifndef _gosAssetHub_h_
#define _gosAssetHub_h_
#include "gosAssetLoader.h"
#include "../gos/memory/gosAllocatorHeap.h"

namespace gos
{
	namespace asset
	{
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
				notLoaded	= 1,
				loading		= 2,
				unloading	= 3,
				error 		= 0xff
			};

		public:
					Hub();
					~Hub();

			bool    setup (const char *baseFolder, gos::GPU *gpu);

			// false se <runtimeName> non e' un nome valido
			bool	getHandle (const char *runtimeName, Handle *out);

			//se ritorna status::ready, puoi usare la risorsa, altrimenti no, provaci al prossimo giro
			template<class TASSET>
			const TASSET* getAsset (const Handle &h)
			{
				void *pData = (void*) (static_cast<const u8*>(h._pt) + sizeof(sHeader));
				const sHeader *header = static_cast<const sHeader*>(h._pt);
				if (eStatus::ready == header->status)
				{
					return static_cast<const TASSET*> (pData);
				}

				TASSET *p = static_cast<TASSET*>( pData);
				loader.load<TASSET>(header->uid, p);

				return p;
			}

			//e' un hint al gestore di risorse.. non mi serve piu' <h>, se vuoi puoi farne l'unload
			//void	dispose (const Handle &h);


			//da chiamaarsi con una certa frequenza nel main loop
			//void	update (u64 timenow_msec);

		private:
			typedef HashMap<u64, void*> HashList;

			typedef gos::AllocatorHeap<gos::AllocPolicy_Track_simple, gos::AllocPolicy_Thread_Unsafe>		LocalAllocator;

			struct sHeader
			{
				eStatus 	status;
				u8 	pad0;
				u8 	pad1;
				u8 	pad2;
				u32 pad3;
				asset::UID	uid;
			};

		private:
			eStatus	priv_getAsset (const Handle &h, const void **out_assetData) const;

		private:
			LocalAllocator		*localAllocator;
			HashList			knownAssetsList;	//mappa asset::uid ad un puntatore che punta alla data-struct specifica dell'asset
			asset::Loader		loader;



		};

	} //namespace asset
}//namespace gos

#endif //_gosAssetHub_h_