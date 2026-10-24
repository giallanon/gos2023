#ifndef _gosEngine_res2_loaders_Loaders_gpushape_h_
#define _gosEngine_res2_loaders_Loaders_gpushape_h_
#include "Loader.h"

namespace gos
{
	namespace res2
	{
		//********************************************************
		class Loader_gpushape: public res2::BaseLoader
		{
		public:
			eResult load (LoaderInfo &loaderInfo, res2::ResDescr *descrIN, CallbackData_onLoad *in_out__callback_data)
			{
				gos::Allocator *thread_allocator = loaderInfo.thread_allocator;

				char s[1024];
				asset2::signature_manufacture_fullFilename (*loaderInfo.ctx, descrIN->UID, s, sizeof(s));

				u32 fsize;
				u8 *buffer = gos::fs::fileLoadInMemory (thread_allocator, s, &fsize);
				if (NULL == buffer)
				{
					logger::err ("Loader_gpushape::load() => file not found %s\n", s);
					return eResult::failed;
				}

				//torno all'engine per creare la risorsa
				in_out__callback_data->user_data_pt = buffer;
				in_out__callback_data->user_1 = fsize;
				return eResult::callback;
			}

			void load_finished (LoaderInfo &loaderInfo, CallbackData_onLoad *callback_data)
			{
				gos::Allocator *thread_allocator = loaderInfo.thread_allocator;
				GOSFREE(thread_allocator, callback_data->user_data_pt);
			}	

		};
    } //namespace res2
} //namespace gos



#endif //_gosEngine_res2_loaders_Loaders_gpushape_h_
