#ifndef _gosEngineLoaders_shader_h_
#define _gosEngineLoaders_shader_h_
#include "gosEngineLoaders.h"

namespace gos
{
    namespace engine
    {
        namespace loaders
        {
            //********************************************************
            class Loader_vtxShader : public loaders::BaseLoader
            {
            public:
                bool    load (LoaderInfo &loaderInfo, asset2::UID uid, void *res_dataIN)
                {
                    ResShader *res_data = static_cast<ResShader*>(res_dataIN);
                    gos::GPU *gpu = loaderInfo.gpu;

                    char s[1024];
                    asset2::asset_manufacture_fullFilename (*loaderInfo.ctx, uid, s, sizeof(s));
                    if (!gpu->vtxshader_createFromFile (s, "main", &res_data->data.shaderHandle))
                        return false;

                    return true;
                }
            };


            //********************************************************
            class Loader_pxlShader : public loaders::BaseLoader
            {
            public:
                bool    load (LoaderInfo &loaderInfo, asset2::UID uid, void *res_dataIN)
                {
                    ResShader *res_data = static_cast<ResShader*>(res_dataIN);
                    gos::GPU *gpu = loaderInfo.gpu;

                    char s[1024];
                    asset2::asset_manufacture_fullFilename (*loaderInfo.ctx, uid, s, sizeof(s));
                    if (!gpu->pxlshader_createFromFile (s, "main", &res_data->data.shaderHandle))
                        return false;

                    return true;

                }
            };


        } //namespace loaders
    } //namespace engine
} //namespace gos


#endif //_gosEngineLoaders_shader_h_
