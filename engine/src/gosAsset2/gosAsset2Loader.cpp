#include "gosAsset2Loader.h"
#include "gosAsset2.h"
#include "gosAsset2Hub.h"

using namespace gos;
using namespace gos::asset2;


//************************************************
Loader::Loader ()
{
    localAllocator = gos::getSysHeapAllocator();

	memset (loaderList, 0, sizeof(loaderList));
    addLoader<Loader_vtxShader>();
    addLoader<Loader_pxlShader>();
    addLoader<Loader_pipe>();
    addLoader<Loader_tex2D>();
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

	asset2::dbcontext_close (ctx);
}

//************************************************
bool Loader::setup (const char *baseFolder, gos::GPU *gpuIN, asset2::Hub *theHubIN)
{
    if (ctx.isValid())
        asset2::dbcontext_close (ctx);

    gpu = gpuIN;
    theHub = theHubIN;

    //il database degli asset deve esistere di gia'
	return asset2::dbcontext_open (baseFolder, false, &ctx);
}

//************************************************
bool Loader::priv_addLoader (LoaderInterface *loader)
{
    assert (NULL != loader);
    
    const u32 index = static_cast<u8>(loader->getAssetType());
    assert (index < NUM_MAX_ASSET_LOADER);

    if (NULL == loaderList[index])
    {
        loaderList[index] = loader;
        return true;
    }
    
    gos::logger::err ("asset2::Loader::priv_addLoader() => a loader for res %s already exists\n", asset2::enumToString(loader->getAssetType()));
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
bool Loader::runtimeNameToUID (const char *runtimeName, UID *out)
{
    return asset2::asset_getBy_rtname (ctx, runtimeName, out);
}
