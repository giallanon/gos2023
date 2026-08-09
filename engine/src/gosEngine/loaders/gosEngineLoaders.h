#ifndef _gosEngineLoaders_h_
#define _gosEngineLoaders_h_
#include "../../gosAsset2/gosAsset2.h"
#include "../../gos/gosBufferReader.h"
#include "../../gos/gosDataBlob.h"
#include "../gosGPU/GOSGPUStageHelper.h"

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
            };

			struct CallbackData
			{
				res::Descr 	*res;
				void		*user_data_pt;
				u64			user_data_1;
				u64			reschedule_load_at_time_msec;
			};

            //********************************************************
            class BaseLoader
            {
			public:
				enum class eResult : u8
				{
					failed = 0,
					success = 1,
					callback = 2
				};

			public:
                                BaseLoader()        { } 
                virtual         ~BaseLoader()       { }
                virtual eResult	load (LoaderInfo &loaderInfo, void *res, CallbackData *in_out__callback_data) = 0;
				virtual	bool	load_continued (LoaderInfo &loaderInfo, bool anyError, CallbackData *callback_data)			{ return false; }
            };
        } //namespace loaders
    } //namespace engine
} //namespace gos
#endif //_gosEngineLoaders_h_
