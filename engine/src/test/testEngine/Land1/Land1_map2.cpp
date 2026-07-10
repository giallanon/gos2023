#include "Land1_map2.h"
#include "gos.h"
#include "gosGeomUtils.h"


using namespace gos;
using namespace Land1;


//************************************* 
Map2::Map2()
{
	localAllocator = NULL;
}

//************************************* 
void Map2::unsetup()
{
	if (NULL == localAllocator)
		return;

	priv_destroy_map();
	localAllocator = NULL;
}

//************************************* 
void Map2::priv_destroy_map()
{
	if (NULL == localAllocator)
		return;

	auto list = exaList._queryList();
	const u32 n= list->getNElem();
	for (u32 i=0; i<n; i++)
		priv_exa_free( list->queryElem(i).value);

	exaList.reset();
}

//*****************************************
void Map2::priv_exa_free (Exa2 *exa)
{
	GOSFREE(localAllocator, exa->vtxInfoList);
	GOSFREE(localAllocator, exa->vtxList);
	GOSFREE(localAllocator, exa);
}

//************************************* 
void Map2::setup (gos::Allocator *allocator)
{
	localAllocator = allocator;
	exaList.setup (localAllocator, 256);
}

//*****************************************
Land1::Exa2* Map2::priv_exa_get (const gos::examap::Coord &c) const
{
	const u32 key = c.pack_coord_u32();

	Exa2 *exa;
	if (exaList.find (key, &exa))
		return exa;
	return NULL;
}

//*****************************************
void Map2::query_visible_exa (Result *out) const
{
	out->priv_reset();

	examap::Coord coord_center(0,0);
	Exa2 *exa = priv_exa_get(coord_center);
	if (NULL != exa)
	{
		out->coordList.append (coord_center);
		out->exaList.append(exa);
	}


	const u32 NUM_RINGS = 3;
	const u32 NUM_MAX_COORD = 128;
	examap::Coord coord_array[NUM_MAX_COORD];
	for (u32 radius = 1; radius <= NUM_RINGS; radius++)
	{
		const u32 n = examap::coord_ring (coord_center, radius, coord_array, NUM_MAX_COORD);
		for (u32 i = 0; i < n; i++)
		{
			Exa2 *exa = priv_exa_get(coord_array[i]);
			if (NULL != exa)
			{
				out->coordList.append (coord_array[i]);
				out->exaList.append(exa);
			}
		}
	}

	assert (out->coordList.getNElem() == out->exaList.getNElem());
}

//*****************************************
bool Map2::exa_query (const gos::examap::Coord &c, const Exa2 **out) const
{
	Exa2 *exa = priv_exa_get(c);
	if (NULL == exa)
		return false;
	*out = exa;
	return true;
}

//************************************* 
void Map2::map_create (f32 exa_radius_world, u32 random_seed)
{
	priv_destroy_map();

	const vec3f WORLD_CENTER(0,0,0);
	exacc.world__set_information (WORLD_CENTER, exa_radius_world);
	rnd.seed (random_seed);
}

//************************************* 
void Map2::priv_add_exa_to_map (Exa2 *exa)
{
	assert (false == exaList.exists(exa->coord.pack_coord_u32()));
	exaList.insertIfNotExists (exa->coord.pack_coord_u32(), exa);
}

//************************************* 
bool Map2::get_vertex_from_GVC (const GVC &gvc, gos::vec2f *out) const
{
	assert (NULL != out);
	const Exa2 *exa;
	if (!exa_query(gvc.get_exa_coord(), &exa))
		return false;

	const u16 vtx_index = gvc.get_vertex_idx();
	assert (vtx_index < exa->num_vtx_originali);
	*out = exa->vtxList[vtx_index];
	return true;
}

//************************************* 
bool Map2::priv_find_adj_exa_with_shared_vtx (const GVC gvc, GVC *out) const
{
	vec2f v;
	if (!get_vertex_from_GVC(gvc, &v))
	{
		DBGBREAK;
		return false;
	}

	return priv_find_adj_exa_with_shared_vtx (gvc.get_exa_coord(), v, out);
}

//************************************* 
bool Map2::priv_find_adj_exa_with_shared_vtx (gos::examap::Coord exa_cood, vec2f v, GVC *out) const
{
	examap::Coord ring_coord[16];
	const u32 n = examap::coord_ring (exa_cood, 1, ring_coord, 16);
	for (u32 i = 0; i < n; i++)
	{
		const Exa2 *e;
		if (exa_query(ring_coord[i], &e))
		{
			for (u32 i2 = 0; i2 < e->num_vtx_originali; i2++)
			{
				vec2f vv = v - e->vtxList[i2];
				if (fabsf(vv.x) < 0.001f && fabsf(vv.y) < 0.001f)
				{
					out->set (ring_coord[i], i2);
					return true;
				}
			}
		}
	}
	return false;
	
}

//************************************* 
void Map2::exa__add (const gos::examap::Coord coordIN)
{
	if (exaList.exists (coordIN.pack_coord_u32()))
	{
		DBGBREAK;
		return;
	}

	//genero un exa
	ExaGenerator exagen;
	exagen.setup (gos::getScrapAllocator());
	{
		vec3f world_exa_center = exa_coord_to_world(coordIN);
		exagen.build (exacc.get_exa_world_radius(), vec2f(world_exa_center.x, world_exa_center.z), &rnd);
	}
	const u32 num_vtx_originali = exagen.vtxList.getNElem();
	const u32 num_quad = exagen.quadCenterList.getNElem();



	FastArray<vec2f> final_vtx_list (gos::getScrapAllocator(), 1024);
	Exa2::VtxInfo *vtxInfoList = GOSALLOCT(Exa2::VtxInfo*, gos::getScrapAllocator(), num_vtx_originali * sizeof(Exa2::VtxInfo));
	memset (vtxInfoList, 0, num_vtx_originali * sizeof(Exa2::VtxInfo));

	//i primi n vtx di vtxList sono i vtx originali
	for (u32 i = 0; i < num_vtx_originali; i++)
	{
		final_vtx_list[i] = exagen.vtxList[i].pos;
	}

	//a seguire ci sono i quad-center
	const u32 START_OF_QUAD_CENTER_VTX = num_vtx_originali;
	for (u32 i = 0; i < num_quad; i++)
	{
		final_vtx_list.append (exagen.quadCenterList[i]);
	}


	//calcolo gli edge-vtx
	u32 key;
	FastHashMap<u64, u32>::Position pos;
	FastHashMap<u64, u32> edge_vtx_hashmap;
	edge_vtx_hashmap.setup (gos::getScrapAllocator(), num_vtx_originali * num_vtx_originali);

#define MAKE_KEY(a,b,c,d)\
			assert(a<=0xFFFF); assert(b<=0xFFFF); assert(c<=0xFFFF); assert(d<=0xFFFF);\
			key = 0;\
			if (a <= b)	key |= ( ((u64)a << 48) | ((u64)b << 32) );\
			else		key |= ( ((u64)b << 48) | ((u64)a << 32) );\
			if (c <= d)	key |= ( ((u64)c << 16) | (u64)d );\
			else		key |= ( ((u64)d << 16) | (u64)c );\



	for (u32 iVtx = 0; iVtx < num_vtx_originali; iVtx++)
	{
		u32 num_idx = 0;
		vtxInfoList[iVtx].height = 0;
		vtxInfoList[iVtx].material_index = 1;
		vtxInfoList[iVtx].num_quad = 0;
		vtxInfoList[iVtx].num_idx = 0;
		vtxInfoList[iVtx].is_border_vtx = 0;
		if (exagen.is_a_border_vertex(iVtx))
			vtxInfoList[iVtx].is_border_vtx = 1;
		for (u32 i = 0; i < 8; i++)
			vtxInfoList[iVtx].connected_vtx[i].set_invalid();

		//il primo vtx e' sempre il centro
		vtxInfoList[iVtx].idx_list[num_idx++] = (u16)iVtx;

		//elenco dei quad con sharano il vtx i-esimo
		u32 adj_quad_list[16];
		const u32 nquad = exagen.get_quad_from_vtx (iVtx, adj_quad_list, 16);
		vtxInfoList[iVtx].num_quad = (u8)nquad;


		u8 num_connected_vtx = 0;
		if (vtxInfoList[iVtx].is_border_vtx)
		{
			vtxInfoList[iVtx].num_quad = 0;

			//vertici connessi al mio interno
			for (u32 i2 = 0; i2 < nquad; i2++)
			{
				const u32 quad_index = adj_quad_list[i2];
				const u32 B_index = exagen.get_index_of_vtx_in_uscita_da (quad_index, iVtx);
				vtxInfoList[iVtx].connected_vtx[num_connected_vtx++].set (coordIN, B_index);
			}

			//cerco, se esiste, un exa adiacente a this che shara lo stesso vtx di frontiera
			GVC gvcOUT;
			if (priv_find_adj_exa_with_shared_vtx (coordIN, final_vtx_list[iVtx], &gvcOUT))
			{
				//gvcOUT e' lo stesso vertice di frontiera ma su un exa adiacente a me
				Exa2 *exaADJ = priv_exa_get (gvcOUT.get_exa_coord());
				assert (NULL != exaADJ);
				const Exa2::VtxInfo *viADJ = &exaADJ->vtxInfoList[gvcOUT.get_vertex_idx()];
				for (u32 i2 = 0; i2 < 8; i2++)
				{
					if (!viADJ->connected_vtx[i2].is_valid())
						break;
					vtxInfoList[iVtx].connected_vtx[num_connected_vtx++] = viADJ->connected_vtx[i2];
				}
				

			}




			
		}
		else
		{
			//non e' un vtx del border
			assert (nquad >= 3);
			for (u32 i2=0; i2<nquad; i2++)
			{
				const u32 quad_index = adj_quad_list[i2];

				u32 quad_prev_index;
				if (i2 == 0)
					quad_prev_index = adj_quad_list[nquad-1];
				else
					quad_prev_index = adj_quad_list[i2-1];

				u32 quad_next_index;
				if (i2 == nquad-1)
					quad_next_index = adj_quad_list[0];
				else
					quad_next_index = adj_quad_list[i2+1];

				//assumo che il vtx 0 del quad sia l'estremo B dell'edge che parte dal vtx in esame (A)
				const u32 A_index = iVtx;
				const u32 B_index = exagen.get_index_of_vtx_in_uscita_da (quad_index, iVtx);
				const u32 C_index = exagen.get_index_of_vtx_in_uscita_da (quad_next_index, iVtx);

				const vec2f A = final_vtx_list(A_index);
				const vec2f B = final_vtx_list(B_index);
				const vec2f C = final_vtx_list(C_index);

				const u32 QC_index = START_OF_QUAD_CENTER_VTX + quad_index;
				assert (QC_index < 0xFFFF);
				const vec2f QC = final_vtx_list(QC_index);

				const u32 QC_prev_index = START_OF_QUAD_CENTER_VTX + quad_prev_index;
				assert (QC_prev_index < 0xFFFF);
				const vec2f QC_prev = final_vtx_list(QC_prev_index);

				const u32 QC_next_index = START_OF_QUAD_CENTER_VTX + quad_next_index;
				assert (QC_next_index < 0xFFFF);
				const vec2f QC_next = final_vtx_list(QC_next_index);



				//calcolo i 2 edge vertex
				//intersezione 1: (A,B) e (QC_prev,QC)
				u32 v_edge1_index;
				u32 v_edge2_index;
				vec2f v_edge;

				MAKE_KEY(A_index, B_index, QC_prev_index, QC_index);
				if (!edge_vtx_hashmap.findWithPos(key, &v_edge1_index, &pos))
				{
	#ifdef _DEBUG
					assert (geom::line2D__intersect (A, B, QC_prev, QC, &v_edge));
	#else
					geom::line2D__intersect (A, B, QC_prev, QC, &v_edge);
	#endif
					v_edge1_index = final_vtx_list.getNElem();
					final_vtx_list.append (v_edge);
					edge_vtx_hashmap.insertInPosition (pos, v_edge1_index);
				}
				
				//intersezione 2: (A,C) (QC, QC_next)
				MAKE_KEY(A_index, C_index, QC_index, QC_next_index);
				if (!edge_vtx_hashmap.findWithPos(key, &v_edge2_index, &pos))
				{
	#ifdef _DEBUG
					assert (geom::line2D__intersect (A, C, QC, QC_next, &v_edge));
	#else
					geom::line2D__intersect (A, C, QC, QC_next, &v_edge);
	#endif
					v_edge2_index = final_vtx_list.getNElem();
					final_vtx_list.append (v_edge);
					edge_vtx_hashmap.insertInPosition (pos, v_edge2_index);
				}
				
				//mi segno tutti i vtx utili centrati su queste vtx primario
				assert (v_edge1_index < 0xFFFF);
				assert (QC_index < 0xFFFF);
				vtxInfoList[iVtx].idx_list[num_idx++] = (u16)v_edge1_index;
				vtxInfoList[iVtx].idx_list[num_idx++] = (u16)QC_index;

				vtxInfoList[iVtx].connected_vtx[i2].set (coordIN, B_index);
				assert (vtxInfoList[iVtx].connected_vtx[i2].get_exa_coord() == coordIN);
				assert (vtxInfoList[iVtx].connected_vtx[i2].get_vertex_idx() == B_index);
			}
		}

	}

#undef MAKE_KEY


	//alloco l'exa da ritornare
	Exa2 *ret =  GOSALLOCT(Exa2*, localAllocator, sizeof(Exa2));
	memset (ret, 0, sizeof(Exa2));
	ret->coord = coordIN;
	ret->num_vtx_originali = (u16)num_vtx_originali;
	ret->num_vtx_tot = (u16)final_vtx_list.getNElem();
	ret->vtxInfoList = GOSALLOCT(Exa2::VtxInfo*, localAllocator, sizeof(Exa2::VtxInfo) * num_vtx_originali);
	ret->vtxList = GOSALLOCT(vec2f*, localAllocator, sizeof(vec2f) * final_vtx_list.getNElem());

	memcpy (ret->vtxInfoList, vtxInfoList, sizeof(Exa2::VtxInfo) * num_vtx_originali);
	memcpy (ret->vtxList, final_vtx_list._queryPointer(), sizeof(vec2f) * final_vtx_list.getNElem());

	GOSFREE(gos::getScrapAllocator(), vtxInfoList);


	//addo alla mappa
	priv_add_exa_to_map (ret);

}

