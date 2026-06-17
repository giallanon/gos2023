#ifndef _gosEngine_renderer_h_
#define _gosEngine_renderer_h_
#include "gosEngineEnumAndDefine.h"
#include "res/gosEngineRes.h"
#include "gosEngine_dynTextureArray.h"
#include "../gosGeom/gosGeomCamera3.h"
#include "entity/gosEntityDefaultComponents.h"


namespace gos
{
    class Engine; //fwd

    namespace engine
    {
        /**********************************************
        * RendererCommon
        * 
        */
        class RendererCommon
        {
        public:
                    RendererCommon()                                                                        { engine = NULL; }
                    ~RendererCommon()                                                                       { unsetup(); }

            bool    setup (gos::Allocator *allocator, Engine *engine, const char *pipeline_asset_name);
            void    unsetup();

            //==== gestione texture ====
            u32             texture_addIfNotExitst (GPUTextureHandle texHandle);
            void            texture_remove (GPUTextureHandle texHandle)                                     { texture_array.remove(texHandle); }
            bool            texture_find (GPUTextureHandle texHandle, u32 *out_index) const                 { return texture_array.find(texHandle, out_index); }


        public:
            ENGPipeline                 handle_pipeline;
            GPUDescrPoolHandle          handle_descrPool;
            GPUZBufferHandle            handle_zbuffer;
            GPURenderTargetHandle       handle_rt0;
            GPUDescrSetInstanceHandle   handle_descrSet0;
            GPUSamplerHandle            handle_samplers[2];
            
        private:
            static constexpr u32    NUM_MAX_TEXTURE     = 1024;

        private:
            Engine                      *engine;
            DynamicTextureArray         texture_array;
        };


    } //namespace engine
} //namespace gos


#endif //_gosEngine_renderer_h_
