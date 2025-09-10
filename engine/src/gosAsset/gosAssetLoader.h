#ifndef _gosAssetLoader_h_
#define _gosAssetLoader_h_
#include "loaders/gosAssetLoader_shader.h"


namespace gos
{
    namespace asset
    {
        /**
        * @brief    Loader;
        * 
        */
        class Loader
        {
        public:
                    Loader ();
                    ~Loader();

            bool    setup (const char *baseFolder, gos::GPU *gpu);


            template<class TLOADER>
            bool    addLoader ()
                    {
                        TLOADER *loader = GOSNEW(localAllocator, TLOADER)();
                        if (priv_addLoader(loader))
                            return true;
                        GOSDELETE(localAllocator, loader);
                        return false;
                    }


            template<class TASSET>
            bool    load (const asset::UID &uid, TASSET *out)
            {
                const eAssetType assType = uid.getAssetType();
                LoaderInterface *loader = priv_getLoader (assType);
                if (NULL == loader)
                {
                    DBGBREAK;
                    return false;
                }

                out->uid = uid;
                if (loader->load (this, ctx, out))
                    return true;

                gos::logger::err ("asset::Loader::Load (%016" PRIX64 ") => asset not found\n", uid._uid);
                return false;
            }

            template<class TASSET>
            bool    load (const char *runtimeName, TASSET *out)
            {
                asset::UID uid;
                if (!priv_runtimeNameToUID(runtimeName, &uid))
                {
                    gos::logger::err ("asset::Loader::Load('%s') => asset not found\n", runtimeName);
                    return false;
                }
                return load<TASSET>(uid, out);
            }

            template<class TASSET>
            bool    load (const u64 uidIN, TASSET *out)
            {
                asset::UID uid;
                uid._uid = uidIN;
                return load<TASSET>(uid, out);
            }



            gos::GPU*   getGPU() const                                      { return gpu; }


        private:
            static constexpr u8 NUM_MAX_ASSET_LOADER = 32;

        private:
            bool    priv_addLoader (LoaderInterface *loader);
            LoaderInterface*   priv_getLoader (eAssetType assType);
            bool    priv_runtimeNameToUID (const char *runtimeName, asset::UID *out);

        private:
            gos::Allocator      *localAllocator;
            gos::GPU            *gpu;
            asset::Context      ctx;
            LoaderInterface     *loaderList[NUM_MAX_ASSET_LOADER];
        };


    } //namespace asset
} //namespace gos

#endif //_gosAssetLoader_h_