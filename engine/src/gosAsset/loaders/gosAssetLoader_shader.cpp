#include "gosAssetLoader_shader.h"
#include "../gosAssetLoader.h"
#include "../gosAsset.h"


using namespace gos;
using namespace gos::asset;


//******************************************************
bool Loader_shader::load (Loader *assetLoader, const asset::Context &ctx, void *in_out_asset)
{
	Asset_shader *out = static_cast <Asset_shader*>(in_out_asset);
	assert (out->uid.isValid());
	assert (out->uid.isAnAssetOfType(this->getAssType()));


	char s[1024];
	asset::asset_manufacture_fullFilename (ctx, out->uid, s, sizeof(s));


	gos::GPU *gpu = assetLoader->getGPU();
	return gpu->vtxshader_createFromFile (s, "main", &out->handle_shader);
}