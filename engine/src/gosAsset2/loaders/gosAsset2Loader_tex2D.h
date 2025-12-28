#ifndef _gosAsset2Loader_tex2D_h_
#define _gosAsset2Loader_tex2D_h_
#include "gosAsset2LoaderInterface.h"


namespace gos
{
    namespace asset2
    {
        /****************************************
        * @brief    Asset_tex2D
        * 
        */
        class Asset_tex2D
        {
        public:
            GPUTextureHandle handle_texture;
        };


        /****************************************
         * @brief Loader_tex2D
         *
         */
        class Loader_tex2D : public LoaderInterface
        {
        public:
                    Loader_tex2D () : LoaderInterface (eAssetType::tex2D)                       { }
            virtual ~Loader_tex2D()                                                             { }

            bool    load (Loader *assetLoader, const DBContext &ctx, const UID &uid, void *in_out_asset);
            void    unload (Loader *assetLoader, const DBContext &ctx, const UID &uid, void *ptToAssetData);

            u32     getSizeOfData() const                                                   { return static_cast<u32>(sizeof(Asset_tex2D)); } 

            
        }; //class Loader_tex2D


       
    } //namespace asset2
} //namespace gos

#endif //_gosAsset2Loader_tex2D_h_