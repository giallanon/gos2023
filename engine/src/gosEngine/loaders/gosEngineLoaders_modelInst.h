#ifndef _gosEngineLoaders_modelInst_h_
#define _gosEngineLoaders_modelInst_h_
#include "gosEngineLoaders.h"

namespace gos
{
    namespace engine
    {
        namespace loaders
        {
            //********************************************************
            class Loader_modelInst : public loaders::BaseLoader
            {
            public:
                eResult load (LoaderInfo &loaderInfo, void *resIN, CallbackData *in_out__callback_data)
                {
					//sembra un po' uno spreco, ma devo tornare nell'engine per allocare
					//la delle strutture
					return eResult::callback;
                }

				bool	load_continued (LoaderInfo &loaderInfo, bool anyError, CallbackData *callback_data)
				{
                    return !anyError;
				}				
            };


        } //namespace loaders
    } //namespace engine
} //namespace gos

#endif //_gosEngineLoaders_modelInst_h_

