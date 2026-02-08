#ifndef _gosEngineLoaders_pipe_h_
#define _gosEngineLoaders_pipe_h_
#include "gosEngineLoaders_shader.h"

namespace gos
{
    namespace engine
    {
        namespace loaders
        {
            //********************************************************
            class Loader_pipeline : public loaders::BaseLoader
            {
            public:
                bool    load (LoaderInfo &loaderInfo, void *resIN)
                {
                    res::Pipeline *res = static_cast<res::Pipeline*>(resIN);
                    gos::Allocator *thread_allocator = loaderInfo.thread_allocator;
					gos::GPU *gpu = loaderInfo.gpu;
					Engine *engine = loaderInfo.engine;

                    char s[1024];
                    asset2::asset_manufacture_fullFilename (*loaderInfo.ctx, res->_descr.uid, s, sizeof(s));

                    u32 fsize;
                    u8 *buffer = fs::fileLoadInMemory (thread_allocator, s, &fsize);
                    if (NULL == buffer)
                    {
                        logger::err ("Loader_pipeline::load() => file not found %s\n", s);
                        return false;
                    }

                    gos::BufferR reader;
                    reader.setup (buffer, fsize);
                    
                    bool ret = false;
                    while (1)
                    {
                        const u32 magic = reader.readU32();
                        if (!magic::signatureMatch(magic, GOS_MAGIC__ASSET_PIPELINE_DEF) || !magic::versionMatch(magic, GOS_MAGIC__ASSET_PIPELINE_DEF))
                        {
                            logger::err ("Loader_pipeline::load() => invalid magic for file %s\n", s);
                            break;
                        }

                        gpu::Pipeline_def def;
                        def.reset();


                        //uid vtx shader
                        //L'asset dovrebbe gia' essere stato caricato perche' engine ha schedulato i vari load in maniera intelligente.
                        asset2::UID uid;
                        uid._uid = reader.readU64 ();
                        {
							const res::Shader *shader;
							if (!engine->internal__getResFromUID(uid, &shader))
                            {
                                logger::log (eTextColor::red, "asset::  Loader_pipeline::load() => unable to match vtx_shader %016" PRIX64 " with raw data\n");
                                break;
                            }                            
                            def.shader_add (shader->shaderHandle);
                        }
                        

                        //uid pxl shader
                        uid._uid = reader.readU64 ();
                        {
							const res::Shader *shader;
							if (!engine->internal__getResFromUID(uid, &shader))
                            {
                                logger::log (eTextColor::red, "asset::  Loader_pipeline::load() => unable to match pxl_shader %016" PRIX64 " with raw data\n");
                                break;
                            }                            
                            def.shader_add (shader->shaderHandle);
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
                            options.setFromU32 (reader.readU32 ());


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
                                if (options.isset(eGPUDescriptrorSetOption::bindless))
                                    builder.addCreationFlag (VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT);

                                builder.add (binding, type, count, usage);
                            }
                        }        

                        //creo la pipe
                        if (!gpu->pipeline_createNew (def, &res->pipeHandle))
                            break;



                        //finito
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


#endif //_gosEngineLoaders_pipe_h_
