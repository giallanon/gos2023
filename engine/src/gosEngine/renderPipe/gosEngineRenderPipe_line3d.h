#ifndef gosEngineRenderPipe_line3d
#define gosEngineRenderPipe_line3d
#include "../res/gosEngineRes.h"
#include "dataTypes/gosColorU32.h"
#include "../renderPipe/gosEngineRenderPipe.h"

namespace gos
{
	class Engine; //fwd

	namespace engine
	{
		/**************
		* @brief	Renderer_line3d
		* 			Renderer per il rendering di line in 3d
		* 
		*/
		class Renderer_line3d : public gos::engine::RenderPipe::Renderer
		{
		public:
			/*****************
			 * @brief	Ctx
			 * 			un contex contiene un elenco di punti, linee, colori e settaggi vari
			 * 			Per renderizzare un ctx, utilizzare Renderer_line3d->appendToCommandBuffer()
			 */
			class Ctx
			{
			public:
						~Ctx()																	{ }

				void	setup (gos::Allocator *allocator, u16 estimated_num_vtx);
				void	unsetup ();

				Ctx&	clear();

				Ctx&	set_color_ARGB (const gos::ColorU32 &col)								{ set_color_ARGB (col.argb); return *this; }
				Ctx&	set_color_ARGB (u32 argb);
				Ctx&	set_line_width (u16 w);
				
				Ctx& 	enable_depth_test(bool b);
				Ctx& 	enable_depth_write(bool b);

				u16		vtx_add (const vec3f &p);
				u16		vtx_add (f32 x, f32 y, f32 z)											{ return vtx_add (vec3f(x, y, z)); }
				u32		vtx_get_num() const														{ return vtxList.getNElem(); }

				Ctx&	line_begin();
				Ctx&	line_add_vtx (f32 x, f32 y, f32 z)										{ line_add_vtx (vec3f(x, y, z)); return *this; }
				Ctx&	line_add_vtx (const vec3f &p)											{ const u16 vtx_index = vtx_add(p); line_add_vtx(vtx_index); return *this; }
				Ctx&	line_add_vtx (u16 vtx_index);
				Ctx&	line_end();

				Ctx&	line (f32 p1x, f32 p1y, f32 p1z, f32 p2x, f32 p2y, f32 p2z)				{ line_begin(); line_add_vtx(p1x, p1y, p1z); line_add_vtx(p2x, p2y, p2z); line_end(); return *this; }
				Ctx&	line (const vec3f &p1, const vec3f &p2)									{ line (p1.x, p1.y, p1.z, p2.x, p2.y, p2.z); return *this; }
				Ctx&	line (u16 vtx_index1, u16 vtx_index2)									{ line_begin(); line_add_vtx(vtx_index1); line_add_vtx(vtx_index2); line_end(); return *this; }
				Ctx&	closed_line (const FastArray<vec3f> &vtxList, u32 num_vtx);

				Ctx&	point_set_radius (u16 radius);
				Ctx& 	point (u16 vtx_index);

				Ctx& 	aabb3 (const vec3f &vmin, const vec3f &vmax, u16 line_width=0);

			private:
				enum class eCMD : u16
				{
					line_def			= 0x0001,
					set_color_ARGB		= 0x0002,
					enable_depth_test	= 0x0003,
					disable_depth_test	= 0x0004,
					enable_depth_write	= 0x0005,
					disable_depth_write	= 0x0006,
					set_line_width		= 0x0007,

					point_def			= 0x0008,
					set_point_radius	= 0x0009,
				};

			private:
						Ctx();

			private:
				FastArray<vec3f>	vtxList;
				FastArray<u16>		program;
				u32					line_started_at;			

			friend Renderer_line3d;
			};

		public:
			using RPIPE = gos::engine::RenderPipe;

		public:
					Renderer_line3d();
					~Renderer_line3d();


			bool 	on__attach (const RPIPE::Context &ctx, u8 renderer_UID) final;
			void 	on__detach (const RPIPE::Context &ctx) final									{ priv_unsetup(); }
			void 	on__render (const RPIPE::Context &ctx, gos::gpu::RenderCtx &rctx) final;


			//=============== gestione dei ctx
			Ctx*	ctx__crete_new (const char *name, u16 estimated_num_vtx);
			void	ctx__delete (const char *name);
			Ctx*	ctx__get (const char *name);


		private:
            struct SceneData
            {
                gos::mat4x4f    matVP;
                gos::vec2f      screen_wh;
            };

			struct sSBO_segment
			{
				GPUStorageBufferHandle      gpu_handle;
				gpu::sMappedBuffer          mapped;
				u32                         size;
			};

			struct sSBO_vtx
			{
				GPUStorageBufferHandle      gpu_handle;
				gpu::sMappedBuffer          mapped;
				u32                         size;
			};

		private:
			static constexpr u32	NUM_MAX_SEGMENT_IN_BUFFER = 0xFFFF; //8192;
			static constexpr u32	NUM_MAX_VTX_IN_BUFFER	= 0xFFFF; //8192;
			static constexpr u8		FLAG__BEGIN_INVOKED	= 0;

		private:
			struct sState
			{
				u32		cur_color_ARGB;
				bool 	bDepthTestEnabled;
				bool 	bDepthWriteEnabled;
				u32		cur_line_width;
				u32		cur_point_radius;

				u32 	first_instance_index;
				u32 	num_seg_to_draw;
			};

			struct sCtxEntry
			{
				Ctx		*ctx;
				char 	name[32];
			};

		private:
			void 	priv_unsetup();
			void 	priv_flushProgram(sState &state);
			u32		priv_ctx__get (const char *name) const;
			void	priv_begin(gos::geom::Camera3 *cam, gpu::RenderCtx *rctx);
			void	priv_appendToCommandBuffer (const Ctx *ctx);
			void	priv_end();

		private:
            gos::Allocator              *localAllocator;
            gos::Engine                 *engine;
            gos::GPU                    *gpu;
            ENGPipeline                 handle_pipeline;
			GPUDescrPoolHandle          handle_descrPool;
            GPUDescrSetInstanceHandle   handle_descrSet0;
            GPUDescrSetInstanceHandle   handle_descrSet1;
			GPUUniformBufferHandle      handle_ubo_scene;
			ENGGPUShape					handle_shape_segmento;
			sSBO_segment				sbo_segment;
			sSBO_vtx					sbo_vtx;
			Flag8						flag;
			u32							num_vtx_in_buffer;
			u32 						num_seg_in_buffer;
			sState 						state;
			gpu::RenderCtx 				*rctx;
			const res::GPUShape 		*res_shape_segmento;

			gos::FastArray<sCtxEntry>	ctx_list;
		};

	} //namespace engine
} //namespace gos


#endif //gosEngineRenderPipe_line3d


