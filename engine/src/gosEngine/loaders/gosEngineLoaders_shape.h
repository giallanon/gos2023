#ifndef _gosEngineLoaders_shape_h_
#define _gosEngineLoaders_shape_h_
#include "gosEngineLoaders_shader.h"
#include "../../gosShape/gosShape.h"

namespace gos
{
    namespace engine
    {
        namespace loaders
        {
            //********************************************************
            class Loader_shape : public loaders::BaseLoader
            {
            public:
                bool    load (LoaderInfo &loaderInfo, asset2::UID uid, void *out_dataIN)
                {
                    ResShape::DataForLoaderThread *out_data = static_cast<ResShape::DataForLoaderThread*>(out_dataIN);
					gos::Allocator *thread_allocator = loaderInfo.thread_allocator;

                    char s[1024];
                    asset2::asset_manufacture_fullFilename (*loaderInfo.ctx, uid, s, sizeof(s));

                    u32 fsize;
                    u8 *buffer = fs::fileLoadInMemory (thread_allocator, s, &fsize);
                    if (NULL == buffer)
                    {
                        logger::err ("Loader_shape::load() => file not found %s\n", s);
                        return false;
                    }

					if (!shape::deserialize (buffer, fsize, loaderInfo.engine_allocator, &out_data->data.shape))
                    {
						GOSFREE(thread_allocator, buffer);
                        logger::err ("Loader_shape::load() => error creating shape from %s\n", s);
                        return false;
                    }
					GOSFREE(thread_allocator, buffer);


                    //mi aggiungo alla lista degli asset noti
					loaderInfo.listof_knownAssets->add_or_replace (uid, &out_data->data, sizeof(out_data->data));

                    return true;
                }
            };


        } //namespace loaders
    } //namespace engine
} //namespace gos


#endif //_gosEngineLoaders_shape_h_
