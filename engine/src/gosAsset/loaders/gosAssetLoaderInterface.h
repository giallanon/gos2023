#ifndef _gosAssetLoaderInterface_h_
#define _gosAssetLoaderInterface_h_
#include "../gosAssetEnumAndDefine.h"
#include "../../gosGPU/gosGPUEnumAndDefine.h"


namespace gos
{
    /**
     * @brief   Asset
     *
     */
    class Asset
    {
    public:
                     Asset()                 { }
        virtual     ~Asset()                { }

    public:
        asset::UID  uid;
    };



    namespace asset
    {
        class Loader;   //fwd decl


        /**
         * @brief   LoaderInterface
         *
         */
        class LoaderInterface
        {
        public:
            LoaderInterface (eAssetType assTypeIN) { assType = assTypeIN; }
            virtual         ~LoaderInterface() { }

            eAssetType      getAssType() const { return assType; }
            virtual bool    load (Loader *assetLoader, const asset::Context &ctx, void *in_out_asset) = 0;

        private:
            eAssetType assType;
        }; //class LoaderInterface

    } //namespace asset
} //namespace gos


#endif //_gosAssetLoaderInterface_h_

