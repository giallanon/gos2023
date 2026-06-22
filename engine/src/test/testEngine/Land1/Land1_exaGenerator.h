#ifndef _Land1_exaGenerator_h_
#define _Land1_exaGenerator_h_
#include "Land1_enumAndDefine.h"
#include "gosUniqueSortedList.h"


namespace Land1
{
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
		};

	public:
		typedef gos::FastArray<gos::vec3f> 	PointList;
		typedef gos::FastArray<sQuad>		QuadList;

	public:
		PointList						vtxList;
		QuadList						quadList;

	public:
				ExaGenerator()			{ allocator = NULL; }
				~ExaGenerator()			{ unsetup(); }

		void	setup (gos::Allocator *allocatorIN);
		void	unsetup();

		//build e' una standalone, fa tutto quello che deve fare e filla vtxList, trisList, quadList, listOfBorderVtxIndex
		//con i dati rilevanti
		void	build (f32 hex_radius, const gos::vec3f &world_center);
		void	translate (const gos::vec3f &tr);

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
		gos::UniqueSortedList<u32>		listOfBorderVtxIndex;	//elenco degli indici dei vtx che rappresentano il bordo dell'exa


	}; //class ExaGenerator

} //namespace Land1

#endif //_Land1_exaGenerator_h_