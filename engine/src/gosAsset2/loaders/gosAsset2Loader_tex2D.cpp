#include "gosAsset2Loader_tex2D.h"
#include "../gosAsset2Loader.h"
#include "../gosAsset2Hub.h"
#include "../gosAsset2.h"
#include "../gosImage/gosImage.h"

using namespace gos;
using namespace gos::asset2;

//******************************************************
void Loader_tex2D::unload (Loader *assetLoader, const DBContext &ctx, const UID &uid, void *ptToAssetData)
{
	Asset_tex2D *asset = static_cast <Asset_tex2D*>(ptToAssetData);
	
    gos::GPU *gpu = assetLoader->getGPU();
    gpu->deleteResource (asset->handle_texture);
}

//******************************************************
bool Loader_tex2D::load (Loader *assetLoader, const DBContext &ctx, const UID &uid, void *in_out_asset)
{
	Asset_tex2D *out = static_cast <Asset_tex2D*>(in_out_asset);
	assert (uid.isValid());
	assert (uid.isAnAssetOfType(this->getAssetType()));


	char s[1024];
	asset_manufacture_fullFilename (ctx, uid, s, sizeof(s));

    gos::Image image;
    if (!image::load (gos::getScrapAllocator(), s, &image))
    {
        logger::err ("Loader_tex2D::load() => file not found %s\n", s);
        return false;
    }

    gos::GPU *gpu = assetLoader->getGPU();
    const bool ret = gpu->texture_create2D (&image, 0, eMemAccessMode::onGPU, &out->handle_texture);
    image::free (gos::getScrapAllocator(), image);
    return ret;    
}