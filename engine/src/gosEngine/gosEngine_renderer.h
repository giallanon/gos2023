#ifndef _gosEngine_renderer_h_
#define _gosEngine_renderer_h_
#include "gosEngineEnumAndDefine.h"
#include "../gosGeom/gosGeomCamera3.h"
#include "../gosGPU/gosGPUMemMappedDynBuffer.h"


namespace gos
{
    class Engine; //fwd

    namespace engine
    {
        class Renderer1
        {
        public:
                    Renderer1();
                    ~Renderer1()                                                                                     { unsetup(); }

            bool    setup (gos::Allocator *allocator, Engine *engine);
            void    unsetup();

            void    begin (gos::geom::Camera3 *cam);
            void    add (const ENGShape shape, u32 matrixIndex, u32 materialIndex);
            void    end (gos::gpu::pipe2::CmdBufferWriter2 &cw);


            GPURenderTargetHandle   getHandle_rt0() const                                           { return handle_rt0; }


        private:
            static constexpr u32    NUM_MAX_TEXTURE                         = 1024;
            static constexpr u32    NUM_MAX_MATERIAL                        = 1024;
            static constexpr u32    NUM_MAX_MATRIX                          = 1024;
                        
        private:
            struct SceneData
            {
                gos::mat4x4f    matVP;
                gos::vec4f      lightDir;
            };

	        struct sMaterial
	        {
		        vec3f	diffuse_col;
		        u32		texture_index;
	        };	


        private:
            gos::Engine                 *engine;
            gos::GPU                    *gpu;
            asset::Handle               assHandle_pipe;

            GPUZBufferHandle            handle_zbuffer;
            GPURenderTargetHandle       handle_rt0;
            GPUDescrPoolHandle          handle_descrPool;
            GPUDescrSetInstanceHandle   handle_descrSet0;
            GPUDescrSetInstanceHandle   handle_descrSet1;
            GPUDescrSetInstanceHandle   handle_descrSet2;
            GPUSamplerHandle            handle_samplers[2];
            GPUUniformBufferHandle      handle_ubo_scene;
            GPUStorageBufferHandle      handle_sbo_matrixList;
            GPUStorageBufferHandle      handle_sbo_materiaList;

            gpu::MemMappedDynBuffer<mat4x4f>    matrixBuffer;
            gpu::MemMappedDynBuffer<sMaterial>  materialBuffer;

            SceneData   scene;
        };
    } //namespace engine
} //namespace gos


#endif //_gosEngine_renderer_h_
