#ifndef _gosEngine_rend_line2d_h_
#define _gosEngine_rend_line2d_h_
#include "../gosEngineEnumAndDefine.h"

namespace gos
{
	class Engine; //fwd

	namespace engine
	{
		/**************
		* @brief	
		* 
		* 
		*/
		class Rend_line2d
		{
		public:
			struct Ctx
			{
			private:
						Ctx();
						~Ctx()																	{ unsetup(); }
				void	setup (gos::Allocator *allocator, u16 estimated_num_vtx);
				void	unsetup ();

				void	clear();
				u16		vtx_add (const vec3f &p);
				u16		vtx_add (f32 x, f32 y, f32 z)											{ return vtx_add (vec3f(x, y, z)); }

				void	line_begin();
				void	line_add_vtx (f32 x, f32 y, f32 z)										{ line_add_vtx (vec3f(x, y, z)); }
				void	line_add_vtx (const vec3f &p)											{ const u16 vtx_index = vtx_add(p); line_add_vtx(vtx_index); }
				void	line_add_vtx (u16 vtx_index);
				void	line_end();

			private:
				enum class eCMD : u16
				{
					line_def	= 0x0001,
				};

			private:
				FastArray<vec3f>	vtxList;
				FastArray<u16>		program;
				u32					line_started_at;			

			friend Rend_line2d;
			};

		public:
					Rend_line2d();
					~Rend_line2d();

			bool	setup (gos::Allocator *allocator, gos::Engine *engineIN);
			void	unsetup();


			void	clear(Ctx *ctx)																	{ ctx->clear(); }
			u16		vtx_add (Ctx *ctx, const vec3f &p)												{ ctx->vtx_add(p); }
			u16		vtx_add (Ctx *ctx, f32 x, f32 y, f32 z)											{ return ctx->vtx_add (vec3f(x, y, z)); }

			void	line_begin(Ctx *ctx)															{ ctx->line_begin(); }
			void	line_add_vtx (Ctx *ctx, f32 x, f32 y, f32 z)									{ ctx->line_add_vtx (vec3f(x, y, z)); }
			void	line_add_vtx (Ctx *ctx, const vec3f &p)											{ const u16 vtx_index = ctx->vtx_add(p); ctx->line_add_vtx(vtx_index); }
			void	line_add_vtx (Ctx *ctx, u16 vtx_index)											{ ctx->line_add_vtx(vtx_index); }
			void	line_end(Ctx *ctx)																{ ctx->line_end(); }

			//utils
			void	line (Ctx *ctx, f32 p1x, f32 p1y, f32 p1z, f32 p2x, f32 p2y, f32 p2z)			{ line_begin(ctx); line_add_vtx(ctx, p1x, p1y, p1z); line_add_vtx(ctx, p2x, p2y, p2z); line_end(ctx); }
			void	line (Ctx *ctx, const vec3f &p1, const vec3f &p2)								{ line (ctx, p1.x, p1.y, p1.z, p2.x, p2.y, p2.z); }


			//render
			void	appendToCommandBuffer (Ctx *ctx, gos::gpu::pipe2::CmdBufferWriter2::BeginRend &rend);

		private:
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
			static constexpr u32	NUM_MAX_SEGMENT_IN_BUFFER = 8192;
			static constexpr u32	NUM_MAX_VTX_IN_BUFFER	= 8192;

		private:
            gos::Allocator              *localAllocator;
            gos::Engine                 *engine;
            gos::GPU                    *gpu;
            ENGPipeline                 handle_pipeline;
			ENGGPUShape					handle_shape_segmento;
			sSBO_segment				sbo_segment;
			sSBO_vtx					sbo_vtx;




		};

	} //namespace engine
} //namespace gos


#endif //_gosEngine_rend_line2d_h_


