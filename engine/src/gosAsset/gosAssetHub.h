#ifndef _gosAssetHub_h_
#define _gosAssetHub_h_
#include "gosAssetEnumAndDefine.h"


namespace gos
{
	namespace asset
	{
		/*******************************
		* @brief	asset::Hub
		* 
		*/
		class Hub
		{
		public:
			enum class Status : u8
			{
				ready		= 0,
				notLoaded	= 1,
				loading		= 2,
				unloading	= 3
			};

		public:
					Hub();
					~Hub();


			// false se <runtimeName> non e' un nome valido
			bool	getHandle (const char *runtimeName, Handle *out);

			//se ritorna status::ready, puoi usare la risorsa, altrimenti no, provaci al prossimo giro
			template<class TASSET>
			Status	getAsset (const Handle &h, TASSET *out_assetData)
			{
				return priv_getAsset (h, static_cast<void*>(out_assetData))
			}

			//e' un hint al gestore di risorse.. non mi serve piu' <h>, se vuoi puoi farne l'unload
			void	dispose (const Handle &h);


			//da chiamaarsi con una certa frequenza nel main loop
			void	update (u64 timenow_msec);

		private:
			Status	priv_getAsset (const Handle &h, void *out_assetData);

		};

	} //namespace asset
}//namespace gos

#endif //_gosAssetHub_h_