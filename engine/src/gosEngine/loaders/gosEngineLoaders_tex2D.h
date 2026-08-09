#ifndef _gosEngineLoaders_tex2D_h_
#define _gosEngineLoaders_tex2D_h_
#include "gosEngineLoaders.h"

namespace gos
{
    namespace engine
    {
        namespace loaders
        {
            //********************************************************
            class Loader_tex2D : public loaders::BaseLoader
            {
            public:
                eResult load (LoaderInfo &loaderInfo, void *resIN, CallbackData *in_out__callback_data)
                {
                    res::Texture2d *res = static_cast<res::Texture2d*>(resIN);
                    gos::Allocator *thread_allocator = loaderInfo.thread_allocator;

                    char s[1024];
                    asset2::asset_manufacture_fullFilename (*loaderInfo.ctx, res->_descr.uid, s, sizeof(s));

                    gos::Image image;
                    if (!image::load (thread_allocator, s, &image))
                    {
                        logger::err ("Loader_tex2D::load() => file not found %s\n", s);
                        return eResult::failed;
                    }

					//torno all'engine per creare la texture
					in_out__callback_data->user_data_pt = image.p;
					return eResult::callback;
                }

				bool	load_continued (LoaderInfo &loaderInfo, bool anyError, CallbackData *callback_data)
				{
					gos::Allocator *thread_allocator = loaderInfo.thread_allocator;

					gos::Image image;
					image.p = callback_data->user_data_pt;
					image::free (thread_allocator, image);

                    return !anyError;
				}	

                // eResult load (LoaderInfo &loaderInfo, void *resIN, CallbackData *in_out__callback_data)
                // {
                //     res::Texture2d *res = static_cast<res::Texture2d*>(resIN);
                //     gos::Allocator *thread_allocator = loaderInfo.thread_allocator;
                //     gos::GPU *gpu = loaderInfo.gpu;

                //     char s[1024];
                //     asset2::asset_manufacture_fullFilename (*loaderInfo.ctx, res->_descr.uid, s, sizeof(s));

                //     gos::Image image;
                //     if (!image::load (thread_allocator, s, &image))
                //     {
                //         logger::err ("Loader_tex2D::load() => file not found %s\n", s);
                //         return eResult::failed;
                //     }
                    
                //     const bool ret = gpu->texture_create2D (&image, 0, eMemAccessMode::onGPU, &res->texHandle, loaderInfo.stageHelper);
                //     image::free (thread_allocator, image);

				// 	if (ret)
				// 		return eResult::success;
				// 	return eResult::failed;
                // }
            };



        } //namespace loaders
    } //namespace engine
} //namespace gos


#endif //_gosEngineLoaders_tex2D_h_
