#include "gosAsset2Loader_shader.h"
#include "../gosAsset2Hub.h"
#include "../gosAsset2Loader.h"
#include "../gosAsset2.h"


using namespace gos;
using namespace gos::asset2;


//******************************************************
bool Loader_shader::load (Loader *assetLoader, const DBContext &ctx, const UID &uid, void *in_out_asset)
{
	Asset_shader *out = static_cast <Asset_shader*>(in_out_asset);
	assert (uid.isValid());
	assert (uid.isAnAssetOfType(this->getAssetType()));


	char s[1024];
	asset_manufacture_fullFilename (ctx, uid, s, sizeof(s));


	gos::GPU *gpu = assetLoader->getGPU();
	switch (this->getAssetType())
	{
	default:
		gos::logger::err ("asset2::Loader_shader => invalid asset type\n"); return false;
	case eAssetType::vtx_shader:	return gpu->vtxshader_createFromFile (s, "main", &out->handle_shader);
	case eAssetType::pxl_shader:	return gpu->pxlshader_createFromFile (s, "main", &out->handle_shader);
	}
}

//******************************************************
void Loader_shader::unload (Loader *assetLoader, const DBContext &ctx, const UID &uid, void *ptToAssetData)
{
	Asset_shader *asset = static_cast <Asset_shader*>(ptToAssetData);
	assetLoader->getGPU()->deleteResource (asset->handle_shader);
}