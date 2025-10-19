#ifndef _gosEngine_renderer_h_
#define _gosEngine_renderer_h_
#include "gosEngineEnumAndDefine.h"
#include "../gosGeom/gosGeomCamera3.h"


namespace gos
{
    class Engine; //fwd

    namespace engine
    {
        class Renderer
        {
        public:
                    Renderer();
                    ~Renderer()                                                                                     { priv_free(); }

            bool    setup (gos::Engine *engine);

            void    begin (gos::geom::Camera3 *cam);
            void    add (const ENGShape shape, const ENGMatrixW worldPos);
            void    end();

        private:
            static constexpr u32    NUM_MAX_MATERIAL                        = 1024;
            static constexpr u32    SIZEOF_ONE_ELEMENT_IN_MATERIAL_SSBO     = 64;
                        
        private:
            struct SceneData
            {
                gos::mat4x4f    camVP;
                gos::vec4f      lightDir;
                gos::vec2f      screenWH;
            };

            struct Material
            {
                gos::vec4f  color;
                u32         textureIndex;
            };
            
        private:
            void    priv_free();

        private:
            gos::Engine             *engine;
            gos::GPU                *gpu;
            asset::Handle           assHandle_pipe;
            asset::Handle           assHandle_tex_checker;

            GPUZBufferHandle            handle_zbuffer;
            GPURenderTargetHandle       handle_rt0;
            GPUDescrPoolHandle          handle_descrPool;
            GPUDescrSetInstanceHandle   handle_descrSet0;
            GPUDescrSetInstanceHandle   handle_descrSet1;
            GPUDescrSetInstanceHandle   handle_descrSet2;
            GPUSamplerHandle            handle_samplers[2];
            GPUUniformBufferHandle      handle_ubo_scene;
            GPUStorageBufferHandle      handle_sbo_materialList;
        };
    } //namespace engine
} //namespace gos


#endif //_gosEngine_renderer_h_
