#ifndef _gosEngineRenderPipe_h_
#define _gosEngineRenderPipe_h_
#include "../gosEngineEnumAndDefine.h"
#include "../res/gosEngineRes.h"
#include "../gosEngine_dynTextureArray.h"
#include "../../gosGeom/gosGeomCamera3.h"
#include "../entity/gosEntityDefaultComponents.h"


namespace gos
{
	class Engine; //fwd

	namespace engine
	{
		/******************************
		 * @brief	RenderPipe
		 * 
		 */
		class RenderPipe
		{
		public:
			static u8 constexpr		SPECIAL_TEXTURE__BIANCA		= 0;

		public:
			struct SceneData
			{
				gos::mat4x4f    matVP;
				gos::vec3f      lightDir;
				f32				ambient_01;
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

	        struct Material
	        {
		        vec3f	diffuse_col;
		        u32		texture_index;
	        };	

			/******************************
			 * @brief	Renderer
			 * 
			 */
			class Renderer
			{
			public:
								Renderer()			{ }
				virtual			~Renderer()			{ }

				virtual bool 	on__attach (const RenderPipe::Context &ctx, u8 renderer_UID) = 0;
				virtual void 	on__detach (const RenderPipe::Context &ctx) = 0;
				virtual void 	on__render (const RenderPipe::Context &ctx, gpu::RenderCtx &rctx) = 0;
			};

		public:
			//==================== render addizionali
						template<class RENDERER>
			RENDERER* 	add_renderer ()
						{
							RENDERER *r = GOSNEW(ctx.allocator, RENDERER)();
							priv_add_renderer(r);
							return r;
						}
			void 	remove_renderer (Renderer *r);
			void	render (gos::gpu::SwapchainImg swapchainImg, GPUCmdBufferHandle cmdBufferHandl, gos::geom::Camera3 *cam);


			//==================== gestione texture
			bool	internal__texture_add_reserved (GPUTextureHandle texHandle, u32 texture_index);
            u32		internal__texture_add_if_dont_exists (GPUTextureHandle texHandle);
            void	internal__texture_remove (GPUTextureHandle texHandle)                                     { texture_array.remove(texHandle); }
            bool	internal__texture_find (GPUTextureHandle texHandle, u32 *out_index) const                 { return texture_array.find(texHandle, out_index); }

			

        private:
            static constexpr u32    NUM_MAX_TEXTURE     = 1024;

		private:
					//solo engine puo' istanziare RenderPipe
					RenderPipe();
					~RenderPipe()																			{ priv_unsetup(); }
			bool	priv_setup (gos::Allocator *allocator, Engine *eng);
			void	priv_unsetup();
			void 	priv_add_renderer (Renderer *r);

        private:
			Context						ctx;
            Engine                      *engine;
			DynamicTextureArray         texture_array;			
			GPUSamplerHandle            handle_samplers[2];
			gos::FastArray<Renderer*>	renderer_list;
			GPUDescrSetLayoutHandle		handle_descr_set_0;
			GPUDescrSetLayoutHandle		handle_descr_set_1;
			u8							next_renderer_UID;


		friend Engine;
		};
	} //namespace engine
} //namespace gos

#endif //_gosEngineRenderPipe_h_