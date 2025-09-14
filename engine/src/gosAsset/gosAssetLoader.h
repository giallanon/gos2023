#ifndef _gosAssetLoader_h_
#define _gosAssetLoader_h_
#include "loaders/gosAssetLoader_shader.h"
#include "loaders/gosAssetLoader_pipe.h"

namespace gos
{
    namespace asset
    {
        class Hub; //fwd decl


        /**
        * @brief    Loader;
        * 
        */
        class Loader
        {
        public:
                    Loader ();
                    ~Loader();

            bool    setup (const char *baseFolder, gos::GPU *gpu, asset::Hub *theHub);


            template<class TLOADER>
            bool    addLoader ()
                    {
                        TLOADER *loader = GOSNEW(localAllocator, TLOADER)();
                        if (priv_addLoader(loader))
                            return true;
                        GOSDELETE(localAllocator, loader);
                        return false;
                    }


            bool    load (const asset::UID &uid, void *ptToAssetData)
            {
                const eAssetType assType = uid.getAssetType();
                LoaderInterface *loader = getLoader (assType);
                if (loader)
                    return loader->load (this, ctx, uid, ptToAssetData);
                return false;
            }

            void    unload (const asset::UID &uid, void *ptToAssetData)
            {
                const eAssetType assType = uid.getAssetType();
                LoaderInterface *loader = getLoader (assType);
                if (loader)
                    loader->unload (this, ctx, uid, ptToAssetData);
            }            

            template<class TASSET>
            bool    load (const asset::UID &uid, TASSET *ptToAssetData)
            {
                const eAssetType assType = uid.getAssetType();
                LoaderInterface *loader = getLoader (assType);
                if (NULL == loader)
                {
                    DBGBREAK;
                    return false;
                }

                if (loader->load (theHub, this, ctx, uid, ptToAssetData))
                    return true;

                gos::logger::err ("asset::Loader::Load (%016" PRIX64 ") => asset not found\n", uid._uid);
                return false;
            }

            template<class TASSET>
            bool    load (const char *runtimeName, TASSET *ptToAssetData)
            {
                asset::UID uid;
                if (!runtimeNameToUID(runtimeName, &uid))
                {
                    gos::logger::err ("asset::Loader::Load('%s') => asset not found\n", runtimeName);
                    return false;
                }
                return load<TASSET>(uid, ptToAssetData);
            }

            template<class TASSET>
            bool    load (const u64 uidIN, TASSET *ptToAssetData)
            {
                asset::UID uid;
                uid._uid = uidIN;
                return load<TASSET>(uid, ptToAssetData);
            }


            gos::GPU*           getGPU() const                                                      { return gpu; }
            LoaderInterface*    getLoader (eAssetType assType);
            bool                runtimeNameToUID (const char *runtimeName, asset::UID *out);
            asset::Context*     getContext()                                                        { return &ctx;}
            asset::Hub*         getTheHub() const                                                   { return theHub; }

        private:
            static constexpr u8 NUM_MAX_ASSET_LOADER = 32;

        private:
            bool    priv_addLoader (LoaderInterface *loader);

        private:
            gos::Allocator      *localAllocator;
            asset::Hub          *theHub;
            gos::GPU            *gpu;
            asset::Context      ctx;
            LoaderInterface     *loaderList[NUM_MAX_ASSET_LOADER];
        };


    } //namespace asset
} //namespace gos

#endif //_gosAssetLoader_h_