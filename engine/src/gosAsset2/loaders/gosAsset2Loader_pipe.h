#ifndef _gosAsset2Loader_pipe_h_
#define _gosAsset2Loader_pipe_h_
#include "gosAsset2LoaderInterface.h"


namespace gos
{
    namespace asset2
    {
        /******************************************
        * @brief    Asset_pipe
        * 
        */
        class Asset_pipe
        {
        public:
            GPUPipelineHandle handle_pipe;
        };


        /******************************************
         * @brief Loader_pipe
         *
         */
        class Loader_pipe : public LoaderInterface
        {
        public:
                    Loader_pipe () : LoaderInterface (eAssetType::pipe)                     { }
            virtual ~Loader_pipe()                                                          { }

            bool    load (Loader *assetLoader, const DBContext &ctx, const UID &uid, void *in_out_asset);
            void    unload (Loader *assetLoader, const DBContext &ctx, const UID &uid, void *ptToAssetData);

            u32     getSizeOfData() const                                                   { return static_cast<u32>(sizeof(Asset_pipe)); } 

            
        }; //class Loader_pipe


       
    } //namespace asset2
} //namespace gos

#endif //_gosAsset2Loader_pipe_h_