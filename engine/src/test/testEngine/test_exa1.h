#ifndef _test_exa1_h_
#define _test_exa1_h_
#include "gosEngine.h"
#include "gosEngine_renderer.h"
#include "../gosGPU/utils/gosFreeMovement.h"
#include "../gosGPU/utils/gosFPSMovement.h"
#include "gosUniqueSortedList.h"


class Test_exa1
{
public:

	//******************************************
	class ExaGenerator
	{
	public:
		struct sTris
		{
			u16	vtx_idx0;
			u16	vtx_idx1;
			u16	vtx_idx2;
		};

		struct sQuad
		{
			u16	vtx_idx0;
			u16	vtx_idx1;
			u16	vtx_idx2;
			u16	vtx_idx3;
		};

		struct sEdgeToRemove
		{
			u32	tris_index;
			u8	which_edge;
			u16	edge_vtx0;
			u16 edge_vtx1;
		};

	public:
		typedef gos::FastArray<gos::vec3f> 	PointList;
		typedef gos::FastArray<sTris>		TrisList;
		typedef gos::FastArray<sQuad>		QuadList;

	public:
				ExaGenerator();
				~ExaGenerator();

		//build e' una standalone, fa tutto quello che deve fare e filla vtxList, trisList, quadList, listOfBorderVtxIndex
		//con i dati rilevanti
		void	build();

		//queste sono utilizzate per la versione "interattiva" dell'algoritmo
		void	create_default_exa();
		void	simplify_90();
		void	subdivide();
		void	relax();

		void	select_edge_to_remove (sEdgeToRemove *out);
		bool	try_remove_edge (const sEdgeToRemove &edge);

	public:
		PointList						vtxList;
		TrisList						trisList;
		QuadList						quadList;
		gos::UniqueSortedList<u32>		listOfBorderVtxIndex;	//elenco degli indici dei vtx che rappresentano il bordo dell'exa

	private:
		u32		find_in_pointList (const PointList &list, u32 index_start, const gos::vec3f &v_to_be_found) const;
		f32		quad_calc_area (const gos::vec3f *vtx) const;
		void	quad_get_vertex (u32 quadIndex, gos::vec3f *out) const;
		void	relax_2();
		
		

	private:
		gos::Allocator *allocator;

	}; //class ExaGenerator


	//******************************************
public:
					Test_exa1();
					~Test_exa1();
	void			run (gos::Engine *engine);

private:
	struct sRing
	{
		u32	first_vtx_idx;
		u32	num_vtx;
	};

private:
	void			doCPUStuff ();
    void    		priv_loop();

	void 			build_line_ctx ();
	gos::vec3f		calc_tris_center(u32 tris_index) const;


private:
	gos::Allocator					*allocator;
	gos::Engine						*engine;
	gos::GPU						*gpu;
    GPURenderTargetHandle			handle_rt0;
	GPUZBufferHandle				handle_zb;

	gos::geom::Camera3				cam;
	gos::engine::Rend_line3d		*rend_line3d;
    gos::FreeMovement				movement;

	gos::engine::Rend_line3d::Ctx 	line_ctx1;
	gos::engine::Rend_line3d::Ctx 	line_ctx2;
	gos::engine::Rend_line3d::Ctx 	line_ctx3;
	
	ExaGenerator					exagen;
	ExaGenerator::sEdgeToRemove 	edge_to_remove;
};

#endif //_test_exa1_h_