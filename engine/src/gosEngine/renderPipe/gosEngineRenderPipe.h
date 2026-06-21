#ifndef _gosEngineRenderPipe_h_
#define _gosEngineRenderPipe_h_
#include "../gosEngineEnumAndDefine.h"
#include "../res/gosEngineRes.h"
#include "../gosEngine_dynTextureArray.h"
#include "../../gosGeom/gosGeomCamera3.h"

namespace gos
{
	class Engine; //fwd

	namespace engine
	{
		/******************************
		 * RenderPipe
		 * 
		 */
		class RenderPipe
		{
		public:
			struct SceneData
			{
				gos::mat4x4f    matVP;
				gos::vec4f      lightDir;
			};

			struct Context 
			{
				gos::Allocator				*allocator;
				Engine 						*engine;
				GPURenderTargetHandle       handle_rt0;
				GPUZBufferHandle            handle_zbuffer;
				GPUDescrSetInstanceHandle   handle_descrSet0;
				GPUDescrSetInstanceHandle   handle_descrSet1;
				GPUUniformBufferHandle		handle_ubo_scene;
				GPUDescrPoolHandle          handle_descrPool;
				SceneData					scene;;
				gos::geom::Camera3 			*cam;
				u32							frame_number;
			};

			/******************************
			 * Renderer
			 * 
			 */
			class Renderer
			{
			public:
								Renderer()			{ }
				virtual			~Renderer()			{ }

				virtual bool 	on__attach (const RenderPipe::Context &ctx) = 0;
				virtual void 	on__detach (const RenderPipe::Context &ctx) = 0;
				virtual void 	on__render (const RenderPipe::Context &ctx, gpu::RenderCtx &rctx) = 0;
			};

		public:
					RenderPipe();
					~RenderPipe()																			{ unsetup(); }

			bool	setup (gos::Allocator *allocator, Engine *eng);
			void	unsetup();

            //==== gestione texture ====
            u32		texture_addIfNotExitst (GPUTextureHandle texHandle);
            void	texture_remove (GPUTextureHandle texHandle)                                     { texture_array.remove(texHandle); }
            bool	texture_find (GPUTextureHandle texHandle, u32 *out_index) const                 { return texture_array.find(texHandle, out_index); }

			//==== render ====
						template<class RENDERER>
			RENDERER* 	add_renderer ()
			{
				RENDERER *r = GOSNEW(ctx.allocator, RENDERER)();
				priv_add_renderer(r);
				return r;
			}

			void 	remove_renderer (Renderer *r);
			void	render (gos::gpu::SwapchainImg swapchainImg, GPUCmdBufferHandle cmdBufferHandl, gos::geom::Camera3 *cam);

        private:
            static constexpr u32    NUM_MAX_TEXTURE     = 1024;

		private:
			void 	priv_add_renderer (Renderer *r);

        private:
			Context						ctx;
            Engine                      *engine;
			DynamicTextureArray         texture_array;			
			ENGPipeline 				handle_pipeline;
			GPUSamplerHandle            handle_samplers[2];
			gos::FastArray<Renderer*>	renderer_list;
		};
	} //namespace engine
} //namespace gos

#endif //_gosEngineRenderPipe_h_