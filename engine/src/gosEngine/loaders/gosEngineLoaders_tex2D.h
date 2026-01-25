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
                bool    load (LoaderInfo &loaderInfo, asset2::UID uid, void *out_dataIN)
                {
                    ResTexture::DataForLoaderThread *out_data = static_cast<ResTexture::DataForLoaderThread*>(out_dataIN);
                    gos::Allocator *thread_allocator = loaderInfo.thread_allocator;
                    gos::GPU *gpu = loaderInfo.gpu;

                    char s[1024];
                    asset2::asset_manufacture_fullFilename (*loaderInfo.ctx, uid, s, sizeof(s));

                    gos::Image image;
                    if (!image::load (thread_allocator, s, &image))
                    {
                        logger::err ("Loader_tex2D::load() => file not found %s\n", s);
                        return false;
                    }
                    
                    const bool ret = gpu->texture_create2D (&image, 0, eMemAccessMode::onGPU, &out_data->data.texHandle);
                    image::free (thread_allocator, image);

                    //mi aggiungo alla lista degli asset noti
                    if (ret)
                        loaderInfo.listof_knownAssets->add_or_replace (uid, &out_data->data, sizeof(out_data->data));
                    return ret;           
                }
            };



        } //namespace loaders
    } //namespace engine
} //namespace gos


#endif //_gosEngineLoaders_tex2D_h_
