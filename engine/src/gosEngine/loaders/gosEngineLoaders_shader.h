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
                eResult load (LoaderInfo &loaderInfo,void *resIN, CallbackData *in_out__callback_data)
                {
                    res::Shader *res = static_cast<res::Shader*>(resIN);
					gos::Allocator *thread_allocator = loaderInfo.thread_allocator;
                    gos::GPU *gpu = loaderInfo.gpu;

                    char s[1024];
                    asset2::asset_manufacture_fullFilename (*loaderInfo.ctx, res->_descr.uid, s, sizeof(s));

                    u32 fsize;
                    u8 *buffer = fs::fileLoadInMemory (thread_allocator, s, &fsize);
                    if (NULL == buffer)
                    {
                        logger::err ("Loader_vtxShader::load() => file not found %s\n", s);
                        return eResult::failed;
                    }

                    const bool ret = gpu->vtxshader_createFromMemory (buffer, fsize, "main", &res->shaderHandle);
                    GOSFREE(thread_allocator, buffer);

					if (ret)
						return eResult::success;
                    return eResult::failed;
                }
            };


            //********************************************************
            class Loader_pxlShader : public loaders::BaseLoader
            {
            public:
                eResult load (LoaderInfo &loaderInfo, void *resIN, CallbackData *in_out__callback_data)
                {
                    res::Shader *res = static_cast<res::Shader*>(resIN);
					gos::Allocator *thread_allocator = loaderInfo.thread_allocator;
                    gos::GPU *gpu = loaderInfo.gpu;

                    char s[1024];
                    asset2::asset_manufacture_fullFilename (*loaderInfo.ctx, res->_descr.uid, s, sizeof(s));
                    
					u32 fsize;
                    u8 *buffer = fs::fileLoadInMemory (thread_allocator, s, &fsize);
                    if (NULL == buffer)
                    {
                        logger::err ("Loader_pxlShader::load() => file not found %s\n", s);
                        return eResult::failed;
                    }

                    const bool ret = gpu->pxlshader_createFromMemory (buffer, fsize, "main", &res->shaderHandle);
                    GOSFREE(thread_allocator, buffer);

                    if (ret)
						return eResult::success;
					return eResult::failed;

                }
            };


        } //namespace loaders
    } //namespace engine
} //namespace gos


#endif //_gosEngineLoaders_shader_h_
