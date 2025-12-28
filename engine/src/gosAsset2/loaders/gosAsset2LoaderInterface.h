#ifndef _gosAsset2LoaderInterface_h_
#define _gosAsset2LoaderInterface_h_
#include "../gosAsset2EnumAndDefine.h"
#include "../../gosGPU/gosGPU.h"
#include "../../gosGPU/gosGPUEnumAndDefine.h"

namespace gos
{
    namespace asset2
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


            virtual bool    load (Loader *assetLoader, const DBContext &ctx, const UID &uid, void *in_out_asset) = 0;
            virtual void    unload (Loader *assetLoader, const DBContext &ctx, const UID &uid, void *ptToAssetData) = 0;
            virtual u32     getSizeOfData() const = 0;


            eAssetType      getAssetType() const                                                                                    { return assType; }


        private:
            eAssetType assType;
        }; //class LoaderInterface



    } //namespace asset2
} //namespace gos


#endif //_gosAssetLoaderInterface_h_

