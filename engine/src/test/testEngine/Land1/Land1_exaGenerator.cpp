#include "Land1_exaGenerator.h"
#include "gos.h"
#include "gosGeomUtils.h"

using namespace gos;
using namespace Land1;


//***************************************
static f32 ExaGenerator__calc_angle (const vec2f p)
{
	f32 ret = atan2f (p.y, p.x);
	ret -= math::PIMEZZI;
	if (ret < 0)
		ret += math::DUEPI;
	if (ret > math::DUEPI)
		ret -= math::DUEPI;
	return ret;
}

//***************************************
void ExaGenerator::ExaGenerator::unsetup ()
{
	if (NULL == allocator)
		return;

	vtxList.unsetup ();
	trisList.unsetup ();
	quadList.unsetup ();
	quadCenterList.unsetup();
	allocator = NULL;
}

//***************************************
void ExaGenerator::setup (gos::Allocator *allocatorIN)
{
	if (NULL != allocator)
		unsetup();

	allocator = allocatorIN;
	vtxList.setup (allocator, 1024);
	trisList.setup (allocator, 1024);
	quadList.setup (allocator, 1024);
	quadCenterList.setup (allocator, 1024);
}

//***************************************
void ExaGenerator::translate (const gos::vec2f &tr)
{
	const u32 n = vtxList.getNElem();
	for (u32 i=0; i<n; i++)
		vtxList[i].pos += tr;
}

//***************************************
void ExaGenerator::build (f32 hex_radius, const gos::vec2f center, gos::Random *rndIN)
{
	assert (NULL != allocator);
	rnd = rndIN;

	//creazione dell'exa
	{
		BorderVtxList listOfBorderVtxIndex;
		listOfBorderVtxIndex.setup (allocator, 1024);

		create_default_exa (hex_radius, &listOfBorderVtxIndex);

		simplify_90();

		subdivide(&listOfBorderVtxIndex);

		for (u32 i=0;i<60; i++)
			relax_2(&listOfBorderVtxIndex);

		translate (center);

		//segno quali sono i vtx del bordo
		listOfBorderVtxIndex.forEach ([&vtxList=this->vtxList](u32 key, u32 value) {
			vtxList[value].isBorderVtx = 1;
			return true;
		});
	}

	//remap vtx
	remap();


	//calcolo il centro di ogni quad
	const u32 num_quad = quadList.getNElem();
	for (u32 i = 0; i < num_quad; i++)
	{
		quadCenterList[i] = quad_calc_center(i);

		//ordino i vtx del quad in senso orario
		f32 angle_list[4];
		for (u32 i2=0; i2<4; i2++)
		{
			const vec2f v = vtxList[ quadList[i].idx[i2] ].pos;
			const vec2f p = v - quadCenterList[i];
			angle_list[i2] = ExaGenerator__calc_angle (p);
		}

		bool bEsci = false;
		u32 n = 4;
		while (bEsci == false)
		{
			bEsci = true;
			n--;
			
			for (u32 i2 = 0; i2 < n; i2++)
			{
				if (angle_list[i2] < angle_list[i2 + 1])
				{
					bEsci = false;
					GOSSWAP(angle_list[i2], angle_list[i2 + 1]);
					GOSSWAP(quadList[i].idx[i2], quadList[i].idx[i2 + 1]);
				}
			}
		}		
	}

	//per ogni vtx, vedo quali sono i quad che lo sharano
	const u32 num_vtx = vtxList.getNElem();
	for (u32 i = 0; i < num_vtx; i++)
	{
		//recupero i quad che sharano il vtx i-esimo
		u32 quads[8];
		vtxList[i].num_adjacent_quad = get_quad_from_vtx (i, quads, 8);

		for (u32 ct = 0; ct < vtxList[i].num_adjacent_quad; ct++)
		{
			vtxList[i].adjacent_quad_list[ct] = (u16)quads[ct];
		}
	}
}

//***************************************
void ExaGenerator::create_default_exa (f32 radiusIN, BorderVtxList *listOfBorderVtxIndex)
{
	vtxList.reset();
	trisList.reset();
	quadList.reset();
	listOfBorderVtxIndex->reset();
	quadCenterList.reset();

	static constexpr u32 NUM_RINGS = 4;
	
	const vec2f	center(0,0);
	PointList exaVtx (allocator, 16);
	FastArray<sRing> ringIndexStartList(allocator, 32);

	//il ring 0 ha solo il vtx centrale
	vtxList.append (center);
	ringIndexStartList.append(sRing{ 0, 1 });

	//gli altri ring..
	f32 radius = 1;
	for (u32 ringLevel = 1; ringLevel <= NUM_RINGS; ringLevel++)
	{
		//creo un anello di vertixi
		const u32 ring_start_at_index = vtxList.getNElem();
		ringIndexStartList.append(sRing{
				.first_vtx_idx = ring_start_at_index,
				.num_vtx = 6 * ringLevel });

		exaVtx.reset();
		gos::geom::circle (&exaVtx, center, radius, 6);

		//devo aggiungere dei vtx a seconda di quanto esterno e' il ring
		if (1 == ringLevel)
		{
			for (u32 i = 0; i < 6; i++)
			{
				vtxList.append(exaVtx(i));

				const u32 vtx_idx1 = ring_start_at_index + i;
				u32 vtx_idx2 = vtx_idx1 + 1;
				if (i == 5)
					vtx_idx2 = ring_start_at_index;
				trisList.append (sTris{ (u16)(vtx_idx1), 0, (u16)(vtx_idx2) });
			}
		}
		else
		{
			const bool isLastRingLevel = (ringLevel == NUM_RINGS);
			const sRing *internal_ring_info = &ringIndexStartList(ringLevel - 1);
			const sRing *external_ring_info = &ringIndexStartList(ringLevel);

			exaVtx.append (exaVtx(0));
			for (u32 i = 0; i < 6; i++)
			{
				//per ogni lato dell'exa, deve generare vtx addizionali in base al ringLevel
				const vec2f vtx1 = exaVtx(i);
				const vec2f vtx2 = exaVtx(i + 1);

				vtxList.append(vtx1);
				if (isLastRingLevel)
					listOfBorderVtxIndex->insertIfNotExists (vtxList.getNElem() - 1);

				{
					const f32 tIncr = 1.0f / (f32)ringLevel;
					f32 t = 0;
					for (u32 i2 = 0; i2 < (ringLevel - 1); i2++)
					{
						t += tIncr;
						const vec2f mid = vtx1 + (vtx2 - vtx1) * t;
						vtxList.append(mid);
						if (isLastRingLevel)
							listOfBorderVtxIndex->insertIfNotExists (vtxList.getNElem() - 1);

					}
				}
			}

			//genero i tris per ogni lato dell'exa
			u32 idxA = external_ring_info->first_vtx_idx;;
			u32 idxB = internal_ring_info->first_vtx_idx;
			for (u32 i = 0; i < 6; i++)
			{
				u32 idxA1;
				u32 idxB1;
				for (u32 i3 = 0; i3 < ringLevel - 1; i3++)
				{
					assert (idxA < external_ring_info->first_vtx_idx + external_ring_info->num_vtx);
					assert (idxB < internal_ring_info->first_vtx_idx + internal_ring_info->num_vtx);

					idxA1 = idxA + 1;
					if (idxA1 >= external_ring_info->first_vtx_idx + external_ring_info->num_vtx)
						idxA1 = external_ring_info->first_vtx_idx;

					idxB1 = idxB + 1;
					if (idxB1 >= internal_ring_info->first_vtx_idx + internal_ring_info->num_vtx)
						idxB1 = internal_ring_info->first_vtx_idx;

					trisList.append (sTris{ (u16)(idxA), (u16)(idxB), (u16)(idxA1) });
					trisList.append (sTris{ (u16)(idxA1), (u16)(idxB), (u16)(idxB1) });
					idxA = idxA1;
					idxB = idxB1;
				}

				if (i == 5)
				{
					idxA1 = external_ring_info->first_vtx_idx;
				}
				else
				{
					idxA1 = idxA + 1;
				}

				trisList.append (sTris{ (u16)(idxA), (u16)(idxB), (u16)(idxA1) });
				idxA = idxA1;
			}
		}

		radius += 1.0f;
	}

	//scalo per fittare "radiusIN"
	const f32 s = radiusIN / (radius-1.0f);
	const u32 n = vtxList.getNElem();
	for (u32 i=0; i<n; i++)
	{
		vtxList[i].pos *= s;
	}	
}

//***************************************
void ExaGenerator::select_edge_to_remove (sEdgeToRemove *out)
{
	//logger::log (eTextColor::grey, "select_edge_to_remove... ");

	const u32 nTris = trisList.getNElem();
	out->tris_index = rnd->getU32(nTris - 1);

	switch (rnd->getU32(2))
	{
	default:
		DBGBREAK;
		break;

	case 0:	//edge 0-1
		//logger::log (eTextColor::grey, "edge 0-1\n");
		out->which_edge = 0;
		out->edge_vtx0 = trisList(out->tris_index).vtx_idx0;
		out->edge_vtx1 = trisList(out->tris_index).vtx_idx1;
		break;

	case 1:	//edge 1-2
		//logger::log (eTextColor::grey, "edge 1-2\n");
		out->which_edge = 1;
		out->edge_vtx0 = trisList(out->tris_index).vtx_idx1;
		out->edge_vtx1 = trisList(out->tris_index).vtx_idx2;
		break;

	case 2:	//edge 2-0
		//logger::log (eTextColor::grey, "edge 2-0\n");
		out->which_edge = 2;
		out->edge_vtx0 = trisList(out->tris_index).vtx_idx2;
		out->edge_vtx1 = trisList(out->tris_index).vtx_idx0;
		break;
	}
}

//***************************************
bool ExaGenerator::try_remove_edge (const sEdgeToRemove &edge)
{
	//logger::log (eTextColor::grey, "try_remove_edge...\n");

	//so per certo che e' coinvolto il tris <edge.tris_index>
	//deve essere coinvolto anche un altro tris che condivide lo stesso edge
	const u32 nTris = trisList.getNElem();

	sEdgeToRemove edge2;
	edge2.tris_index = u32MAX;
	u32 vtx_idx_to_add = 0;
	for (u32 i=0; i<nTris; i++)
	{
		//cero un tris con edge <edge.edge_vtx1> <edge.edge_vtx0>
		if (trisList(i).vtx_idx0 == edge.edge_vtx1 && trisList(i).vtx_idx1 == edge.edge_vtx0)
		{
			edge2.tris_index = i;
			edge2.which_edge = 0;
			edge2.edge_vtx0 = trisList(i).vtx_idx0;
			edge2.edge_vtx1 = trisList(i).vtx_idx1;
			vtx_idx_to_add = trisList(i).vtx_idx2;
			break;
		}
		else if (trisList(i).vtx_idx1 == edge.edge_vtx1 && trisList(i).vtx_idx2 == edge.edge_vtx0)
		{
			edge2.tris_index = i;
			edge2.which_edge = 1;
			edge2.edge_vtx0 = trisList(i).vtx_idx1;
			edge2.edge_vtx1 = trisList(i).vtx_idx2;
			vtx_idx_to_add = trisList(i).vtx_idx0;
			break;
		}		
		else if (trisList(i).vtx_idx2 == edge.edge_vtx1 && trisList(i).vtx_idx0 == edge.edge_vtx0)
		{
			edge2.tris_index = i;
			edge2.which_edge = 2;
			edge2.edge_vtx0 = trisList(i).vtx_idx2;
			edge2.edge_vtx1 = trisList(i).vtx_idx0;
			vtx_idx_to_add = trisList(i).vtx_idx1;
			break;
		}		
	}

	if (u32MAX == edge2.tris_index)
	{
		//logger::log ("second tris not found\n");
		return false;
	}

	//logger::log ("edge src:%d, edge dst:%d\n", edge.which_edge, edge2.which_edge);

	//in base all'edge che ho eliminato, creo un quad al posto dei 2 trix
	switch (edge.which_edge)
	{
	default:
		DBGBREAK;
		return false;

	case 0:
		quadList.append (sQuad{
				trisList(edge.tris_index).vtx_idx1,
				trisList(edge.tris_index).vtx_idx2,
				trisList(edge.tris_index).vtx_idx0,
				(u16)vtx_idx_to_add
			});
		break;

	case 1:
		quadList.append (sQuad{
				trisList(edge.tris_index).vtx_idx2,
				trisList(edge.tris_index).vtx_idx0,
				trisList(edge.tris_index).vtx_idx1,
				(u16)vtx_idx_to_add
			});
		break;
		
	case 2:
		quadList.append (sQuad{
				trisList(edge.tris_index).vtx_idx0,
				trisList(edge.tris_index).vtx_idx1,
				trisList(edge.tris_index).vtx_idx2,
				(u16)vtx_idx_to_add
			});
		break;
	}		

	if (edge.tris_index > edge2.tris_index)
	{
		trisList.removeAndSwapWithLast(edge.tris_index);
		trisList.removeAndSwapWithLast(edge2.tris_index);
	}
	else
	{
		trisList.removeAndSwapWithLast(edge2.tris_index);
		trisList.removeAndSwapWithLast(edge.tris_index);
	}	

	return true;
}

//***************************************
void ExaGenerator::simplify_90()
{
	// logger::log ("simplify_90\n");
	// logger::incIndent();
	// logger::log ("tris:%d, quad:%d\n", trisList.getNElem(), quadList.getNElem());
	const u32 tris_limit = (trisList.getNElem() * 20) / 60;
	const u32 max_consecutive_fail = trisList.getNElem() / 2;

	u32 numFail = 0;
	while (trisList.getNElem() > tris_limit)
	{
		sEdgeToRemove edge;
		select_edge_to_remove(&edge);
		if (try_remove_edge(edge))
			numFail = 0;
		else
			numFail++;

		if (numFail > max_consecutive_fail)
			break;
	}

	// logger::log ("tris:%d, quad:%d\n", trisList.getNElem(), quadList.getNElem());
	// logger::decIndent();
}

//***************************************
u32 ExaGenerator::find_in_pointList (const VtxList &list, u32 index_start, const vec2f &v_to_be_found) const
{
	static constexpr f32 MAX_DIST = 0.1f * 0.1f;
	const u32 n = list.getNElem();
	while (index_start < n)
	{
		const f32 d = math::distance2(list(index_start).pos, v_to_be_found);
		if (d < MAX_DIST)
			return index_start;
		index_start++;
	}
	return u32MAX;
}

//***************************************
void ExaGenerator::subdivide(BorderVtxList *listOfBorderVtxIndex)
{
	if (0 == trisList.getNElem())
		return;

	QuadList 	newQuadList (allocator, quadList.getNElem()*4 + trisList.getNElem()*3);
	
	const u32 num_vtx_before_subdivide = vtxList.getNElem();

	//subdivide dei tris
	u32 n = trisList.getNElem();
	for (u32 i=0; i<n; i++)
	{
		const u16 idx0 = trisList(i).vtx_idx0;
		const u16 idx1 = trisList(i).vtx_idx1;
		const u16 idx2 = trisList(i).vtx_idx2;
		const vec2f v0 = vtxList(idx0).pos;
		const vec2f v1 = vtxList(idx1).pos;
		const vec2f v2 = vtxList(idx2).pos;

		const vec2f v_center = (v0 + v1 + v2) / 3.0f;
		const vec2f v_01 = v0 + (v1-v0) * 0.5f;
		const vec2f v_12 = v1 + (v2-v1) * 0.5f;
		const vec2f v_20 = v2 + (v0-v2) * 0.5f;

		const u32 idx_center = vtxList.getNElem();
		vtxList.append(v_center);

		//riguardo agli altri 3 vtx, potrebbe gia' esistere in quanto creati da altri split
		u32 idx_v01 = find_in_pointList (vtxList, num_vtx_before_subdivide, v_01);
		if (u32MAX == idx_v01)
		{
			idx_v01 = vtxList.getNElem();
			vtxList.append(v_01);

			if (listOfBorderVtxIndex->exists(idx0) && listOfBorderVtxIndex->exists(idx1))
			{
				listOfBorderVtxIndex->insertIfNotExists(idx_v01);
			}
		}

		u32 idx_v12 = find_in_pointList (vtxList, num_vtx_before_subdivide, v_12);
		if (u32MAX == idx_v12)
		{
			idx_v12 = vtxList.getNElem();
			vtxList.append(v_12);

			if (listOfBorderVtxIndex->exists(idx1) && listOfBorderVtxIndex->exists(idx2))
			{
				listOfBorderVtxIndex->insertIfNotExists(idx_v12);
			}
		}
		
		u32 idx_v20 = find_in_pointList (vtxList, num_vtx_before_subdivide, v_20);
		if (u32MAX == idx_v20)
		{
			idx_v20 = vtxList.getNElem();
			vtxList.append(v_20);

			if (listOfBorderVtxIndex->exists(idx2) && listOfBorderVtxIndex->exists(idx0))
			{
				listOfBorderVtxIndex->insertIfNotExists(idx_v20);
			}
		}

		newQuadList.append ( sQuad{
			(u16)trisList(i).vtx_idx0,
			(u16)idx_v01,
			(u16)idx_center,
			(u16)idx_v20
		});

		newQuadList.append ( sQuad{
			(u16)idx_v01,
			(u16)trisList(i).vtx_idx1,
			(u16)idx_v12,
			(u16)idx_center

		});	
		
		newQuadList.append ( sQuad{
			(u16)idx_v12,
			(u16)trisList(i).vtx_idx2,
			(u16)idx_v20,
			(u16)idx_center
		});			
	}
	trisList.reset();

	//subdivide dei quad
	n = quadList.getNElem();
	for (u32 i=0; i<n; i++)
	{
		const u16 idx0 = quadList(i).idx[0];
		const u16 idx1 = quadList(i).idx[1];
		const u16 idx2 = quadList(i).idx[2];
		const u16 idx3 = quadList(i).idx[3];

		const vec2f v0 = vtxList(idx0).pos;
		const vec2f v1 = vtxList(idx1).pos;
		const vec2f v2 = vtxList(idx2).pos;
		const vec2f v3 = vtxList(idx3).pos;

		const vec2f v_center = (v0 + v1 + v2 + v3) / 4.0f;
		const vec2f v_01 = v0 + (v1-v0) * 0.5f;
		const vec2f v_12 = v1 + (v2-v1) * 0.5f;
		const vec2f v_23 = v2 + (v3-v2) * 0.5f;
		const vec2f v_30 = v3 + (v0-v3) * 0.5f;

		const u32 idx_center = vtxList.getNElem();
		vtxList.append(v_center);

		//riguardo agli altri vtx, potrebbe gia' esistere in quanto creati da altri split
		u32 idx_v01 = find_in_pointList (vtxList, num_vtx_before_subdivide, v_01);
		if (u32MAX == idx_v01)
		{
			idx_v01 = vtxList.getNElem();
			vtxList.append(v_01);

			if (listOfBorderVtxIndex->exists(idx0) && listOfBorderVtxIndex->exists(idx1))
			{
				listOfBorderVtxIndex->insertIfNotExists(idx_v01);
			}			
		}

		u32 idx_v12 = find_in_pointList (vtxList, num_vtx_before_subdivide, v_12);
		if (u32MAX == idx_v12)
		{
			idx_v12 = vtxList.getNElem();
			vtxList.append(v_12);

			if (listOfBorderVtxIndex->exists(idx1) && listOfBorderVtxIndex->exists(idx2))
			{
				listOfBorderVtxIndex->insertIfNotExists(idx_v12);
			}				
		}
		
		u32 idx_v23 = find_in_pointList (vtxList, num_vtx_before_subdivide, v_23);
		if (u32MAX == idx_v23)
		{
			idx_v23 = vtxList.getNElem();
			vtxList.append(v_23);

			if (listOfBorderVtxIndex->exists(idx2) && listOfBorderVtxIndex->exists(idx3))
			{
				listOfBorderVtxIndex->insertIfNotExists(idx_v23);
			}				
		}

		u32 idx_v30 = find_in_pointList (vtxList, num_vtx_before_subdivide, v_30);
		if (u32MAX == idx_v30)
		{
			idx_v30 = vtxList.getNElem();
			vtxList.append(v_30);

			if (listOfBorderVtxIndex->exists(idx3) && listOfBorderVtxIndex->exists(idx0))
			{
				listOfBorderVtxIndex->insertIfNotExists(idx_v30);
			}				
		}		

		newQuadList.append ( sQuad{
			(u16)quadList(i).idx[0],
			(u16)idx_v01,
			(u16)idx_center,
			(u16)idx_v30
		});

		newQuadList.append ( sQuad{
			(u16)idx_v01,
			(u16)quadList(i).idx[1],
			(u16)idx_v12,
			(u16)idx_center

		});	
		
		newQuadList.append ( sQuad{
			(u16)idx_v12,
			(u16)quadList(i).idx[2],
			(u16)idx_v23,
			(u16)idx_center
		});			

		newQuadList.append ( sQuad{
			(u16)idx_v23,
			(u16)quadList(i).idx[3],
			(u16)idx_v30,
			(u16)idx_center
		});			

	}
	quadList.reset();

	quadList.copyFrom (newQuadList);
}


/***************************************
 * https://www.youtube.com/watch?v=Jm3pLya3d9c
 * per ogni quad
 * 	calcolare AREA del quad
 * 	in base a AREA, calcolare il lato L e la diagnole D del quadrato che avrebbe la stessa area
 * 
 *  per ogni vertice:
 * 		linea dal centro del quad verso verice i; muoviti di distanza (D/2). Questo punto e' l'ipotetico vertice
 * 		dal quale disegnare un quad di lato L
 * 		Calcola la distanza dei vtx del quad dai vertici dell'ipotetico quadrato e memorizzare
 * 
 *  scegliere il quadrato che minimizza il movimento dei vertici.
 * 	muovere "un po'" i vertici in direzione del quadrato
 */
void ExaGenerator::relax_2(const BorderVtxList *listOfBorderVtxIndex)
{
	struct sAdjust
	{
		vec2f	sum;
		u32 	n;
	};

	gos::FastArray<sAdjust> adj;
	adj.setup (gos::getScrapAllocator(), vtxList.getNElem());
	for (u32 i=0; i<vtxList.getNElem(); i++)
	{
		adj[i].sum.set(0,0);
		adj[i].n = 0;
	}

	const u32 n = quadList.getNElem();
	for (u32 i=0; i<n; i++)
	{
		vec2f	vtx[4];
		quad_get_vertex (i, vtx);


		//angoli del quad
		bool bSkip = true;
		{
			vec2f lato[4];
			lato[0] = vtx[1] - vtx[0];
			lato[1] = vtx[2] - vtx[1];
			lato[2] = vtx[3] - vtx[2];
			lato[3] = vtx[0] - vtx[3];

			for (u32 i2=0; i2<4; i2++)
				lato[i2].normalize();

			f32 alfa[4];
			alfa[0] = acosf(math::dot (lato[0], lato[1]));
			alfa[1] = acosf(math::dot (lato[1], lato[2]));
			alfa[2] = acosf(math::dot (lato[2], lato[3]));
			alfa[3] = acosf(math::dot (lato[3], lato[0]));

			constexpr f32 A_MIN = math::gradToRad(-70);
			constexpr f32 A_MAX = math::gradToRad(110);
			for (u32 i2=0; i2<4; i2++)
			{
				if (alfa[i2] < A_MIN || alfa[i2] > A_MAX)
					bSkip = false;
			}
		}

		if (bSkip)
			continue;




		//const f32 area = quad_calc_area (vtx);
		//const f32 lato = sqrtf(area);
		
		const f32 lato = 0.4f;
		const f32 diag_full = lato * 1.4f;
		const f32 diag_half = diag_full * 0.5f;


		// const f32 area_desiderata = lato*lato;
		// const f32 area = quad_calc_area (vtx);
		// if (fabsf(area - area_desiderata) < 0.05f)
		// 	continue;


		const vec2f center = (vtx[0] + vtx[1] + vtx[2] + vtx[3]) / 4.0f;
		vec2f p[4][4];
		f32   best_dist = 1e36f;
		u32	  best = 0;
		for (u32 i2=0; i2<4; i2++)
		{
			vec2f dir_center_to_vtx = vtx[i2] - center;
			dir_center_to_vtx.normalize();

			u32 i3 = i2+1;
			if (i3 == 4) i3=0;
			vec2f dir_vtx_to_vtx = vtx[i3] - vtx[i2];
			dir_vtx_to_vtx.normalize();
			if (fabsf(dir_vtx_to_vtx.x) > fabsf(dir_vtx_to_vtx.y))
				dir_vtx_to_vtx.y = 0;
			else
				dir_vtx_to_vtx.x = 0;
			dir_vtx_to_vtx.normalize();

			
			vec2f dir2(-dir_vtx_to_vtx.y, dir_vtx_to_vtx.x);
			dir2.normalize();

			//calcolo il quadratro ipotetico costruito sul vtx i2-esimo
			p[i2][0] = center + (dir_center_to_vtx * diag_half);
			p[i2][1] = p[i2][0] + dir_vtx_to_vtx * lato;
			p[i2][2] = p[i2][1] - dir2 * lato;
			p[i2][3] = p[i2][2] - dir_vtx_to_vtx * lato;

			//distanza media dei vtx del quadrato ipotetico rispetto ai vtx originali
			f32 avg_dist = 0;
			for (u32 i3=0; i3<4; i3++)
			{
				vec2f v = vtx[i3] - p[i2][i3];
				avg_dist += v.length2();
			}

			if (avg_dist < best_dist)
			{
				avg_dist = best_dist;
				best = i2;
			}

		}
	
			
		// in "best" ho l'indice del miglior quadrato ipotetico.
		// Sposto i vertici "un po'" in quella direzione
		u32 best_start_vtx = best;
		for (u32 i2=0; i2<4; i2++)
		{
			const f32 t = 0.03f;
			const vec2f v = vtx[best_start_vtx] + (p[best][i2] - vtx[best_start_vtx]) * t;

			u32 vtx_index = quadList(i).idx[best_start_vtx];


			if (!listOfBorderVtxIndex->exists(vtx_index))
			{
				//vtxList[vtx_index] = v;
				adj[vtx_index].n++;
				adj[vtx_index].sum += v;
			}
			

			best_start_vtx++;
			if (best_start_vtx >= 4)
				best_start_vtx = 0;
		}
	}

	for (u32 i=0; i<vtxList.getNElem(); i++)
	{
		if (0 != adj(i).n)
		{
			vec2f sum = adj(i).sum / (f32)adj(i).n;
			vtxList[i] = sum;
		}
	}
}

//***************************************
void ExaGenerator::quad_get_vertex (u32 quad_index, vec2f *out) const
{
	assert (quad_index < quadList.getNElem());
	out[0] = vtxList(quadList(quad_index).idx[0]).pos;
	out[1] = vtxList(quadList(quad_index).idx[1]).pos;
	out[2] = vtxList(quadList(quad_index).idx[2]).pos;
	out[3] = vtxList(quadList(quad_index).idx[3]).pos;
}

//***************************************
f32 ExaGenerator::quad_calc_area (const vec2f *vtx) const
{
	f32 a = 0.5f * (   vtx[0].x * vtx[1].y 
					- vtx[0].y * vtx[1].x
					+ vtx[1].x * vtx[2].y 
					- vtx[1].y * vtx[2].x 
					+ vtx[2].x * vtx[3].y 
					- vtx[2].y * vtx[3].x 
					+ vtx[3].x * vtx[0].y 
					- vtx[3].y * vtx[0].x);
	if (a < 0)
		return -a;
	return a;
}

//************************************************
gos::vec2f ExaGenerator::quad_calc_center (u32 quad_index) const
{
	assert (quad_index < quadList.getNElem());
	gos::vec2f ret = vtxList(quadList(quad_index).idx[0]).pos
		+ vtxList(quadList(quad_index).idx[1]).pos
		+ vtxList(quadList(quad_index).idx[2]).pos
		+ vtxList(quadList(quad_index).idx[3]).pos;
	ret /= 4.0f;
	return ret;
}

//************************************************
u32 ExaGenerator::get_quad_from_vtx (u32 vtx_index, u32 *out__quadList, u32 num_elem_in_quad_list) const
{
	assert (vtx_index < vtxList.getNElem());

	u32 ret = 0;
	for (u32 i = 0; i < quadList.getNElem(); i++)
	{
		for (u8 i2 = 0; i2 < 4; i2++)
		{
			if (vtx_index == quadList(i).idx[i2])
			{
				if (ret < num_elem_in_quad_list)
					out__quadList[ret++] = i;
			}
		}
	}

	//ordino il risulato in senso orario
	if (ret > 1)
	{
		assert (ret <=8);
		const vec2f center = vtxList(vtx_index).pos;
		
		f32 angle_list[8];
		for (u32 i = 0; i < ret; i++)
		{
			const vec2f  quad_center = quadCenterList(out__quadList[i]);
			const vec2f p = quad_center - center;
			angle_list[i] = ExaGenerator__calc_angle(p);
		}

		bool bEsci = false;
		u32 n = ret;
		while (bEsci == false)
		{
			bEsci = true;
			n--;
			
			for (u32 i = 0; i < n; i++)
			{
				if (angle_list[i] < angle_list[i + 1])
				{
					bEsci = false;
					GOSSWAP(angle_list[i], angle_list[i + 1]);
					GOSSWAP(out__quadList[i], out__quadList[i + 1]);
				}
			}
		}

	}

	return ret;
}

//************************************************
u16 ExaGenerator::priv_get_index_of_vtx_in_uscita_da (u32 quad_index, u16 vtx_index_A) const
{
	assert (quad_index < quadList.getNElem());

#ifdef _DEBUG
	bool debug_ok = false;
	for (u8 i = 0; i < 4; i++)
	{
		if (quadList(quad_index).idx[i] == vtx_index_A)
			debug_ok = true;
	}
	assert (debug_ok);
#endif

	u8 i=0;
	while (i < 3)
	{
		if (quadList(quad_index).idx[i] == vtx_index_A)
			return (i+1);
		i++;
	}

	return 0;
}

u16 ExaGenerator::get_index_of_vtx_in_uscita_da (u32 quad_index, u16 vtx_index_A) const
{
	const u16 ii = priv_get_index_of_vtx_in_uscita_da (quad_index, vtx_index_A);
	return quadList(quad_index).idx[ii];
}

//************************************************
u16 ExaGenerator::get_index_of_vtx_in_entrata_a (u32 quad_index, u16 vtx_index_A) const
{
	const u16 ii = priv_get_index_of_vtx_in_uscita_da (quad_index, vtx_index_A);
	return quadList(quad_index).idx[(ii + 2) % 4];
}


//************************************************
void ExaGenerator::remap()
{
	//devo operare solo su vtxList e quadList
	// vtx[0] e' il centro, a seguire ci sono i 48 vtx del bordo e poi tutto il resto in ordine sparso
	const u32 num_vtx = vtxList.getNElem();
	VtxList		new_vtxList;
	FastArray<u32>	vtx_remap_list;

	new_vtxList.setup (gos::getScrapAllocator(), num_vtx);
	
	vtx_remap_list.setup (gos::getScrapAllocator(), num_vtx);
	for (u32 i=0; i<num_vtx; i++)
		vtx_remap_list[i] = i;


	//vtx 0 e' sempre il cento
	u32 ct_vtx = 0;
	new_vtxList[ct_vtx++] = vtxList(0);
	
	//Voglio che i vtx da 1 a 48 (inclusi) siano i vtx del bordo, e li voglio in ordine
	//in modo che il vtx 1 sia il primo a dx e gli altri siano i vtx in senso antiorario (in ordine corretto)
	//Aggiungo i primi 24 vtx di bordo (perche' sono gia' in ordine corretto)
	//e li metto in posizione 1, 3, 5, 7 ...
	{
		vec2f primi24[24];

		u32 n_border_vtx = 0;
		u32 i=1;
		for (; i<num_vtx; i++)
		{
			if (!vtxList(i).isBorderVtx)
				continue;

			const u32 n = 1 + n_border_vtx*2;
			new_vtxList[n] = vtxList(i);
			vtx_remap_list[i] = n;

			assert (n_border_vtx < 24);
			primi24[n_border_vtx] = vtxList(i).pos;

			n_border_vtx++;
			if (24 == n_border_vtx)
				break;
		}
		assert (n_border_vtx == 24);
		i++;

		//aggiungo gli altri border vtx in posizione 2, 4, 6..
		n_border_vtx = 0;
		for (; i<num_vtx; i++)
		{
			if (!vtxList(i).isBorderVtx)
				continue;

			for (u32 t=0; t<24;t++)
			{
				const vec2f p1 = primi24[t];
				vec2f p2;
				if (t==23)
					p2 = primi24[0];
				else
					p2 = primi24[t+1];

				const vec2f p = p1 + (p2 - p1) * 0.5f;
				static constexpr f32 MAX_DIST = 0.1f * 0.1f;
				const f32 d = math::distance2(vtxList(i).pos, p);
				if (d < MAX_DIST)
				{
					const u32 n = 2 + 2*t;
					new_vtxList[n] = vtxList(i);
					vtx_remap_list[i] = n;
				}
			}
		}		
	}

	//tutti gli altri
	ct_vtx = new_vtxList.getNElem();
	for (u32 i=1; i<num_vtx; i++)
	{
		if (vtxList(i).isBorderVtx)
			continue;

		new_vtxList[ct_vtx] = vtxList(i);
		vtx_remap_list[i] = ct_vtx;
		ct_vtx++;
	}
	assert (ct_vtx == num_vtx);



	//remappo i quad
	const u32 num_quad = quadList.getNElem();
	for (u32 i=0; i<num_quad; i++)
	{
		for (u8 t=0; t<4; t++)
		{
			const u16 ii = quadList[i].idx[t];
			quadList[i].idx[t] = vtx_remap_list[ii];
		}
	}


	//copio i nuovi vtx
	vtxList.reset();
	for (u32 i=0; i<num_vtx; i++)
	{
		vtxList[i] = new_vtxList[i];
	}
}