#ifndef _gosEngine_rend_line3d_h_
#define _gosEngine_rend_line3d_h_
#include "../gosEngineRes.h"
#include "dataTypes/gosColorU32.h"

namespace gos
{
	class Engine; //fwd

	namespace engine
	{
		/**************
		* @brief	Rend_line3d
		* 			Renderer per il rendering di line in 3d
		* 
		*/
		class Rend_line3d
		{
		public:
			/*****************
			 * @brief	Ctx
			 * 			un contex contiene un elenco di punti, linee, colori e settaggi vari
			 * 			Per renderizzare un ctx, utilizzare Rend_line3d->appendToCommandBuffer()
			 */
			class Ctx
			{
			public:
						Ctx();
						~Ctx()																	{ unsetup(); }

				void	setup (gos::Allocator *allocator, u16 estimated_num_vtx);
				void	unsetup ();

				void	clear();

				void	set_color_ARGB (const gos::ColorU32 &col)								{ set_color_ARGB (col.argb); }
				void	set_color_ARGB (u32 argb);
				void	set_line_width (u16 w);
				
				void 	enable_depth_test(bool b);
				void 	enable_depth_write(bool b);

				u16		vtx_add (const vec3f &p);
				u16		vtx_add (f32 x, f32 y, f32 z)											{ return vtx_add (vec3f(x, y, z)); }

				void	line_begin();
				void	line_add_vtx (f32 x, f32 y, f32 z)										{ line_add_vtx (vec3f(x, y, z)); }
				void	line_add_vtx (const vec3f &p)											{ const u16 vtx_index = vtx_add(p); line_add_vtx(vtx_index); }
				void	line_add_vtx (u16 vtx_index);
				void	line_end();

				void	line (f32 p1x, f32 p1y, f32 p1z, f32 p2x, f32 p2y, f32 p2z)				{ line_begin(); line_add_vtx(p1x, p1y, p1z); line_add_vtx(p2x, p2y, p2z); line_end(); }
				void	line (const vec3f &p1, const vec3f &p2)									{ line (p1.x, p1.y, p1.z, p2.x, p2.y, p2.z); }
				void	line (u16 vtx_index1, u16 vtx_index2)									{ line_begin(); line_add_vtx(vtx_index1); line_add_vtx(vtx_index2); line_end(); }
				void	closed_line (const FastArray<vec3f> &vtxList, u32 num_vtx);

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
				};

			private:
				FastArray<vec3f>	vtxList;
				FastArray<u16>		program;
				u32					line_started_at;			

			friend Rend_line3d;
			};

		public:
					Rend_line3d();
					~Rend_line3d();

			bool	setup (gos::Allocator *allocator, gos::Engine *engineIN);
			void	unsetup();

			void	begin(gos::geom::Camera3 *cam, gpu::RenderCtx *rctx);
			void	appendToCommandBuffer (const Ctx &ctx);
			void	end();

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

				u32 	first_instance_index;
				u32 	num_seg_to_draw;
			};

		private:
			void 	priv_flushProgram(sState &state);

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
			const engine::ResGPUShape 	*res_shape_segmento;




		};

	} //namespace engine
} //namespace gos


#endif //_gosEngine_rend_line3d_h_


