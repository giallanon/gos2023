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
	exagen.build (exacc.get_exa_world_radius(), world_coord);

	const u32 num_vtx = exagen.vtxList.getNElem();
	const u32 num_quad = exagen.quadList.getNElem();
	assert (num_vtx <= 0xff);
	assert (num_quad <= u16MAX);

	Exa *ret;

	u32 to_alloc = sizeof(Exa);
	
	//vtx list
	to_alloc = utils::calcNextMultipleOf8(to_alloc);
	const u32 offset_to_vtx_list = to_alloc;
	to_alloc += sizeof(vec2f) * num_vtx;

	//quad list
	to_alloc = utils::calcNextMultipleOf8(to_alloc);
	const u32 offset_to_quad_list = to_alloc;
	to_alloc += sizeof(Exa::Quad) * num_quad;

	//quadCenterList
	to_alloc = utils::calcNextMultipleOf8(to_alloc);
	const u32 offset_to_quadCenter_list = to_alloc;
	to_alloc += sizeof(vec2f) * num_quad;

	//vtxInfoList
	to_alloc = utils::calcNextMultipleOf8(to_alloc);
	const u32 offset_to_vtxInfo_list = to_alloc;
	to_alloc += sizeof(Exa::VtxInfo) * num_vtx;

	//alloco
	u8 *p = GOSALLOCT(u8*, localAllocator, to_alloc);
	ret = reinterpret_cast<Exa*>(p);
	memset (ret, 0, to_alloc);
	ret->num_vtx = (u16)num_vtx;
	ret->num_quad= (u16)num_quad;
	ret->vtxList = reinterpret_cast<vec2f*>( &p[offset_to_vtx_list] );
	ret->quadList = reinterpret_cast<Exa::Quad*>( &p[offset_to_quad_list] );
	ret->quadCenterList = reinterpret_cast<vec2f*>( &p[offset_to_quadCenter_list] );
	ret->vtxInfoList = reinterpret_cast<Exa::VtxInfo*>( &p[offset_to_vtxInfo_list] );


	//ora copio vtx e quadlist
	for (u32 i = 0; i < num_vtx; i++)
	{
		ret->vtxList[i].set (exagen.vtxList(i).x, exagen.vtxList(i).z);
		ret->vtxInfoList[i].material_index = 0;
		memset (ret->vtxInfoList[i].adjacent_quad_list, 0xff, sizeof(ret->vtxInfoList[i].adjacent_quad_list));
	}

	for (u32 i = 0; i < num_quad; i++)
	{
		ret->quadList[i].height = 0;
		ret->quadList[i].material_index = 0;
		ret->quadList[i].idx[0] = (u8)exagen.quadList(i).vtx_idx0;
		ret->quadList[i].idx[1] = (u8)exagen.quadList(i).vtx_idx1;
		ret->quadList[i].idx[2] = (u8)exagen.quadList(i).vtx_idx2;
		ret->quadList[i].idx[3] = (u8)exagen.quadList(i).vtx_idx3;
	}

	//calcolo il centro di ogni quad
	for (u32 i = 0; i < num_quad; i++)
	{
		ret->quadCenterList[i] = ret->vtxList[ ret->quadList[i].idx[0] ]
			+ ret->vtxList[ ret->quadList[i].idx[1] ]
			+ ret->vtxList[ ret->quadList[i].idx[2] ]
			+ ret->vtxList[ ret->quadList[i].idx[3] ];
		ret->quadCenterList[i] /= 4.0f;


		//ordino i vtx del quad in senso orario
		f32 angle_list[4];
		for (u32 i2=0; i2<4; i2++)
		{
			const vec2f v = ret->vtxList[ ret->quadList[i].idx[i2] ];
			const vec2f p = v - ret->quadCenterList[i];
			angle_list[i2] = atan2f (p.y, p.x);
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
					GOSSWAP(ret->quadList[i].idx[i2], ret->quadList[i].idx[i2 + 1]);
				}
			}
		}		

	}

	//per ogni vtx, vedo quali sono i quad che lo sharano
	for (u32 i = 0; i < ret->num_vtx; i++)
	{
		//recupero i quad che sharano il vtx i-esimo
		u32 quads[8];
		const u32 nquad = ret->get_quad_from_vtx (i, quads, 8);

		for (u32 ct = 0; ct < nquad; ct++)
		{
			ret->vtxInfoList[i].adjacent_quad_list[ct] = (u16)quads[ct];
		}
	}

	priv_exa_calc_v2 (ret);

	return ret;
}

//*****************************************
void Map::priv_exa_calc_v2 (Exa *exa) const
{
	memset (&exa->v2, 0, sizeof(Exa::V2));
	exa->v2.vtx_info = GOSALLOCT(Exa::VtxInfo2*, localAllocator, sizeof(Exa::VtxInfo2) * exa->num_vtx);

	//nell'array finale di tutti i vtx utili di questo exa, i primi N sono i vtx originali, a seguire
	//ci sono tutti i vtx dei centri dei quad (in ordine da quad 0 a quad N) e a seguire, in ordine sparso,
	//ci sono gli edge vertex
	FastArray<vec2f> final_vtx_list(gos::getScrapAllocator(), 2048);

	for (u32 i=0; i<exa->num_vtx; i++)
	{
		final_vtx_list.append( exa->vtxList[i] );
	}

	const u32 start_of_quad_center_vtx = final_vtx_list.getNElem();
	for (u32 i=0; i<exa->num_quad; i++)
	{
		final_vtx_list.append( exa->quadCenterList[i] );
	}

	FastHashMap<u32, u32> edge_vtx_hashmap;
	edge_vtx_hashmap.setup (gos::getScrapAllocator(), exa->num_vtx*exa->num_vtx);

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

	for (u32 i=0; i<exa->num_vtx; i++)
	{
		u32 num_idx = 0;
		memset (exa->v2.vtx_info[i].idx_list, 0xFF, sizeof(exa->v2.vtx_info[i].idx_list));
		exa->v2.vtx_info[i].height = 0;
		exa->v2.vtx_info[i].material_index = 1;
		exa->v2.vtx_info[i].idx_list[num_idx++] = (u16)i;


		//elenco dei quad con sharano il vtx i-esimo
		u32 adj_quad_list[16];
		const u32 nquad = exa->get_quad_from_vtx (i, adj_quad_list, 16);
		exa->v2.vtx_info[i].num_quad = nquad;

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
			const u32 A_index = i;
			const u32 B_index = exa->get_index_of_vtx_in_uscita_da (quad_index, i);
			const u32 C_index = exa->get_index_of_vtx_in_uscita_da (quad_next_index, i);


			const vec2f A = final_vtx_list(A_index);
			const vec2f B = final_vtx_list(B_index);
			const vec2f C = final_vtx_list(C_index);

			const u32 QC_index = start_of_quad_center_vtx + quad_index;
			assert (QC_index < 0xFFFF);
			const vec2f QC = final_vtx_list(QC_index);

			const u32 QC_prev_index = start_of_quad_center_vtx + quad_prev_index;
			assert (QC_prev_index < 0xFFFF);
			const vec2f QC_prev = final_vtx_list(QC_prev_index);

			const u32 QC_next_index = start_of_quad_center_vtx + quad_next_index;
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
				geom::utils::line2D__intersect (A, B, QC_prev, QC, &v_edge);
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
				geom::utils::line2D__intersect (A, C, QC, QC_next, &v_edge);
#endif
				v_edge2_index = final_vtx_list.getNElem();
				final_vtx_list.append (v_edge);
				edge_vtx_hashmap.insertInPosition (pos, v_edge2_index);
			}
			

			//mi segno tutti i vtx utili centrati su queste vtx primario
			assert (v_edge1_index < 0xFFFF);
			assert (QC_index < 0xFFFF);
			exa->v2.vtx_info[i].idx_list[num_idx++] = (u16)v_edge1_index;
			exa->v2.vtx_info[i].idx_list[num_idx++] = (u16)QC_index;
		}

		assert (num_idx <= 16);
	}

#undef MAKE_KEY

	exa->v2.num_tot_vtx = final_vtx_list.getNElem();
	exa->v2.vtx = GOSALLOCT(vec2f*, localAllocator, sizeof(vec2f) * exa->v2.num_tot_vtx);
	memcpy (exa->v2.vtx, final_vtx_list._queryPointer(), sizeof(vec2f) * exa->v2.num_tot_vtx);
}

//*****************************************
void Map::priv_exa_free (Exa *exa)
{
	GOSFREE(localAllocator, exa->v2.vtx);
	GOSFREE(localAllocator, exa->v2.vtx_info);
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
		siv::PerlinNoise perlin{ 1234 }; //gos::randomU32(u32MAX)};

		exaList.forEach ([&perlin, exa_radius_world] (u32 key, Exa *exa) {
			for (u32 i = 0; i < exa->num_quad; i++)
			{
				vec2f c = exa->utils__calc_quad_center(i);
				c /= (exa_radius_world);

				f32 h = (f32)perlin.octave2D_01(c.x, c.y, 2);
				//if (h > 0.8)	h = 4.0f;
				//else if (h > 0.5)	h = 2.0f;
				//else h = 0;

				h = 0;
				exa->quadList[i].height = h;
				exa->quadList[i].material_index = gos::randomU32(2);
			}

			for (u32 i = 0; i < exa->num_vtx; i++)
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