#ifndef _gosEngineLoaders_materialPBR_h_
#define _gosEngineLoaders_materialPBR_h_
#include "gosEngineLoaders.h"
#include "../gosEngine.h"
#include "../gosAsset2/assetFile/gosAssetFile_materialPBR.h"

namespace gos
{
    namespace engine
    {
        namespace loaders
        {
            //********************************************************
            class Loader_materialPBR : public loaders::BaseLoader
            {
            public:
                bool    load (LoaderInfo &loaderInfo, void *resIN)
                {
                    res::MaterialPBR *res = static_cast<res::MaterialPBR*>(resIN);
                    gos::Allocator *thread_allocator = loaderInfo.thread_allocator;
					//Engine *eng = loaderInfo.engine;

                    char s[1024];
                    asset2::asset_manufacture_fullFilename (*loaderInfo.ctx, res->_descr.uid, s, sizeof(s));

                    u32 fsize;
                    u8 *buffer = fs::fileLoadInMemory (thread_allocator, s, &fsize);
                    if (NULL == buffer)
                    {
                        logger::err ("Loader_materialPBR::load() => file not found %s\n", s);
                        return false;
                    }

					bool ret = false;
					while (1)
					{
						asset2::MaterialPBR matIN;
						if (0 == asset2::AssetFile_materialPBR::deserialize (buffer, fsize, &matIN))
						{
							logger::err ("Loader_materialPBR::load() => invalid file for file %s\n", s);
							break;
						}

						res->set_default_material_params();
						res->diffuse_col_HDR_RGBA.set (matIN.diffuse_col_RGBA_HDR[0], matIN.diffuse_col_RGBA_HDR[1], matIN.diffuse_col_RGBA_HDR[2], matIN.diffuse_col_RGBA_HDR[3]);
						res->diffuse_texture_index = engine::RenderPipe::SPECIAL_TEXTURE__BIANCA;


						//fine, tutto ok
						ret = true;
						break;
					}

					GOSFREE(thread_allocator, buffer);
                    return ret;
                }
            };


        } //namespace loaders
    } //namespace engine
} //namespace gos


#endif //_gosEngineLoaders_materialPBR_h_
