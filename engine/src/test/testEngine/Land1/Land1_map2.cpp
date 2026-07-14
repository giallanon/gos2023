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
	nodemap.reset();
	examap.reset();
}

//************************************* 
void Map2::setup (gos::Allocator *allocator)
{
	localAllocator = allocator;
	nodemap.setup (localAllocator, 1024);
	examap.setup (localAllocator, 1024);
}

//************************************* 
bool Map2::priv_vtxmap__add_vtx (const GVC gvc, const Node &vtxIN)
{
	return nodemap.insertIfNotExists (gvc, vtxIN);
}

//************************************* 
bool Map2::priv_vtxmap__get_vtx (const GVC gvc, Node *out) const
{
	return nodemap.find (gvc, out);
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
void Map2::exa__add_with_radius (const gos::examap::Coord center_coord, u32 map_radius)
{
	exa__add (center_coord);

	const u32 MAX_RADIUS = 128;
	const u32 MAX_NUM_COORD = MAX_RADIUS * 6;
	examap::Coord coordList[MAX_NUM_COORD];

	assert (map_radius <= MAX_RADIUS);

	for (u32 ring = 1; ring <= map_radius; ring++)
	{
		const u32 radius = ring;
		u32 n = examap::coord_ring (center_coord, radius, coordList, MAX_NUM_COORD);
		for (u32 i = 0; i < n; i++)
		{
			exa__add (coordList[i]);
		}
	}
}

//************************************* 
void Map2::exa__add (const gos::examap::Coord coordIN)
{
	if (examap.exists (coordIN))
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


	//Creo un elenco di vtx dell'exa e dtermino GVC per ciascuno
	FastArray<Node> exavtx (gos::getScrapAllocator(), num_vtx_originali);
	for (u32 iVtx=0; iVtx<num_vtx_originali; iVtx++)
	{
		const ExaGenerator::VtxInfo *vi = &exagen.vtxList[iVtx];

		//creo il vtx
		Node vv;
		vv.pos = vi->pos;
		vv.material_index = 0;
		vv.num_adj_vtx = 0;
		vv.height = 0;

		//determino il GVC
		GVC gvc;		
		if (vi->isBorderVtx)
		{
			assert (iVtx >=1 && iVtx <= 48);
			if ( (iVtx >= 1 && iVtx<=16) || (iVtx >=42 && iVtx <= 48) )
			{
				//questo vtx del bordo e' mio
				gvc.set (coordIN, iVtx);
			}
			else
			{
				const i32 xx = coordIN.x;
				const i32 zz = coordIN.z;
				//il vtx appartiene ad un altro exa
				if (iVtx ==17)
					gvc.set (examap::Coord(xx-1, zz+1), 1);
				else if (iVtx >=18 && iVtx <= 24)
					gvc.set (examap::Coord(xx-1, zz+1), 42 + 24 - iVtx);
				else if (iVtx >=25 && iVtx <= 33)
					gvc.set (examap::Coord(xx-1, zz), 1 + 33 - iVtx);
				else if (iVtx >=34 && iVtx <= 41)
					gvc.set (examap::Coord(xx, zz-1), 9 + 41 - iVtx);
				else
				{
					DBGBREAK;
				}
			}
		}
		else
		{
			assert (iVtx ==0 || iVtx > 48);
			gvc.set (coordIN, iVtx);
		}

		vv.coord = gvc;
		exavtx[iVtx] = vv;
	}

	//calcolo i vtx adiacenti ad ogni vtx dell'exa
	for (u32 iVtx=0; iVtx<num_vtx_originali; iVtx++)
	{
		//elenco dei quad con sharano il vtx i-esimo
		u32 adj_quad_list[16];
		const u32 nquad = exagen.get_quad_from_vtx (iVtx, adj_quad_list, 16);
		
		if (exagen.is_a_border_vertex(iVtx))
		{
			switch (nquad)
			{
			default:
				DBGBREAK;
				break;

			case 1: 
				{
					const u32 B_index = exagen.get_index_of_vtx_in_uscita_da (adj_quad_list[0], iVtx);
					const u32 C_index = exagen.get_index_of_vtx_in_entrata_a (adj_quad_list[0], iVtx);
					exavtx[iVtx].num_adj_vtx = 2;
					exavtx[iVtx].connected_vtx[0] = exavtx[B_index].coord;
					exavtx[iVtx].connected_vtx[1] = exavtx[C_index].coord;
				}
				break;
			
			case 2:
				{
					u32 B_index = exagen.get_index_of_vtx_in_uscita_da (adj_quad_list[0], iVtx);
					u32 C_index = exagen.get_index_of_vtx_in_uscita_da (adj_quad_list[1], iVtx);
					u32 D_index = exagen.get_index_of_vtx_in_entrata_a (adj_quad_list[1], iVtx);

					if (D_index == B_index || D_index == C_index)
					{
						B_index = exagen.get_index_of_vtx_in_uscita_da (adj_quad_list[0], iVtx);
						C_index = exagen.get_index_of_vtx_in_entrata_a (adj_quad_list[0], iVtx);
						D_index = exagen.get_index_of_vtx_in_uscita_da (adj_quad_list[1], iVtx);
					}

					assert (B_index != C_index);
					assert (B_index != D_index);
					assert (C_index != D_index);

					exavtx[iVtx].num_adj_vtx = 3;
					exavtx[iVtx].connected_vtx[0] = exavtx[B_index].coord;
					exavtx[iVtx].connected_vtx[1] = exavtx[C_index].coord;
					exavtx[iVtx].connected_vtx[2] = exavtx[D_index].coord;
				}
				break;

			case 3:
				{
					u32 B_index = exagen.get_index_of_vtx_in_uscita_da (adj_quad_list[0], iVtx);
					u32 C_index = exagen.get_index_of_vtx_in_uscita_da (adj_quad_list[1], iVtx);
					u32 D_index = exagen.get_index_of_vtx_in_uscita_da (adj_quad_list[2], iVtx);
					u32 E_index = exagen.get_index_of_vtx_in_entrata_a (adj_quad_list[2], iVtx);

					if (E_index == B_index || E_index == C_index || E_index == D_index)
					{
						B_index = exagen.get_index_of_vtx_in_uscita_da (adj_quad_list[0], iVtx);
						C_index = exagen.get_index_of_vtx_in_uscita_da (adj_quad_list[1], iVtx);
						D_index = exagen.get_index_of_vtx_in_entrata_a (adj_quad_list[1], iVtx);
						E_index = exagen.get_index_of_vtx_in_uscita_da (adj_quad_list[2], iVtx);

						if (D_index == B_index || D_index == C_index || D_index == E_index)
						{
							B_index = exagen.get_index_of_vtx_in_uscita_da (adj_quad_list[0], iVtx);
							C_index = exagen.get_index_of_vtx_in_entrata_a (adj_quad_list[0], iVtx);
							D_index = exagen.get_index_of_vtx_in_uscita_da (adj_quad_list[1], iVtx);
							E_index = exagen.get_index_of_vtx_in_uscita_da (adj_quad_list[2], iVtx);
	
							if (C_index == B_index || C_index == D_index || C_index == E_index)
								DBGBREAK;

						}
					}

					assert (B_index != C_index);
					assert (B_index != D_index);
					assert (B_index != E_index);
					assert (C_index != D_index);
					assert (C_index != E_index);
					assert (D_index != E_index);

					exavtx[iVtx].num_adj_vtx = 4;
					exavtx[iVtx].connected_vtx[0] = exavtx[B_index].coord;
					exavtx[iVtx].connected_vtx[1] = exavtx[C_index].coord;
					exavtx[iVtx].connected_vtx[2] = exavtx[D_index].coord;
					exavtx[iVtx].connected_vtx[3] = exavtx[E_index].coord;

				}
				break;
			}
		}
		else
		{
			assert (nquad >= 3);
			exavtx[iVtx].num_adj_vtx = (u8)nquad;
			for (u32 t = 0; t < nquad; t++)
			{
				const u32 quad_index = adj_quad_list[t];
				const u32 B_index = exagen.get_index_of_vtx_in_uscita_da (quad_index, iVtx);

				exavtx[iVtx].connected_vtx[t] = exavtx[B_index].coord;
			}
		}
	}	

	//addo i vtx alla mappa
	for (u32 iVtx=0; iVtx<num_vtx_originali; iVtx++)
	{		
		const GVC gvc = exavtx(iVtx).coord;

		if (priv_vtxmap__add_vtx (gvc, exavtx(iVtx)))
		{
			//il vtx e' nuovo, non esisteva in mappa
			//Lo aggiungo e basta, non ho altro da fare
		}
		else
		{
			//il vtx esisteva gia' in mappa, vuol dire che devo aggiornare l'elenco
			//delle sue adj integrandolo con quelle di <exavtx(iVtx)>
			Node vReal;
			priv_vtxmap__get_vtx (gvc, &vReal);

			const u32 n = exavtx(iVtx).num_adj_vtx;
			assert (n <= 7);
			for (u32 i=0; i<n; i++)
			{
				const GVC gvc_connected = exavtx(iVtx).connected_vtx[i];
				bool bFound = false;
				for (u32 t = 0; t < vReal.num_adj_vtx; t++)
				{
					if (vReal.connected_vtx[t] == gvc_connected)
					{
						bFound = true;
						break;
					}
				}
				if (!bFound)
				{
					const u32 ii = vReal.num_adj_vtx++;
					assert (ii<=7);
					vReal.connected_vtx[ii] = gvc_connected;
				}
			}

			//ordino i vtx adiacenti in senso orario
			{
				vec2f vvv[8];
				for (u32 i=0; i<vReal.num_adj_vtx; i++)
				{
					Node node;
					if (vReal.connected_vtx[i].get_exa_coord() == coordIN)
						vvv[i] = exavtx(vReal.connected_vtx[i].get_vertex_idx()).pos;
					else if (GVC_to_node (vReal.connected_vtx[i], &node))
					{
						vvv[i] = node.pos;
					}
					else
					{
						vvv[i].set (0,0);
					}
				}

				u32 ordered_index[8];
				geom::point2D_order_clockwise (vReal.pos, vvv, vReal.num_adj_vtx, ordered_index, sizeof(ordered_index));
				
				GVC gvc_temp[8];
				for (u32 i=0; i<vReal.num_adj_vtx; i++)
					gvc_temp[i] = vReal.connected_vtx[ ordered_index[i] ];
				for (u32 i=0; i<vReal.num_adj_vtx; i++)					
					vReal.connected_vtx[i] = gvc_temp[i];
			}

			//aggiorno il nodo
			nodemap.insertOrReplaceValue (gvc, vReal);
		}
	}

	//addo l'exa alla mappa di hex
	HexInfo hexinfo;
	hexinfo.coord = coordIN;
	hexinfo.num_vtx = num_vtx_originali;
	examap.insertIfNotExists (coordIN, hexinfo);
}

//************************************* 
void Map2::priv_node_to_vtx (const Node &node, Vtx *out) const
{
	out->pos = node.pos;
	out->material_index = node.material_index;
	out->num_adj_vtx = node.num_adj_vtx;
	out->height = node.height;
	out->coord = node.coord;
}
//************************************* 
void Map2::priv_node_to_vtx (const GVC gvc, Vtx *out) const
{
	Node node;
	if (nodemap.find (gvc, &node))
		priv_node_to_vtx (node, out);
	else
	{
		DBGBREAK;
		memset (out, 0, sizeof(Vtx));
	}
}

//************************************* 
bool Map2::world_coord_to_GVC  (const gos::vec3f &world_coord, GVC *out) const
{
	examap::Coord exa_coord = exacc.world_coord_to_exa (world_coord);
	
	HexInfo hex;
	if (!examap.find (exa_coord, &hex))
		return false;


	const vec2f p (world_coord.x, world_coord.z);
	f32 best_d = 1e36f;
	for (u32 iVtx = 0; iVtx < hex.num_vtx; iVtx++)
	{
		GVC gvc;
		gvc.set (exa_coord, iVtx);

		Node node;
		if (nodemap.find (gvc, &node))
		{
			const f32 d = math::distance2(node.pos, p);
			if (d < best_d)
			{
				best_d = d;
				(*out) = gvc;
			}			
		}
	}

	return true;
}

//************************************* 
bool Map2::GVC_to_world_coord  (const GVC gvc, gos::vec3f *out_world_coord) const
{
	Node node;
	if (!nodemap.find (gvc, &node))
		return false;
	out_world_coord->set (node.pos.x, 0, node.pos.y);
	return true;
}

//************************************* 
bool Map2::GVC_to_node (const GVC gvc, Node *out_node) const
{
	return nodemap.find (gvc, out_node);
}

//************************************* 
u32 Map2::get_exa_vtxList (const gos::examap::Coord &exa_coord, FastArray<Vtx> &outList, bool bClear_outList) const
{
	if (bClear_outList)
		outList.reset();
	if (!examap.exists(exa_coord))
		return 0;

	//metto in <outlist> l'elenco dei vtx di exa_coord
	//uso <map> per mappare GVC all'indice del vtx all'interno di <outlist>
	gos::FastHashMap<GVC, u32>	map (gos::getScrapAllocator(), 1024);
	nodemap.forEach ([&map, &outList, exa_coord, me=this](GVC gvc, const Node node) {
		if (gvc.get_exa_coord() == exa_coord)
		{
			Vtx vtx;
			me->priv_node_to_vtx (node, &vtx);
			
			const u32 ii = outList.getNElem ();
			outList.append(vtx);
			map.insertIfNotExists (gvc, ii);
		}
		return true;
	});

	const u32 ret = outList.getNElem();

	//sistemo le adiacenze
	const u32 n = outList.getNElem();
	for (u32 iVtx = 0; iVtx < n; iVtx++)
	{
		Vtx *v = &outList[iVtx];
		
		Node node;
		if (!nodemap.find (v->coord, &node))
			DBGBREAK;
		for (u32 t = 0; t < v->num_adj_vtx; t++)
		{
			const GVC gvc = node.connected_vtx[t];
			
			u32 index;
			if (!map.find (gvc, &index))
			{
				//stiamo puntando ad un vtx situato su un altro exa
				Vtx vtx;
				priv_node_to_vtx (gvc, &vtx);
				vtx.num_adj_vtx=0;

				index = outList.getNElem ();
				map.insertIfNotExists (gvc, index);
				outList.append(vtx);
			}
			
			assert (t<8);
			v->adj_vtx_list[t] = (u16)index;
		}
	}

	//calcolo i quad center
	// for (u32 iVtx = 0; iVtx < n; iVtx++)
	// {
	// 	Vtx *v = &outList[iVtx];
	// }

	return ret;
}
