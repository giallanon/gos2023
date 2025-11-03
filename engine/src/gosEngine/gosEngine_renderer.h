#ifndef _gosEngine_renderer_h_
#define _gosEngine_renderer_h_
#include "gosEngineEnumAndDefine.h"
#include "gosEngine_dynTextureArray.h"
#include "../gosGeom/gosGeomCamera3.h"


namespace gos
{
    class Engine; //fwd

    namespace engine
    {
        class Renderer1
        {
        public:
	        struct Material
	        {
		        vec3f	diffuse_col;
		        u32		texture_index;
	        };	

        public:
                    Renderer1();
                    ~Renderer1()                                                                            { unsetup(); }

            bool    setup (gos::Allocator *allocator, Engine *engine);
            void    unsetup();

            void    begin (gos::geom::Camera3 *cam);
            void    add (const ENGShape shape, u32 matrixIndex, u32 materialIndex);
            void    end (gos::gpu::pipe2::CmdBufferWriter2 &cw);

            GPURenderTargetHandle   getHandle_rt0() const                                                   { return handle_rt0; }

            //==================== gestione delle risorse
            u32             texture_addIfNotExitst (GPUTextureHandle texHandle);
            void            texture_remove (GPUTextureHandle texHandle)                                     { texture_array.remove(texHandle); }
            bool            texture_find (GPUTextureHandle texHandle, u32 *out_index) const                 { return texture_array.find(texHandle, out_index); }

            u32             material_create (u32 texture_index, const vec3f diffuse_col);
            void            material_delete (u32 material_index);
            Material*       material_getForUpdate (u32 material_index);
            const Material* material_query (u32 material_index) const;

            u32             matrix_create ();
            u32             matrix_create (const mat4x4f &m);
            void            matrix_delete (u32 matrix_index);
            mat4x4f*        matrix_getForUpdate (u32 matrix_index);
            void            matrix_update (u32 matrix_index, const mat4x4f &m);
            const mat4x4f*  matrix_query (u32 matrix_index) const;

        private:
            static constexpr u32    NUM_MAX_TEXTURE     = 1024;
            static constexpr u32    NUM_MAX_MATERIAL    = 1024;
            static constexpr u32    NUM_MAX_MATRIX      = 1024;
                        
        private:
            struct SceneData
            {
                gos::mat4x4f    matVP;
                gos::vec4f      lightDir;
            };

            struct Renderable
            {
                    ENGShape shape;
                    u32 matrixIndex;
                    u32 materialIndex;
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

            DynamicTextureArray                 texture_array;

            mat4x4f                             matrix_default;
            mat4x4f                             *matrix_buffer;
            u32                                 matrix_sizeof_buffer;
            gos::Bitfield                       matrix_bitmask;
            u32                                 matrix_wasUpdated;
            
            Material                            material_default;
            Material                            *material_buffer;
            u32                                 material_sizeof_buffer;
            gos::Bitfield                       material_bitmask;
            u32                                 material_wasUpdated;

            SceneData                   scene;
            FastArray<Renderable>       renderableList;
        };
    } //namespace engine
} //namespace gos


#endif //_gosEngine_renderer_h_
