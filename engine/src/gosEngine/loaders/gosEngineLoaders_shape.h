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
                eResult load (LoaderInfo &loaderInfo, void *resIN, CallbackData *in_out__callback_data)
                {
                    res::Shape *res = static_cast<res::Shape*>(resIN);
					gos::Allocator *thread_allocator = loaderInfo.thread_allocator;

                    char s[1024];
                    asset2::asset_manufacture_fullFilename (*loaderInfo.ctx, res->_descr.uid, s, sizeof(s));

                    u32 fsize;
                    u8 *buffer = fs::fileLoadInMemory (thread_allocator, s, &fsize);
                    if (NULL == buffer)
                    {
                        logger::err ("Loader_shape::load() => file not found %s\n", s);
                        return eResult::failed;
                    }

					if (!shape::deserialize (buffer, fsize, loaderInfo.engine_allocator, &res->shape))
                    {
						GOSFREE(thread_allocator, buffer);
                        logger::err ("Loader_shape::load() => error creating shape from %s\n", s);
                        return eResult::failed;
                    }
					GOSFREE(thread_allocator, buffer);

                    return eResult::success;
                }
            };


        } //namespace loaders
    } //namespace engine
} //namespace gos


#endif //_gosEngineLoaders_shape_h_
