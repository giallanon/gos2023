#ifndef _gosEngineLoaders_skeleton_h_
#define _gosEngineLoaders_skeleton_h_
#include "gosEngineLoaders_shader.h"
#include "../../gosShape/skeleton/gosSkeleton.h"

namespace gos
{
    namespace engine
    {
        namespace loaders
        {
            //********************************************************
            class Loader_skeleton : public loaders::BaseLoader
            {
            public:
                bool    load (LoaderInfo &loaderInfo, asset2::UID uid, void *out_dataIN)
                {
                    ResSkeleton::DataForLoaderThread *out_data = static_cast<ResSkeleton::DataForLoaderThread*>(out_dataIN);
					gos::Allocator *thread_allocator = loaderInfo.thread_allocator;

                    char s[1024];
                    asset2::asset_manufacture_fullFilename (*loaderInfo.ctx, uid, s, sizeof(s));

                    u32 fsize;
                    u8 *buffer = fs::fileLoadInMemory (thread_allocator, s, &fsize);
                    if (NULL == buffer)
                    {
                        logger::err ("Loader_skeleton::load() => file not found %s\n", s);
                        return false;
                    }

					u32 n;
					out_data->data.skeleton = Skeleton::createFromMemory (loaderInfo.engine_allocator, buffer, fsize, &n);
					if (NULL == out_data->data.skeleton)
                    {
						GOSFREE(thread_allocator, buffer);
                        logger::err ("Loader_skeleton::load() => error creating skeleton from %s\n", s);
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


#endif //_gosEngineLoaders_skeleton_h_
