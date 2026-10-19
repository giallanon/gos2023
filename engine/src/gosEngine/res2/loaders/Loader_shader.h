#ifndef _gosEngine_res2_loaders_Loaders_shader_h_
#define _gosEngine_res2_loaders_Loaders_shader_h_
#include "Loader.h"

namespace gos
{
	namespace res2
	{
		//********************************************************
		class Loader_VtxShader : public res2::BaseLoader
		{
		public:
			eResult load (LoaderInfo &loaderInfo, res2::ResDescr *descrIN, CallbackData_onLoad *in_out__callback_data)
			{
				gos::Allocator *thread_allocator = loaderInfo.thread_allocator;
				GPU *gpu = loaderInfo.gpu;

				char s[1024];
				asset2::signature_manufacture_fullFilename (*loaderInfo.ctx, descrIN->UID, s, sizeof(s));

				u32 fsize;
				u8 *buffer = gos::fs::fileLoadInMemory (thread_allocator, s, &fsize);
				if (NULL == buffer)
				{
					logger::err ("Loader_VtxShader::load() => file not found %s\n", s);
					return eResult::failed;
				}

				GPUShaderHandle shader_handle;
				if (!gpu->vtxshader_createFromMemory (buffer, fsize, "main", &shader_handle))
				{
					logger::err ("Loader_VtxShader::load(%s) => error creating shader\n", s);
					GOSFREE(thread_allocator, buffer);
					return eResult::failed;
				}

				//torno all'engine per creare la risorsa
				GOSFREE(thread_allocator, buffer);
				in_out__callback_data->user_1 = shader_handle.viewAsU32();
				return eResult::success;
			}
		};
		
		//********************************************************
		class Loader_PxlShader : public res2::BaseLoader
		{
		public:
			eResult load (LoaderInfo &loaderInfo, res2::ResDescr *descrIN, CallbackData_onLoad *in_out__callback_data)
			{
				gos::Allocator *thread_allocator = loaderInfo.thread_allocator;
				GPU *gpu = loaderInfo.gpu;

				char s[1024];
				asset2::signature_manufacture_fullFilename (*loaderInfo.ctx, descrIN->UID, s, sizeof(s));

				u32 fsize;
				u8 *buffer = gos::fs::fileLoadInMemory (thread_allocator, s, &fsize);
				if (NULL == buffer)
				{
					logger::err ("Loader_PxlShader::load() => file not found %s\n", s);
					return eResult::failed;
				}

				GPUShaderHandle shader_handle;
				if (!gpu->pxlshader_createFromMemory (buffer, fsize, "main", &shader_handle))
				{
					logger::err ("Loader_PxlShader::load(%s) => error creating shader\n", s);
					GOSFREE(thread_allocator, buffer);
					return eResult::failed;
				}

				//torno all'engine per creare la risorsa
				GOSFREE(thread_allocator, buffer);
				in_out__callback_data->user_1 = shader_handle.viewAsU32();
				return eResult::success;
			}
		};			
    } //namespace res2
} //namespace gos



#endif //_gosEngine_res2_loaders_Loaders_shader_h_