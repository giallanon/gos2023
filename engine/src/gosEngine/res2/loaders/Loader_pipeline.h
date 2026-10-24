#ifndef _gosEngine_res2_loaders_Loaders_pipeline_h_
#define _gosEngine_res2_loaders_Loaders_pipeline_h_
#include "Loader.h"

namespace gos
{
	namespace res2
	{
		//********************************************************
		class Loader_pipeline : public res2::BaseLoader
		{
		public:
			eResult load (LoaderInfo &loaderInfo, res2::ResDescr *descrIN, CallbackData_onLoad *in_out__callback_data)
			{
				gos::Allocator *thread_allocator = loaderInfo.thread_allocator;

				char s[1024];
				asset2::signature_manufacture_fullFilename (*loaderInfo.ctx, descrIN->UID, s, sizeof(s));

				u32 fsize;
				u8 *buffer = gos::fs::fileLoadInMemory (thread_allocator, s, &fsize);
				if (NULL == buffer)
				{
					logger::err ("Loader_pipeline::load() => file not found %s\n", s);
					return eResult::failed;
				}

				gos::BufferR reader;
				reader.setup (buffer, fsize);

				const u32 magic = reader.readU32();
				if (!magic::signatureMatch(magic, GOS_MAGIC__ASSET_PIPELINE_DEF) || !magic::versionMatch(magic, GOS_MAGIC__ASSET_PIPELINE_DEF))
				{
					logger::err ("Loader_pipeline::load() => invalid magic for file %s\n", s);
					GOSFREE(thread_allocator, buffer);
					return eResult::failed;
				}

				//torno all'engine per creare la risorsa
				in_out__callback_data->user_data_pt = buffer;
				in_out__callback_data->user_1 = fsize;
				in_out__callback_data->user_2 = 0;
				return eResult::callback;
			}

			eResult load_continued (LoaderInfo &loaderInfo, CallbackData_onLoad *callback_data)
			{
				const u32 current_step = callback_data->user_2;

				if (0 == current_step)
				{
					callback_data->user_2 = 1;
					return eResult::callback;
				}

				//a questo punto, vtx/pxl shader sono stati caricati, posso procedere alla creazione della pipe
				assert (1 == current_step);
				callback_data->user_2 = 2;


				gos::Allocator *thread_allocator = loaderInfo.thread_allocator;
				GPU *gpu = loaderInfo.gpu;
				res2::Pipeline::Resource *res_pipeline = reinterpret_cast<res2::Pipeline::Resource*>( callback_data->descr );

				gos::BufferR reader;
				void *file_buffer = callback_data->user_data_pt;
				const u32 fsize = callback_data->user_1;
				reader.setup (file_buffer, fsize);

				//skip del <magic> che e' gia' stato verificato dal loader
				reader.readU32();

				//skippo UID vtx shader
				reader.readU64 ();

				//skippo UID pxl shader
				reader.readU64 ();


				gpu::Pipeline_def def;
				def.reset();

				{
					res2::ResHandleChain *p = res_pipeline->base.list_figli;
					res2::PxlShader::Asset *pxlshader_asset = reinterpret_cast<res2::PxlShader::Asset*>( p->descr );
					p = p->next;
					res2::VtxShader::Asset *vtxshader_asset = reinterpret_cast<res2::VtxShader::Asset*>( p->descr );

					assert (res2::is_ready(&pxlshader_asset->base));
					assert (res2::is_ready(&vtxshader_asset->base));

					def.shader_add (vtxshader_asset->res_data_pt->vtxshader_handle);
					def.shader_add (pxlshader_asset->res_data_pt->pxlshader_handle);
				}

				//cull/draw
				def.set_cullMode (static_cast<eCullMode>(reader.readU8()));
				def.set_drawPrimitive (static_cast<eDrawPrimitive>(reader.readU8()));
				if (0 != reader.readU8())
					def.enable_wireframe();

				//zbuffer
				{
					Flag8 zbuffer_flag;
					zbuffer_flag.setBitmask (reader.readU8());
					const eImageFormat fmt = static_cast<eImageFormat>(reader.readU8());
					const eZFunc zfunc = static_cast<eZFunc>(reader.readU8());

					if (zbuffer_flag.isBitSet(gpu::Pipeline_def::ZBUFFER_FLAG__ENABLED))
					{
						bool zwrite = false;
						if (zbuffer_flag.isBitSet(gpu::Pipeline_def::ZBUFFER_FLAG__ZWRITE_ENABLED))
							zwrite = true;
						def.zbuffer_define (fmt, zwrite, zfunc);

						if (zbuffer_flag.isBitSet(gpu::Pipeline_def::ZBUFFER_FLAG__ALLOW_DEPTH_TEST_ENABLE_DISABLE))
							def.zbuffer_allow_depthTestEnablingDisabling();

						if (zbuffer_flag.isBitSet(gpu::Pipeline_def::ZBUFFER_FLAG__ALLOW_DEPTH_WRITE_ENABLE_DISABLE))
							def.zbuffer_allow_depthWriteEnablingDisabling();
					}
				}


				//render target
				u32 n = reader.readU32 ();
				for (u32 i=0; i<n; i++)
				{
					const eImageFormat fmt = static_cast<eImageFormat>(reader.readU8());
					def.rt_add (fmt);
				}

				//vtx declaration
				n = reader.readU32 ();
				if (n)
				{
					auto &builder = def.vtxStream_add(eVtxStreamInputRate::perVertex);
					for (u32 i = 0; i < n; i++)
					{
						const u8 binding = reader.readU8 ();
						const u32 offset = reader.readU32 ();
						const eDataFormat fmt = static_cast<eDataFormat> (reader.readU8 ());
						builder.add (binding, offset, fmt);
					}        
				}

				//push constant
				n = reader.readU32 ();
				while (n--)
				{
					const u32 offset = reader.readU32();
					const u32 paddedSize = reader.readU32();
					const eShaderType shaderType = static_cast<eShaderType> (reader.readU32());
					def.pushConst_add (offset, paddedSize, shaderType);
				}


				//descriptor set
				const u32 numSet = reader.readU32 ();
				for (u32 i = 0; i < numSet; i++)
				{
					auto &builder = def.descriptorset_add();

					eGPUDescriptrorSetOptionBitmask options;
					options.set_from_U32 (reader.readU32 ());


					const u32 numElem = reader.readU32 ();
					for (u32 i2 = 0; i2 < numElem; i2++)
					{
						const u8 binding = reader.readU8();
						const eGPUDescriptrorType type = static_cast<eGPUDescriptrorType>(reader.readU8());
						u32 count = reader.readU32();
						eGPUDescriptrorUsageBitmask usage;
						usage.bitmask = reader.readU32();

						//TODO
						//u32MAX == count => il buffer e' di tipo bindless... in attesa di capirci megli qualcosa
						//                      semplicemente lo alloco "grosso"
						if (u32MAX == count)
						{
							count = 1;
							builder.addCreationFlag (VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT);
						}
						if (options.is_set(eGPUDescriptrorSetOption::bindless))
							builder.addCreationFlag (VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT);

						builder.add (binding, type, count, usage);
					}
				}   
				
				GOSFREE(thread_allocator, file_buffer);

				//creo la pipe
				GPUPipelineHandle pipe_handle;
				if (!gpu->pipeline_createNew (def, &pipe_handle))
				{
					logger::err ("Loader_pipeline::load_continued() => error creating pipe def\n");
					return eResult::failed;
				}
					
				//torno all'engine
				callback_data->user_1 = pipe_handle.viewAsU32();
				return eResult::success;
			}
		};
    } //namespace res2
} //namespace gos



#endif //_gosEngine_res2_loaders_Loaders_pipeline_h_
