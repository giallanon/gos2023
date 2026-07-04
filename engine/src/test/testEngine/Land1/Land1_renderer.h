#ifndef _Land1_renderer_h_
#define _Land1_renderer_h_
#include "Land1_Exa.h"
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

		bool 	on__attach (const RPIPE::Context &ctx, u8 renderer_UID) final;
		void 	on__detach (const RPIPE::Context &ctx) final										{ priv_unsetup(); }
		void 	on__render (const RPIPE::Context &ctx, gos::gpu::RenderCtx &rctx) final;


	private:
		static constexpr u32 HEXA__NUM_VTX = 1024;
		static constexpr u32 NUM_MAX_EXA = 0xFFFF / HEXA__NUM_VTX;

		static constexpr u32 HEXA__MAX_NUM_QUAD = 900;



	private:
		struct SBO_ExaVtxList
		{
			GPUStorageBufferHandle	handle_sbo;
			gos::gpu::sMappedBuffer	mapped_buffer;
			u32						sizeof_buffer;
		};

		struct sVtxInfo
		{
    		u32	height;
			u32 material_index;
		};

		struct SBO_ExaVtxInfo
		{
			GPUStorageBufferHandle	handle_sbo;
			gos::gpu::sMappedBuffer	mapped_buffer;
			u32						sizeof_buffer;
		};		

		struct sInstanceData
		{
			u32 quad_indices_0_1;	//2 indici da 16 bit
			u32 quad_indices_2_3;	//altri 2 indici da 16 bit
			u32 reference_vtx_index;
		};

		struct SBO_PackedInstanceData
		{
			GPUStorageBufferHandle	handle_sbo;
			gos::gpu::sMappedBuffer	mapped_buffer;
			u32						sizeof_buffer;
		};

		struct SBO_MeshInstanceData
		{
			GPUStorageBufferHandle	handle_sbo;
			gos::gpu::sMappedBuffer	mapped_buffer;
			u32						sizeof_buffer;
		};		

		static constexpr u32 NUM_MAX_INSTANCE_PER_MESH = NUM_MAX_EXA * HEXA__MAX_NUM_QUAD;
		struct MeshInstanceData
		{
			u32	*quad_index_list;
			u32	num_quad;
			u32 start_index_in_SBO;
		};

	private:
		void	priv_unsetup();
		void	priv_do_render(const RPIPE::Context &ctx, gos::gpu::RenderCtx &rctx);
		void	priv_add_vtx (const gos::vec2f &v);
		void	priv_add_vtxInfo (u32 height, u32 material_index);
		void	priv_add_quad (Exa::eMeshType mesh_type, u32 reference_vtx_index, u32 idx1, u32 idx2, u32 idx3, u32 idx4);

		void	priv_begin2();
		void	priv_add_exa2 (const Exa *exa);
		void	priv_end2();


	private:
		gos::Allocator					*localAllocator;
		gos::Engine						*engine;
		gos::GPU						*gpu;

		gos::ENGPipeline 				handle_pipeline;
		GPUDescrSetInstanceHandle   	handle_descrSet2;

		SBO_ExaVtxList					sbo_exaVtxList;
		SBO_ExaVtxInfo					sbo_exaVtxInfo;
		SBO_PackedInstanceData			sbo_packedInstanceData;
		SBO_MeshInstanceData			sbo_meshInstanceData;

		gos::ENGModel3d 				handle__model_tile1;
		const gos::ENGGPUShape			*shape_list;

		u32 		num_vtx;
		u32 		num_vtxInfo;
		u32			num_quad;
		MeshInstanceData	mesh_instance_data[(u32)Exa::eMeshType::_COUNT];
	};

} //namespace Land1
#endif //_Land1_renderer_h_


