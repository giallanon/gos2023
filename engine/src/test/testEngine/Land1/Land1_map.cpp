#include "Land1_map.h"
#include "gos.h"
#include "../PerlinNoise.hpp"
#include "gosGeomUtils.h"
#include "gosRandom.h"

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
Land1::Exa* Map::priv_exa_alloc (Land1::ExaGenerator &exagen, const vec3f &world_coord, gos::Random *rnd)
{
	exagen.build (exacc.get_exa_world_radius(), vec2f(world_coord.x, world_coord.z), rnd);
	
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
	FastHashMap<u64, u32> edge_vtx_hashmap;
	edge_vtx_hashmap.setup (gos::getScrapAllocator(), num_vtx_originali * num_vtx_originali);

#define MAKE_KEY(a,b,c,d)\
			assert(a<=0xFFFF); assert(b<=0xFFFF); assert(c<=0xFFFF); assert(d<=0xFFFF);\
			key = 0;\
			if (a <= b)	key |= ( ((u64)a << 48) | ((u64)b << 32) );\
			else		key |= ( ((u64)b << 48) | ((u64)a << 32) );\
			if (c <= d)	key |= ( ((u64)c << 16) | (u64)d );\
			else		key |= ( ((u64)d << 16) | (u64)c );\


	u32 key;
	FastHashMap<u64, u32>::Position pos;

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
		memset (vtxInfoList[iVtx].connected_vtx, 0xFF, sizeof(vtxInfoList[iVtx].connected_vtx));

		//il primo vtx e' sempre il centro
		vtxInfoList[iVtx].idx_list[num_idx++] = (u16)iVtx;

		//elenco dei quad con sharano il vtx i-esimo
		u32 adj_quad_list[16];
		const u32 nquad = exagen.get_quad_from_vtx (iVtx, adj_quad_list, 16);
		vtxInfoList[iVtx].num_quad = (u8)nquad;

		if (vtxInfoList[iVtx].is_border_vtx)
		{
			switch (nquad)
			{
			default:
				DBGBREAK;
				break;

			case 1:
				//siamo su un vtx del bordo
				//Dato che non ci sono le informazioni su tutte i quad adiacenti (perche' farebbero parte di un altro exa)
				//faccio qualche trucco
				{
					const u16 quad_index = adj_quad_list[0];
					
					const u32 QC_index = START_OF_QUAD_CENTER_VTX + quad_index;

					const u32 A_index = iVtx;
					const u32 B_index = exagen.get_index_of_vtx_in_uscita_da (quad_index, iVtx);
					const u32 C_index = exagen.get_index_of_vtx_in_entrata_a (quad_index, iVtx);

					const vec2f A = final_vtx_list(A_index);
					const vec2f B = final_vtx_list(B_index);
					const vec2f C = final_vtx_list(C_index);

					vec2f v_AB;
					u32 A_B_index;
					MAKE_KEY(A_index, B_index, A_index, B_index);
					if (!edge_vtx_hashmap.findWithPos(key, &A_B_index, &pos))
					{
						v_AB = A + (B - A) * 0.5f;
						A_B_index = final_vtx_list.getNElem();
						final_vtx_list.append (v_AB);
						edge_vtx_hashmap.insertInPosition (pos, A_B_index);
					}

					vec2f v_AC;
					u32 A_C_index;
					MAKE_KEY(A_index, C_index, A_index, C_index);
					if (!edge_vtx_hashmap.findWithPos(key, &A_C_index, &pos))
					{
						v_AC = A + (C - A) * 0.5f;
						A_C_index = final_vtx_list.getNElem();
						final_vtx_list.append (v_AC);
						edge_vtx_hashmap.insertInPosition (pos, A_C_index);
					}


					vtxInfoList[iVtx].idx_list[num_idx++] = (u16)A_B_index;
					vtxInfoList[iVtx].idx_list[num_idx++] = (u16)QC_index;
					vtxInfoList[iVtx].idx_list[num_idx++] = (u16)A_C_index;
				}
				break;

			case 2:
				//siamo su un vtx del bordo
				//Dato che non ci sono le informazioni su tutte i quad adiacenti (perche' farebbero parte di un altro exa)
				//faccio qualche trucco
				{
					u16 quad_index = adj_quad_list[0];
					u16 quad_prev_index = adj_quad_list[1];
					
					const u32 A_index = iVtx;
					u32 B_index = exagen.get_index_of_vtx_in_uscita_da (quad_index, iVtx);
					u32 C_index = exagen.get_index_of_vtx_in_uscita_da (quad_prev_index, iVtx);
					u32 D_index = exagen.get_index_of_vtx_in_entrata_a (quad_prev_index, iVtx);

					if (B_index == D_index)
					{
						quad_index = adj_quad_list[1];
						quad_prev_index = adj_quad_list[0];
						B_index = exagen.get_index_of_vtx_in_uscita_da (quad_index, iVtx);
						C_index = exagen.get_index_of_vtx_in_uscita_da (quad_prev_index, iVtx);
						D_index = exagen.get_index_of_vtx_in_entrata_a (quad_prev_index, iVtx);
					}
					

					const u32 QC_index = START_OF_QUAD_CENTER_VTX + quad_index;
					const u32 QC_prev_index = START_OF_QUAD_CENTER_VTX + quad_prev_index;
					const vec2f A = final_vtx_list(A_index);
					const vec2f B = final_vtx_list(B_index);
					const vec2f C = final_vtx_list(C_index);
					const vec2f D = final_vtx_list(D_index);
					const vec2f QC = final_vtx_list(QC_index);
					const vec2f QC_prev = final_vtx_list(QC_prev_index);

					vec2f v_AB;
					u32 A_B_index;
					MAKE_KEY(A_index, B_index, A_index, B_index);
					if (!edge_vtx_hashmap.findWithPos(key, &A_B_index, &pos))
					{
						v_AB = A + (B - A) * 0.5f;
						A_B_index = final_vtx_list.getNElem();
						final_vtx_list.append (v_AB);
						edge_vtx_hashmap.insertInPosition (pos, A_B_index);
					}

					vec2f v_AD;
					u32 A_D_index;
					MAKE_KEY(A_index, D_index, A_index, D_index);
					if (!edge_vtx_hashmap.findWithPos(key, &A_D_index, &pos))
					{
						v_AD = A + (D - A) * 0.5f;
						A_D_index = final_vtx_list.getNElem();
						final_vtx_list.append (v_AD);
						edge_vtx_hashmap.insertInPosition (pos, A_D_index);
					}

					vec2f v_QC_QCprev;
					u32 QC_QCprev_index;
					MAKE_KEY(A_index, C_index, QC_index, QC_prev_index);
					if (!edge_vtx_hashmap.findWithPos(key, &QC_QCprev_index, &pos))
					{
		#ifdef _DEBUG
						assert (geom::line2D__intersect (A, C, QC, QC_prev, &v_QC_QCprev));
		#else
						geom::line2D__intersect (A, C, QC, QC_prev, &v_QC_QCprev);
		#endif
						QC_QCprev_index = final_vtx_list.getNElem();
						final_vtx_list.append (v_QC_QCprev);
						edge_vtx_hashmap.insertInPosition (pos, QC_QCprev_index);
					}				

					vtxInfoList[iVtx].idx_list[num_idx++] = (u16)A_B_index;
					vtxInfoList[iVtx].idx_list[num_idx++] = (u16)QC_index;
					vtxInfoList[iVtx].idx_list[num_idx++] = (u16)QC_QCprev_index;
					vtxInfoList[iVtx].idx_list[num_idx++] = (u16)QC_prev_index;
					vtxInfoList[iVtx].idx_list[num_idx++] = (u16)A_D_index;
				}
				break;

			case 3:
				//siamo su un vtx del bordo
				//Dato che non ci sono le informazioni su tutte i quad adiacenti (perche' farebbero parte di un altro exa)
				//faccio qualche trucco
				{
					u16 quad_index = adj_quad_list[0];
					u16 quad_next_index = adj_quad_list[1];
					u16 quad_prev_index = adj_quad_list[2];
					
					const u32 A_index = iVtx;
					u32 B_index = exagen.get_index_of_vtx_in_uscita_da (quad_index, iVtx);
					u32 C_index = exagen.get_index_of_vtx_in_uscita_da (quad_next_index, iVtx);
					u32 D_index = exagen.get_index_of_vtx_in_uscita_da (quad_prev_index, iVtx);
					u32 E_index = exagen.get_index_of_vtx_in_entrata_a (quad_prev_index, iVtx);

					if (B_index == E_index)
					{
						quad_index = adj_quad_list[1];
						quad_next_index = adj_quad_list[2];
						quad_prev_index = adj_quad_list[0];
					
						B_index = exagen.get_index_of_vtx_in_uscita_da (quad_index, iVtx);
						C_index = exagen.get_index_of_vtx_in_uscita_da (quad_next_index, iVtx);
						D_index = exagen.get_index_of_vtx_in_uscita_da (quad_prev_index, iVtx);
						E_index = exagen.get_index_of_vtx_in_entrata_a (quad_prev_index, iVtx);

						if (B_index == E_index)
						{
							quad_index = adj_quad_list[2];
							quad_next_index = adj_quad_list[0];
							quad_prev_index = adj_quad_list[1];
						
							B_index = exagen.get_index_of_vtx_in_uscita_da (quad_index, iVtx);
							C_index = exagen.get_index_of_vtx_in_uscita_da (quad_next_index, iVtx);
							D_index = exagen.get_index_of_vtx_in_uscita_da (quad_prev_index, iVtx);
							E_index = exagen.get_index_of_vtx_in_entrata_a (quad_prev_index, iVtx);
						}
					}

					assert (B_index != E_index);

					const u32 QC_index = START_OF_QUAD_CENTER_VTX + quad_index;
					const u32 QC_next_index = START_OF_QUAD_CENTER_VTX + quad_next_index;
					const u32 QC_prev_index = START_OF_QUAD_CENTER_VTX + quad_prev_index;

					const vec2f A = final_vtx_list(A_index);
					const vec2f B = final_vtx_list(B_index);
					const vec2f C = final_vtx_list(C_index);
					const vec2f D = final_vtx_list(D_index);
					const vec2f E = final_vtx_list(E_index);
					const vec2f QC = final_vtx_list(QC_index);
					const vec2f QC_next = final_vtx_list(QC_next_index);
					const vec2f QC_prev = final_vtx_list(QC_prev_index);

					vec2f v_AB;
					u32 A_B_index;
					MAKE_KEY(A_index, B_index, A_index, B_index);
					if (!edge_vtx_hashmap.findWithPos(key, &A_B_index, &pos))
					{
						v_AB = A + (B - A) * 0.5f;
						A_B_index = final_vtx_list.getNElem();
						final_vtx_list.append (v_AB);
						edge_vtx_hashmap.insertInPosition (pos, A_B_index);
					}

					vec2f v_AE;
					u32 A_E_index;
					MAKE_KEY(A_index, E_index, A_index, E_index);
					if (!edge_vtx_hashmap.findWithPos(key, &A_E_index, &pos))
					{
						v_AE = A + (E - A) * 0.5f;
						A_E_index = final_vtx_list.getNElem();
						final_vtx_list.append (v_AE);
						edge_vtx_hashmap.insertInPosition (pos, A_E_index);
					}

					vec2f v_QC_QCnext;
					u32 QC_QCnext_index;
					MAKE_KEY(A_index, C_index, QC_index, QC_next_index);
					if (!edge_vtx_hashmap.findWithPos(key, &QC_QCnext_index, &pos))
					{
		#ifdef _DEBUG
						assert (geom::line2D__intersect (A, C, QC, QC_next, &v_QC_QCnext));
		#else
						geom::line2D__intersect (A, C, QC, QC_next, &v_QC_QCnext);
		#endif
						QC_QCnext_index = final_vtx_list.getNElem();
						final_vtx_list.append (v_QC_QCnext);
						edge_vtx_hashmap.insertInPosition (pos, QC_QCnext_index);
					}				

					vec2f v_QCnext_QCprev;
					u32 QCnext_QCprev_index;
					MAKE_KEY(A_index, D_index, QC_next_index, QC_prev_index);
					if (!edge_vtx_hashmap.findWithPos(key, &QCnext_QCprev_index, &pos))
					{
		#ifdef _DEBUG
						assert (geom::line2D__intersect (A, D, QC_next, QC_prev, &v_QCnext_QCprev));
		#else
						geom::line2D__intersect (A, D, QC_next, QC_prev, &v_QCnext_QCprev);
		#endif
						QCnext_QCprev_index = final_vtx_list.getNElem();
						final_vtx_list.append (v_QCnext_QCprev);
						edge_vtx_hashmap.insertInPosition (pos, QCnext_QCprev_index);
					}	

					vtxInfoList[iVtx].idx_list[num_idx++] = (u16)A_B_index;
					vtxInfoList[iVtx].idx_list[num_idx++] = (u16)QC_index;
					vtxInfoList[iVtx].idx_list[num_idx++] = (u16)QC_QCnext_index;
					vtxInfoList[iVtx].idx_list[num_idx++] = (u16)QC_next_index;
					vtxInfoList[iVtx].idx_list[num_idx++] = (u16)QCnext_QCprev_index;
					vtxInfoList[iVtx].idx_list[num_idx++] = (u16)QC_prev_index;
					vtxInfoList[iVtx].idx_list[num_idx++] = (u16)A_E_index;
				}
				break;
			}
		}
		else
		{
			//non e' un vtx del borde
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

				vtxInfoList[iVtx].connected_vtx[i2] = B_index;
			}
		}



		assert (num_idx <= 16);
		vtxInfoList[iVtx].num_idx = (u8)num_idx;

		//mesh type
		for (u32 i=0; i<nquad; i++)
		{
			vtxInfoList[iVtx].mesh_type[i] = Land1::eMeshType::boh;
		}
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
	exaList.insertIfNotExists (coord, exa);
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
void Map::debug_print_exa_stat (const Exa *exa) const
{
	u32 nq = 0;
	for (u32 i=0; i<exa->num_vtx_originali; i++)
	{
		nq += exa->vtxInfoList[i].num_quad;
	}


	logger::log ("exa stat => vtx=%d, num_quad=%d\n", exa->num_vtx_tot, nq);
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
	gos::Random rnd;
	rnd.seed (12345);

	Exa *exa = priv_exa_alloc ( exagen, exacc.exa_coord_to_world (examap::Coord(0,0)), &rnd );
	priv_exa_add_to_map (examap::Coord(0,0), exa);
	debug_print_exa_stat(exa);

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
				exa = priv_exa_alloc ( exagen, exacc.exa_coord_to_world (coordList[i]), &rnd );
				priv_exa_add_to_map (coordList[i], exa);
				debug_print_exa_stat(exa);
			}
		}
	}

	//genero delle height
	{
		exaList.forEach ([exa_radius_world, &rnd] (examap::Coord coord, Exa *exa) {
			for (u32 i = 0; i < exa->num_vtx_originali; i++)
			{
				const u32 r = rnd.getU32(100);
				if (r < 70)			exa->vtxInfoList[i].material_index = 1; 
				else if (r < 90)	exa->vtxInfoList[i].material_index = 2; 
				else				exa->vtxInfoList[i].material_index = 3; 
			}
			return true;
		});
	}

	map_recalc_meshType();
}

//*****************************************
void Map::map_recalc_meshType()
{
	const u32 N = exaList.getNElem();
	const auto list = exaList._queryList();

	for (u32 iExa=0; iExa<N; iExa++)
	{
		const HASHMAP::sElem *elem = &list->queryElem(iExa);
		const Exa *exa = elem->value;

		for (u32 iVtx=0; iVtx<exa->num_vtx_originali; iVtx++)
		{
			// if (108 == iVtx)
			// 	DBGBREAK;

			Exa::VtxInfo *vi = &exa->vtxInfoList[iVtx];

			if (vi->is_border_vtx)
				continue;

			//if (vi->material_index == 3)	vi->height = 1;
			for (u32 iQuad=0; iQuad<vi->num_quad; iQuad++)
			{
				u16 quad_indices[8];
				if (!exa->get_quad_indices (iVtx, iQuad, quad_indices))
					continue;
				
				const u8 adj_vtx_idx_0 = vi->connected_vtx[iQuad];
				const u8 adj_vtx_idx_1 = vi->connected_vtx[(iQuad+1) % vi->num_quad];
				if (0xFF == adj_vtx_idx_0)
					continue;

				const u8 material_adj_vtx_0 = exa->vtxInfoList[adj_vtx_idx_0].material_index;
				const u8 material_adj_vtx_1 = exa->vtxInfoList[adj_vtx_idx_1].material_index;

				u8 mask = 0;
				if (material_adj_vtx_0 != vi->material_index)	mask |= 0x01;
				if (material_adj_vtx_1 != vi->material_index)	mask |= 0x02;
				switch (mask)
				{
				default:	vi->mesh_type[iQuad] = Land1::eMeshType::full;	break;
				case 0x01:	vi->mesh_type[iQuad] = Land1::eMeshType::bordo_singolo_su;	break;
				case 0x02:	vi->mesh_type[iQuad] = Land1::eMeshType::bordo_singolo_dx;	break;
				case 0x03:	vi->mesh_type[iQuad] = Land1::eMeshType::angolo;	break;
				}
			}
		}
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
Land1::Exa* Map::priv_exa_get (const gos::examap::Coord &coord) const
{
	Exa *exa;
	if (exaList.find (coord, &exa))
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


