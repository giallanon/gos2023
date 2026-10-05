#ifndef _gosEngine_res2_loaders_Loader_h_
#define _gosEngine_res2_loaders_Loader_h_
#include "../../../gosAsset2/gosAsset2.h"
#include "../../../gos/gosBufferReader.h"
#include "../../../gos/gosDataBlob.h"
#include "../../../gosGPU/GOSGPUStageHelper.h"
#include "../res2_res.h"
#include "../res2_asset.h"

namespace gos
{
    class Engine; //fwd

	namespace res2
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

		struct CallbackData_onLoad
		{
			res2::ResDescr 	*descr;
			void			*user_data_pt;
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

			virtual bool	load (LoaderInfo &loaderInfo, res2::ResDescr *descr, CallbackData_onLoad *in_out__callback_data) = 0;
			virtual void	load_finished (LoaderInfo &loaderInfo, CallbackData_onLoad *callback_data) = 0;
		};
	} //namespace res2
} //namespace gos
#endif //_gosEngine_res2_loaders_Loader_h_
