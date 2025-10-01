#ifndef _gosAssetLoader_tex2D_h_
#define _gosAssetLoader_tex2D_h_
#include "gosAssetLoaderInterface.h"
#include "../../gosGPU/gosGPU.h"

namespace gos
{
    namespace asset
    {
        /**
        * @brief    Asset_tex2D
        * 
        */
        class Asset_tex2D
        {
        public:
            GPUTextureHandle handle_texture;
        };


        /**
         * @brief Loader_tex2D
         *
         */
        class Loader_tex2D : public LoaderInterface
        {
        public:
                    Loader_tex2D () : LoaderInterface (eAssetType::tex2D)                       { }
            virtual ~Loader_tex2D()                                                             { }

            bool    load (Loader *assetLoader, const asset::Context &ctx, const asset::UID &uid, void *in_out_asset);
            void    unload (Loader *assetLoader, const asset::Context &ctx, const asset::UID &uid, void *ptToAssetData);

            u32     getSizeOfData() const                                                   { return static_cast<u32>(sizeof(Asset_tex2D)); } 

            
        }; //class Loader_tex2D


       
    } //namespace asset
} //namespace gos

#endif //_gosAssetLoader_tex2D_h_