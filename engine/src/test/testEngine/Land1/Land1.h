#ifndef _Land1_h_
#define _Land1_h_
#include "../DefaultApp/DefaultApp.h"
#include "../gosGameUtils/hexmap/gosHexMap.h"

/******************************************
* Land1 
*
*/
class Land1 : public gos::engine::RenderPipe::Renderer
{
public:
	static constexpr f32 HEX_RADIUS = 3.0f;

public:
	using RPIPE = gos::engine::RenderPipe;

public:
			Land1();
			~Land1()										{ priv_unsetup(); }

	bool 	on__attach (const RPIPE::Context &ctx) final;
	void 	on__detach (const RPIPE::Context &ctx) final;
	void 	on__render (const RPIPE::Context &ctx, gos::gpu::RenderCtx &rctx) final;

private:
	/******************************************
	* ExaGenerator
	* 
	*/
	class ExaGenerator
	{
	public:
		struct sQuad
		{
			u16	vtx_idx0;
			u16	vtx_idx1;
			u16	vtx_idx2;
			u16	vtx_idx3;
			f32 height;
			u32 material_index;
		};

	public:
		typedef gos::FastArray<gos::vec3f> 	PointList;
		typedef gos::FastArray<sQuad>		QuadList;

	public:
				ExaGenerator()									{ allocator = NULL; }
				~ExaGenerator()									{ unsetup(); }

		void	setup (gos::Allocator *allocatorIN);
		void	unsetup();

		//build e' una standalone, fa tutto quello che deve fare e filla vtxList, trisList, quadList, listOfBorderVtxIndex
		//con i dati rilevanti
		void	build (f32 radius, const gos::vec3f &center);
		void	translate (const gos::vec3f &tr);

		gos::vec3f	quad_center (u32 quad_number) const;

	public:
		PointList						vtxList;
		QuadList						quadList;
		gos::UniqueSortedList<u32>		listOfBorderVtxIndex;	//elenco degli indici dei vtx che rappresentano il bordo dell'exa

	private:
		struct sTris
		{
			u16	vtx_idx0;
			u16	vtx_idx1;
			u16	vtx_idx2;
		};

		struct sEdgeToRemove
		{
			u32	tris_index;
			u8	which_edge;
			u16	edge_vtx0;
			u16 edge_vtx1;
		};

		struct sRing
		{
			u32	first_vtx_idx;
			u32	num_vtx;
		};

	private:
		typedef gos::FastArray<sTris>		TrisList;

	private:
		void	create_default_exa(f32 radius, const gos::vec3f &center);
		void	simplify_90();
		void	subdivide();
		void	relax();
		void	select_edge_to_remove (sEdgeToRemove *out);
		bool	try_remove_edge (const sEdgeToRemove &edge);
		u32		find_in_pointList (const PointList &list, u32 index_start, const gos::vec3f &v_to_be_found) const;
		f32		quad_calc_area (const gos::vec3f *vtx) const;
		void	quad_get_vertex (u32 quadIndex, gos::vec3f *out) const;
		void	relax_2();

	private:
		gos::Allocator *allocator;
		TrisList		trisList;


	}; //class ExaGenerator


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
	void	add__exa(const ExaGenerator *exa);
	void 	priv_generate_terrain();


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

	gos::FastArray<ExaGenerator*>	exagenList;
};

#endif //_Land1_h_
