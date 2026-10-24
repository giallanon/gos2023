#ifndef _gosEngine_res2_loaders_Loader_h_
#define _gosEngine_res2_loaders_Loader_h_
#include "../../../gosAsset2/gosAsset2.h"
#include "../../../gos/gosBufferReader.h"
#include "../../../gos/gosDataBlob.h"
#include "../../../gosGPU/GOSGPUStageHelper.h"
#include "../res2_res.h"


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
			u32 			user_1;
			u32 			user_2;
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

			virtual eResult	load (LoaderInfo &loaderInfo, res2::ResDescr *descr, CallbackData_onLoad *in_out__callback_data) = 0;
			virtual eResult	load_continued (LoaderInfo &loaderInfo, CallbackData_onLoad *callback_data) 							{ return eResult::failed; }
			virtual void	load_finished (LoaderInfo &loaderInfo, CallbackData_onLoad *callback_data)								{ }
		};
	} //namespace res2
} //namespace gos
#endif //_gosEngine_res2_loaders_Loader_h_
