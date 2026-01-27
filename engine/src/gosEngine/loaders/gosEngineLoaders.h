#ifndef _gosEngineLoaders_h_
#define _gosEngineLoaders_h_
#include "../../gosAsset2/gosAsset2.h"
#include "../../gos/gosBufferReader.h"
#include "../../gos/gosDataBlob.h"
#include "../gosEngineRes.h"

namespace gos
{
    class Engine; //fwd

    namespace engine
    {
        namespace loaders
        {
            //********************************************************
            struct LoaderInfo
            {
                gos::Allocator      *thread_allocator;
				gos::Allocator		*engine_allocator;
				Engine				*engine;
                gos::Logger			*logger;
                gos::GPU            *gpu;
                asset2::DBContext   *ctx;
				gpu::StageHelper	stageHelper;
            };


            //********************************************************
            class BaseLoader
            {
            public:
                                BaseLoader()        { } 
                virtual         ~BaseLoader()       { }
                virtual bool    load (LoaderInfo &loaderInfo, asset2::UID uid, void *res_data) = 0;
            };
        } //namespace loaders
    } //namespace engine
} //namespace gos
#endif //_gosEngineLoaders_h_
