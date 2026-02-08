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
                bool    load (LoaderInfo &loaderInfo, void *resIN)
                {
                    res::Texture2d *res = static_cast<res::Texture2d*>(resIN);
                    gos::Allocator *thread_allocator = loaderInfo.thread_allocator;
                    gos::GPU *gpu = loaderInfo.gpu;

                    char s[1024];
                    asset2::asset_manufacture_fullFilename (*loaderInfo.ctx, res->_descr.uid, s, sizeof(s));

                    gos::Image image;
                    if (!image::load (thread_allocator, s, &image))
                    {
                        logger::err ("Loader_tex2D::load() => file not found %s\n", s);
                        return false;
                    }
                    
                    const bool ret = gpu->texture_create2D (&image, 0, eMemAccessMode::onGPU, &res->texHandle, loaderInfo.stageHelper);
                    image::free (thread_allocator, image);

                    return ret;           
                }
            };



        } //namespace loaders
    } //namespace engine
} //namespace gos


#endif //_gosEngineLoaders_tex2D_h_
