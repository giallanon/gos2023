#include "Land1_map.h"
#include "gos.h"
#include "../PerlinNoise.hpp"
#include "gosGeomUtils.h"

using namespace gos;
using namespace Land1;

//*****************************************
Map::Map()
{
	localAllocator = NULL;
}

//*****************************************
void Map::unsetup()
{
	if (NULL == localAllocator)
		return;

	priv_map_destroy();
}

//*****************************************
void Map::setup (gos::Allocator *allocatorIN)
{
	assert (NULL == localAllocator);
	localAllocator = allocatorIN;
	exaList.setup (localAllocator, 256);

}

//*****************************************
Land1::Exa* Map::priv_exa_alloc (Land1::ExaGenerator &exagen, const vec3f &world_coord)
{
	exagen.build (exacc.get_exa_world_radius(), vec2f(world_coord.x, world_coord.z));
	
	const u32 num_vtx_originali = exagen.vtxList.getNElem();
	const u32 num_quad = exagen.quadCenterList.getNElem();

	FastArray<vec2f> final_vtx_list (gos::getScrapAllocator(), 1024);
	Exa::VtxInfo *vtxInfoList = GOSALLOCT(Exa::VtxInfo*, gos::getScrapAllocator(), num_vtx_originali * sizeof(Exa::VtxInfo));
	memset (vtxInfoList, 0, num_vtx_originali * sizeof(Exa::VtxInfo));

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
	FastHashMap<u32, u32> edge_vtx_hashmap;
	edge_vtx_hashmap.setup (gos::getScrapAllocator(), num_vtx_originali * num_vtx_originali);

#define MAKE_KEY(a,b,c,d)\
			key = 0;\
			if (a <= b)	key |= ( ((u32)a << 24) | ((u32)b << 16) );\
			else		key |= ( ((u32)b << 24) | ((u32)a << 16) );\
			if (c <= d)	key |= ( ((u32)c << 8) | (u32)d );\
			else		key |= ( ((u32)d << 8) | (u32)c );\


	u32 key;
	u32 v_edge1_index;
	u32 v_edge2_index;
	vec2f v_edge;
	FastHashMap<u32, u32>::Position pos;

	for (u32 iVtx=0; iVtx<num_vtx_originali; iVtx++)
	{
		u32 num_idx = 0;
		vtxInfoList[iVtx].height = 0;
		vtxInfoList[iVtx].material_index = 1;
		vtxInfoList[iVtx].num_quad = 0;
		vtxInfoList[iVtx].num_idx = 0;
		vtxInfoList[iVtx].is_border_vtx = 0;
		if (exagen.is_a_border_vertex(iVtx))
			vtxInfoList[iVtx].is_border_vtx = 1;

		//il primo vtx e' sempre il centro
		vtxInfoList[iVtx].idx_list[num_idx++] = (u16)iVtx;

		//elenco dei quad con sharano il vtx i-esimo
		u32 adj_quad_list[16];
		const u32 nquad = exagen.get_quad_from_vtx (iVtx, adj_quad_list, 16);
		vtxInfoList[iVtx].num_quad = (u8)nquad;


		//TODO
		if (vtxInfoList[iVtx].is_border_vtx)
			continue;


		assert (nquad >= 3);
		for (u32 i = 0; i < nquad; i++)
		{
			const u32 quad_index = adj_quad_list[i];
			vtxInfoList[iVtx].adj_vtx_list[i] = exagen.get_index_of_vtx_in_entrata_a (quad_index, iVtx);
		}
		//se e' un vtx del bordo, per il momento skippo
		if (exagen.is_a_border_vertex(iVtx))
		{
			continue;
		}


		/*
		if (1 == nquad)
		{
			//siamo verosimilmente su un vtx del bordo
			//Dato che non ci sono le informazioni su tutte i quad adiacenti (perche' farebbero parte di un altro exa)
			//faccio qualche trucco
			const u16 quad_index = adj_quad_list[0];
			
			const u32 QC_index = start_of_quad_center_vtx + quad_index;

			const u32 A_index = i;
			const u32 B_index = exa->get_index_of_vtx_in_uscita_da (quad_index, i);
			const u32 C_index = exa->get_index_of_vtx_in_entrata_a (quad_index, i);

			const vec2f A = final_vtx_list(A_index);
			const vec2f B = final_vtx_list(B_index);
			const vec2f C = final_vtx_list(C_index);

			MAKE_KEY(A_index, B_index, A_index, B_index);
			if (!edge_vtx_hashmap.findWithPos(key, &v_edge1_index, &pos))
			{
				v_edge = A + (B - A) * 0.5f;
				v_edge1_index = final_vtx_list.getNElem();
				final_vtx_list.append (v_edge);
				edge_vtx_hashmap.insertInPosition (pos, v_edge1_index);
			}

			MAKE_KEY(A_index, C_index, A_index, C_index);
			if (!edge_vtx_hashmap.findWithPos(key, &v_edge2_index, &pos))
			{
				v_edge = A + (C - A) * 0.5f;
				v_edge2_index = final_vtx_list.getNElem();
				final_vtx_list.append (v_edge);
				edge_vtx_hashmap.insertInPosition (pos, v_edge2_index);
			}

			exa->v2.vtx_info[i].idx_list[num_idx++] = (u16)v_edge1_index;
			exa->v2.vtx_info[i].idx_list[num_idx++] = (u16)QC_index;
			exa->v2.vtx_info[i].idx_list[num_idx++] = (u16)v_edge2_index;

			continue;
		}


		if (2 == nquad)
		{
			//siamo verosimilmente su un vtx del bordo
			//Dato che non ci sono le informazioni su tutte i quad adiacenti (perche' farebbero parte di un altro exa)
			//faccio qualche trucco
			const u16 quad_index = adj_quad_list[0];
			const u16 quad_next_index = adj_quad_list[1];
			
			const u32 QC_index = start_of_quad_center_vtx + quad_index;
			const u32 QC_next_index = start_of_quad_center_vtx + quad_next_index;

			const u32 A_index = i;
			const u32 B_index = exa->get_index_of_vtx_in_uscita_da (quad_index, i);
			const u32 C_index = exa->get_index_of_vtx_in_entrata_a (quad_index, i);

			const vec2f A = final_vtx_list(A_index);
			const vec2f B = final_vtx_list(B_index);
			const vec2f C = final_vtx_list(C_index);
			continue;
		}
		*/

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
		}

		assert (num_idx <= 16);
		vtxInfoList[iVtx].num_idx = (u8)num_idx;
	}

#undef MAKE_KEY


	//alloco l'exa da ritornare
	Exa *ret =  GOSALLOCT(Exa*, localAllocator, sizeof(Exa));
	memset (ret, 0, sizeof(Exa));
	ret->num_vtx_originali = (u16)num_vtx_originali;
	ret->num_vtx_tot = (u16)final_vtx_list.getNElem();
	ret->vtxInfoList = GOSALLOCT(Exa::VtxInfo*, localAllocator, sizeof(Exa::VtxInfo) * num_vtx_originali);
	ret->vtxList = GOSALLOCT(vec2f*, localAllocator, sizeof(vec2f) * final_vtx_list.getNElem());

	memcpy (ret->vtxInfoList, vtxInfoList, sizeof(Exa::VtxInfo) * num_vtx_originali);
	memcpy (ret->vtxList, final_vtx_list._queryPointer(), sizeof(vec2f) * final_vtx_list.getNElem());

	GOSFREE(gos::getScrapAllocator(), vtxInfoList);
	return ret;

}

//*****************************************
void Map::priv_exa_free (Exa *exa)
{
	GOSFREE(localAllocator, exa->vtxInfoList);
	GOSFREE(localAllocator, exa->vtxList);
	GOSFREE(localAllocator, exa);
}

//*****************************************
void Map::priv_exa_add_to_map (const examap::Coord &coord, Exa *exa)
{
	const u32 key = coord.pack_coord_u32();
	exaList.insertIfNotExists (key, exa);
}

//*****************************************
void Map::priv_map_destroy()
{
	auto list = exaList._queryList();
	const u32 n= list->getNElem();
	for (u32 i=0; i<n; i++)
		priv_exa_free( list->queryElem(i).value);

	exaList.reset();
}

//*****************************************
void Map::map_create (f32 exa_radius_world, u32 map_radius)
{
	const vec3f WORLD_CENTER(0,0,0);
	
	priv_map_destroy();

	exacc.world__set_information (WORLD_CENTER, exa_radius_world);

	Land1::ExaGenerator exagen;
	exagen.setup (gos::getScrapAllocator());

	//genero exa in 0,0
	Exa *exa = priv_exa_alloc ( exagen, exacc.exa_coord_to_world (examap::Coord(0,0)) );
	priv_exa_add_to_map (examap::Coord(0,0), exa);

	//creo una serie di anelli attorno a 0,0
	{
		const u32 MAX_RADIUS = 128;
		const u32 MAX_NUM_COORD = MAX_RADIUS * 6;
		examap::Coord coordList[MAX_NUM_COORD];

		assert (map_radius <= MAX_RADIUS);


		for (u32 ring = 1; ring <= map_radius; ring++)
		{
			const u32 radius = ring;
			u32 n = examap::coord_ring (examap::Coord(0, 0), radius, coordList, MAX_NUM_COORD);
			for (u32 i = 0; i < n; i++)
			{
				exa = priv_exa_alloc ( exagen, exacc.exa_coord_to_world (coordList[i]) );
				priv_exa_add_to_map (coordList[i], exa);
			}
		}
	}


	//genero delle height
	{
		exaList.forEach ([exa_radius_world] (u32 key, Exa *exa) {
			for (u32 i = 0; i < exa->num_vtx_originali; i++)
			{
				const u32 r = gos::randomU32(100);
				if (r < 70)			exa->vtxInfoList[i].material_index = 1; 
				else if (r < 90)	exa->vtxInfoList[i].material_index = 2; 
				else				exa->vtxInfoList[i].material_index = 3; 
			}
			return true;
		});

		
	}
}

//*****************************************
bool Map::exa_query (const gos::examap::Coord &c, const Exa **out) const
{
	Exa *exa = priv_exa_get(c);
	if (NULL == exa)
		return false;
	*out = exa;
	return true;
}

//*****************************************
Land1::Exa* Map::priv_exa_get (const gos::examap::Coord &c) const
{
	const u32 key = c.pack_coord_u32();

	Exa *exa;
	if (exaList.find (key, &exa))
		return exa;
	return NULL;
}

//*****************************************
void Map::query_visible_exa (Result *out) const
{
	out->priv_reset();


	examap::Coord coord_center(0,0);
	Exa *exa = priv_exa_get(coord_center);
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
			Exa *exa = priv_exa_get(coord_array[i]);
			if (NULL != exa)
			{
				out->coordList.append (coord_array[i]);
				out->exaList.append(exa);
			}
		}
	}

	assert (out->coordList.getNElem() == out->exaList.getNElem());
}


