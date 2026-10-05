#ifndef _gosEngine_res2_loaders_Loaders_tex2D_h_
#define _gosEngine_res2_loaders_Loaders_tex2D_h_
#include "Loader.h"

namespace gos
{
	namespace res2
	{
		//********************************************************
		class Loader_tex2D : public res2::BaseLoader
		{
		public:
			bool load (LoaderInfo &loaderInfo, res2::ResDescr *descrIN, CallbackData_onLoad *in_out__callback_data)
			{
				gos::Allocator *thread_allocator = loaderInfo.thread_allocator;

				char s[1024];
				asset2::signature_manufacture_fullFilename (*loaderInfo.ctx, descrIN->signatureUID, s, sizeof(s));

				gos::Image image;
				if (!image::load (thread_allocator, s, &image))
				{
					logger::err ("Loader_tex2D::load() => file not found %s\n", s);
					return false;
				}

				//torno all'engine per creare la texture
				in_out__callback_data->user_data_pt = image.p;
				return true;
			}

			void load_finished (LoaderInfo &loaderInfo, CallbackData_onLoad *callback_data)
			{
				gos::Allocator *thread_allocator = loaderInfo.thread_allocator;

				gos::Image image;
				image.p = callback_data->user_data_pt;
				image::free (thread_allocator, image);
			}	

		};
    } //namespace res2
} //namespace gos


#endif //_gosEngine_res2_loaders_Loaders_tex2D_h_
