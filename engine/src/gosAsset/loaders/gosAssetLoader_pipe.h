#ifndef _gosAssetLoader_pipe_h_
#define _gosAssetLoader_pipe_h_
#include "gosAssetLoaderInterface.h"
#include "../../gosGPU/gosGPU.h"

namespace gos
{
    namespace asset
    {
        /**
        * @brief    Asset_pipe
        * 
        */
        class Asset_pipe
        {
        public:
            GPUPipelineHandle handle_pipe;
        };


        /**
         * @brief Loader_pipe
         *
         */
        class Loader_pipe : public LoaderInterface
        {
        public:
                    Loader_pipe () : LoaderInterface (eAssetType::pipe)                     { }
            virtual ~Loader_pipe()                                                          { }

            bool    load (Loader *assetLoader, const asset::Context &ctx, const asset::UID &uid, void *in_out_asset);
            void    unload (Loader *assetLoader, const asset::Context &ctx, const asset::UID &uid, void *ptToAssetData);

            u32     getSizeOfData() const                                                   { return static_cast<u32>(sizeof(Asset_pipe)); } 

            
        }; //class Loader_pipe


       
    } //namespace asset
} //namespace gos

#endif //_gosAssetLoader_pipe_h_