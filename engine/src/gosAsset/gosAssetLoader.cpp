#include "gosAssetLoader.h"
#include "gosAsset.h"

using namespace gos;
using namespace gos::asset;


//************************************************
Loader::Loader ()
{
    localAllocator = gos::getSysHeapAllocator();

	memset (loaderList, 0, sizeof(loaderList));
    addLoader<Loader_vtxShader>();
    addLoader<Loader_pxlShader>();
}

//************************************************
Loader::~Loader()
{
    for (u32 i=0; i<NUM_MAX_ASSET_LOADER; i++)
    {
        if (NULL == loaderList[i])
            continue;
        GOSDELETE(localAllocator, loaderList[i]);
    }

	asset::context_close (ctx);
}

//************************************************
bool Loader::setup (const char *baseFolder, gos::GPU *gpuIN)
{
    if (ctx.isValid())
        asset::context_close (ctx);

    gpu = gpuIN;
	return asset::context_open (baseFolder, &ctx);
}

//************************************************
bool Loader::priv_addLoader (LoaderInterface *loader)
{
    assert (NULL != loader);
    
    const u32 index = static_cast<u8>(loader->getAssType());
    assert (index < NUM_MAX_ASSET_LOADER);

    if (NULL == loaderList[index])
    {
        loaderList[index] = loader;
        return true;
    }
    
    gos::logger::err ("asset::Loader::priv_addLoader() => a loader for res %s already exists\n", asset::enumToString(loader->getAssType()));
    return false;
}

//***********************************
LoaderInterface* Loader::getLoader (eAssetType assType)
{
    const u32 index = static_cast<u8>(assType);
    assert (index < NUM_MAX_ASSET_LOADER);
    return loaderList[index];
}

//************************************************
bool Loader::runtimeNameToUID (const char *runtimeName, asset::UID *out)
{
    return asset::rtname_exists (ctx, runtimeName, out);
}
