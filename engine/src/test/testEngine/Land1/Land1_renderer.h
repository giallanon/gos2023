#ifndef _Land1_renderer_h_
#define _Land1_renderer_h_
#include "Land1_enumAndDefine.h"
#include "../DefaultApp/DefaultApp.h"
#include "../gosGameUtils/examap/gosExamap.h"


namespace Land1
{
	/******************************************
	* Renderer
	*
	*/
	class Renderer : public gos::engine::RenderPipe::Renderer
	{
	public:
		using RPIPE = gos::engine::RenderPipe;

	public:
				Renderer();
				~Renderer() { priv_unsetup(); }

		void	begin();
		void	add_exa (const Exa *exa);
		void	end();

		bool 	on__attach (const RPIPE::Context &ctx) final;
		void 	on__detach (const RPIPE::Context &ctx) final;
		void 	on__render (const RPIPE::Context &ctx, gos::gpu::RenderCtx &rctx) final;


	private:
		static const u32 NUM_MAX_EXA = 256;
		static const u32 HEXA__NUM_VTX = 256;
		static const u32 HEXA__AVG_NUM_QUAD = 256;



	private:
		struct sExaVtxList
		{
			GPUStorageBufferHandle	handle_sbo;
			gos::gpu::sMappedBuffer	mapped_buffer;
			u32						sizeof_buffer;
		};

		struct sInstanceData
		{
			u32 quad_indices_0_1;	//2 indici da 16 bit
			u32 quad_indices_2_3;	//altri 2 indici da 16 bit
			u32 material_index;
			f32	height;
		};

		struct sPackedInstanceData
		{
			GPUStorageBufferHandle	handle_sbo;
			gos::gpu::sMappedBuffer	mapped_buffer;
			u32						sizeof_buffer;
		};


	private:
		void	priv_unsetup();
		void	priv_do_render(const RPIPE::Context &ctx, gos::gpu::RenderCtx &rctx);
		void	priv_add_vtx (const gos::vec3f &v);
		void	priv_add_quad (u32 idx1, u32 idx2, u32 idx3, u32 idx4, f32 height, u32 material_index);
		void	priv_add_exa (const Exa *exa);


	private:
		gos::Allocator					*localAllocator;
		gos::Engine						*engine;
		gos::GPU						*gpu;

		gos::ENGPipeline 				handle_pipeline;
		GPUDescrSetInstanceHandle   	handle_descrSet2;

		sExaVtxList						exaVtxList;
		sPackedInstanceData				packedInstanceData;

		gos::ENGModel3d 				handle__model_tile1;
		const gos::ENGGPUShape			*shape_list;

		u32 		num_vtx;
		u32 		num_quad;
	};

} //namespace Land1
#endif //_Land1_renderer_h_


