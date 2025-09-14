#ifndef _gosAssetLoaderInterface_h_
#define _gosAssetLoaderInterface_h_
#include "../gosAssetEnumAndDefine.h"
#include "../../gosGPU/gosGPUEnumAndDefine.h"

namespace gos
{
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
                            LoaderInterface (eAssetType assTypeIN)                                                                  { assType = assTypeIN; }
            virtual         ~LoaderInterface()                                                                                      { }


            virtual bool    load (Loader *assetLoader, const asset::Context &ctx, const asset::UID &uid, void *in_out_asset) = 0;
            virtual u32     getSizeOfData() const = 0;


            eAssetType      getAssType() const                                                                                      { return assType; }


        private:
            eAssetType assType;
        }; //class LoaderInterface



    } //namespace asset
} //namespace gos


#endif //_gosAssetLoaderInterface_h_

