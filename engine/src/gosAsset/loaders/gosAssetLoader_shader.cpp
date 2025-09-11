#include "gosAssetLoader_shader.h"
#include "../gosAssetLoader.h"
#include "../gosAsset.h"


using namespace gos;
using namespace gos::asset;


//******************************************************
bool Loader_shader::load (Loader *assetLoader, const asset::Context &ctx, const asset::UID &uid, void *in_out_asset)
{
	Asset_shader *out = static_cast <Asset_shader*>(in_out_asset);
	assert (uid.isValid());
	assert (uid.isAnAssetOfType(this->getAssType()));


	char s[1024];
	asset::asset_manufacture_fullFilename (ctx, uid, s, sizeof(s));


	gos::GPU *gpu = assetLoader->getGPU();
	switch (this->getAssType())
	{
	default:
		gos::logger::err ("asset::Loader_shader => invalid asset type\n"); return false;
	case eAssetType::vtx_shader:	return gpu->vtxshader_createFromFile (s, "main", &out->handle_shader);
	case eAssetType::pxl_shader:	return gpu->fragshader_createFromFile (s, "main", &out->handle_shader);
	}
}