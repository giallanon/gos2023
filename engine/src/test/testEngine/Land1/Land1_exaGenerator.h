#ifndef _Land1_exaGenerator_h_
#define _Land1_exaGenerator_h_
#include "Land1_enumAndDefine.h"
#include "gosUniqueSortedList.h"
#include "gosRandom.h"


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
			sQuad()													{ }
			sQuad (u16 idx1, u16 idx2, u16 idx3, u16 idx4)			{ idx[0] = idx1; idx[1] = idx2; idx[2] = idx3; idx[3] = idx4; }
			
			u16	idx[4];
		};

		struct VtxInfo
		{
					VtxInfo (const gos::vec2f p)							{ reset(); pos = p; }
			void	reset()													{ pos.set(0, 0); num_adjacent_quad = 0; isBorderVtx = 0; }

		public:
			gos::vec2f	pos;
			u16			adjacent_quad_list[8];
			u8			num_adjacent_quad;
			u8			isBorderVtx;
		};

	public:
		typedef gos::FastArray<gos::vec2f> 	PointList;
		typedef gos::FastArray<sQuad>		QuadList;
		typedef gos::FastArray<VtxInfo>		VtxList;

	public:
		VtxList				vtxList;
		QuadList			quadList;
		PointList			quadCenterList;

	public:
				ExaGenerator()			{ allocator = NULL; }
				~ExaGenerator()			{ unsetup(); }

		void	setup (gos::Allocator *allocatorIN);
		void	unsetup();

		//build e' una standalone, fa tutto quello che deve fare e filla vtxList, trisList, quadList, listOfBorderVtxIndex
		//con i dati rilevanti
		void	build (f32 hex_radius, const gos::vec2f center, gos::Random *rnd);
		void	translate (const gos::vec2f &tr);

		bool		is_a_border_vertex (u32 vtx_index) const						{ assert(vtx_index < vtxList.getNElem()); return (vtxList(vtx_index).isBorderVtx != 0); }
		void		quad_get_vertex (u32 quadIndex, gos::vec2f *out) const;
		gos::vec2f	quad_calc_center (u32 quad_index) const;

					//dato un vtx_index, ritorna l'elenco dei quad che sharano lo stesso vtx
					//Ritorna il num di quad_index inseriti in <out__quadList>
					//I quad sono ordinati in senso orario
		u32			get_quad_from_vtx (u32 vtx_index, u32 *out__quadList, u32 num_elem_in_quad_list) const;

					//dato il quad <quad_index>, e posto che <vtx_index_A> sia uno dei suoi 4 vertici,
					//ritorna il vtx_index_B del vertice del quad appartenente al lato (vtx_index_A, vtx_index_B) del quad stesso
		u16			get_index_of_vtx_in_uscita_da (u32 quad_index, u16 vtx_index_A) const;
		u16			get_index_of_vtx_in_entrata_a (u32 quad_index, u16 vtx_index_A) const;


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
		typedef gos::UniqueSortedList<u32>	BorderVtxList;

	private:
		void	create_default_exa(f32 radius, BorderVtxList *listOfBorderVtxIndex);
		void	simplify_90();
		void	subdivide(BorderVtxList *listOfBorderVtxIndex);
		void	select_edge_to_remove (sEdgeToRemove *out);
		bool	try_remove_edge (const sEdgeToRemove &edge);
		u32		find_in_pointList (const VtxList &list, u32 index_start, const gos::vec2f &v_to_be_found) const;
		f32		quad_calc_area (const gos::vec2f *vtx) const;
		u16		priv_get_index_of_vtx_in_uscita_da (u32 quad_index, u16 vtx_index_A) const;
		void	relax_2 (const BorderVtxList *listOfBorderVtxIndex);

		void	remap();

	private:
		gos::Allocator *allocator;
		gos::Random		*rnd;
		TrisList		trisList;
		


	}; //class ExaGenerator

} //namespace Land1

#endif //_Land1_exaGenerator_h_