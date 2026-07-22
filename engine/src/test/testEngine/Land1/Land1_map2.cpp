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

	examap.forEach ( [localAllocator=this->localAllocator](examap::Coord coord, ExaInfo exaInfo) {
		GOSFREE(localAllocator, exaInfo.node_list);
		return true;
	});
	examap.reset();
	temp_node_list.reset();
}

//************************************* 
void Map2::setup (gos::Allocator *allocator)
{
	localAllocator = allocator;
	examap.setup (localAllocator, 1024);
	temp_node_list.setup (localAllocator, 1024);
}

//************************************* 
bool Map2::priv_examap__update_node (const GVC gvc, const Node &nodeIN)
{
	ExaInfo *exaInfo = examap.get_pointer (gvc.get_exa_coord());
	if (NULL == exaInfo)
	{
		DBGBREAK;
		return false;
	}

	const u32 idx = gvc.get_vertex_idx();
	assert (idx < exaInfo->num_node);
	assert (exaInfo->node_list[idx].gvc == gvc);
	exaInfo->node_list[idx] = nodeIN;
	return true;
}

//************************************* 
bool Map2::priv_examap__get_node (const GVC gvc, Node *out) const
{
	const ExaInfo *exaInfo = examap.query_pointer (gvc.get_exa_coord());
	if (NULL == exaInfo)
		return false;

	const u32 idx = gvc.get_vertex_idx();
	assert (idx < exaInfo->num_node);
	assert (exaInfo->node_list[idx].gvc == gvc);
	*out = exaInfo->node_list[idx];
	return true;
}

//************************************* 
Map2::Node* Map2::priv_examap__get_nodePointer (const GVC gvc)
{
	const ExaInfo *exaInfo = examap.query_pointer (gvc.get_exa_coord());
	if (NULL == exaInfo)
		return NULL;

	const u32 idx = gvc.get_vertex_idx();
	assert (idx < exaInfo->num_node);
	assert (exaInfo->node_list[idx].gvc == gvc);
	return &exaInfo->node_list[idx];
}

//************************************* 
const Map2::Node* Map2::priv_examap__get_nodePointer (const GVC gvc) const
{
	const ExaInfo *exaInfo = examap.query_pointer (gvc.get_exa_coord());
	if (NULL == exaInfo)
		return NULL;

	const u32 idx = gvc.get_vertex_idx();
	assert (idx < exaInfo->num_node);
	assert (exaInfo->node_list[idx].gvc == gvc);
	return &exaInfo->node_list[idx];
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
	for (u32 iVtx = 0; iVtx < num_vtx_originali; iVtx++)
	{
		const ExaGenerator::VtxInfo *vi = &exagen.vtxList[iVtx];

		//creo il vtx
		Node vv;
		vv.pos = vi->pos;
		vv.material_index = 1 + rnd.getU32(2);
		vv.num_adj_vtx = 0;
		vv.height = 0;

		for (u32 i=0; i<6; i++)
			vv.mesh_type[i] = eMeshType::full;

		//determino il GVC
		GVC gvc;
		if (vi->isBorderVtx)
		{
			assert (iVtx >= 1 && iVtx <= 48);
			if ((iVtx >= 1 && iVtx <= 16) || (iVtx >= 42 && iVtx <= 48))
			{
				//questo vtx del bordo e' mio
				gvc.set (coordIN, iVtx);
			}
			else
			{
				const i32 xx = coordIN.x;
				const i32 zz = coordIN.z;
				//il vtx appartiene ad un altro exa
				if (iVtx == 17)
					gvc.set (examap::Coord(xx - 1, zz + 1), 1);
				else if (iVtx >= 18 && iVtx <= 24)
					gvc.set (examap::Coord(xx - 1, zz + 1), 42 + 24 - iVtx);
				else if (iVtx >= 25 && iVtx <= 33)
					gvc.set (examap::Coord(xx - 1, zz), 1 + 33 - iVtx);
				else if (iVtx >= 34 && iVtx <= 41)
					gvc.set (examap::Coord(xx, zz - 1), 9 + 41 - iVtx);
				else
				{
					DBGBREAK;
				}
			}
		}
		else
		{
			assert (iVtx == 0 || iVtx > 48);
			gvc.set (coordIN, iVtx);
		}

		vv.gvc = gvc;
		exavtx[iVtx] = vv;
	}

	//calcolo i vtx adiacenti ad ogni vtx dell'exa
	for (u32 iVtx = 0; iVtx < num_vtx_originali; iVtx++)
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
				exavtx[iVtx].connected_vtx[0] = exavtx[B_index].gvc;
				exavtx[iVtx].connected_vtx[1] = exavtx[C_index].gvc;
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
				exavtx[iVtx].connected_vtx[0] = exavtx[B_index].gvc;
				exavtx[iVtx].connected_vtx[1] = exavtx[C_index].gvc;
				exavtx[iVtx].connected_vtx[2] = exavtx[D_index].gvc;
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
				exavtx[iVtx].connected_vtx[0] = exavtx[B_index].gvc;
				exavtx[iVtx].connected_vtx[1] = exavtx[C_index].gvc;
				exavtx[iVtx].connected_vtx[2] = exavtx[D_index].gvc;
				exavtx[iVtx].connected_vtx[3] = exavtx[E_index].gvc;

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

				assert (t < 6);
				exavtx[iVtx].connected_vtx[t] = exavtx[B_index].gvc;
			}
		}
	}

	//a questo punto in exavtx[] ho l'elenco dei vtx utili per l'exa
	//Alcuni dei vtx del bordo non hanno la stessa Coord di questo exa perche' tecnicamente
	//fanno parte di exa adiacenti
	assert (exavtx.getNElem() == num_vtx_originali);

	//creo l'exaInfo e lo addo alla mappa
	ExaInfo exaInfo;
	exaInfo.reset();
	exaInfo.coord = coordIN;
	exaInfo.num_node = num_vtx_originali;
	exaInfo.node_list = GOSALLOCT(Node*, localAllocator, sizeof(Node) * exaInfo.num_node);
	{
		//nell'array dei nodi, ci saranno dei "buchi" in corrispondenza dei nodi
		//che non hanno un GVC la cui exa_coord sia == a coordIN
		//Lo faccio per questioni di praticita': in questo modo ho un array lineare il cui elemento i-esimo corrisponde al vtx
		//di indice i
		for (u32 iVtx = 0; iVtx < num_vtx_originali; iVtx++)
			exaInfo.node_list[iVtx].gvc.set_invalid();
		for (u32 iVtx = 0; iVtx < num_vtx_originali; iVtx++)
		{
			if (exavtx(iVtx).gvc.get_exa_coord() == coordIN)
			{
				const u16 idx = exavtx(iVtx).gvc.get_vertex_idx();
				assert (idx < exaInfo.num_node);
				exaInfo.node_list[idx] = exavtx(iVtx);
			}
		}

		examap.insertIfNotExists (coordIN, exaInfo);
	}


	//i nodi del bordo sono un po' speciali. Alcuni nodi del bordo infatti non appartengono
	//a questo exa ma potrebbero gia' essere stati creati come nodi temporanei da altri exa che 
	//condivino il bordo
	for (u32 iVtx = 0; iVtx < num_vtx_originali; iVtx++)
	{
		if (!exagen.is_a_border_vertex(iVtx))
			continue;
		
		const GVC gvc = exavtx(iVtx).gvc;
		if (exavtx(iVtx).gvc.get_exa_coord() == coordIN)
		{
			//questo nodo appartiene a questo exa
			//Verifico se e' stato gia' creato da altri nodi nel qual caso
			//faccio il merge del nodo temporaneo con il nodo reale e poi elimino il temporaneo
			const Node *vTempNode = temp_node_list.query_pointer(gvc);
			if (NULL != vTempNode)
			{
				priv_examap__merge_node_adj (priv_examap__get_nodePointer(gvc), vTempNode);
				temp_node_list.remove (gvc);
			}
		}
		else
		{
			//questo nodo NON appartiene a questo exa.
			//Se il nodo esiste gia' in mappa, faccio il merge altrimenti creo un nodo temporaneo
			Node *nodeSRC = priv_examap__get_nodePointer (gvc);
			if (NULL != nodeSRC)
			{
				priv_examap__merge_node_adj (nodeSRC, &exavtx(iVtx));
			}
			else
			{
				//il nodo non esiste in mappa, ma potrebbe esistere come nodo temporaneao
				nodeSRC = temp_node_list.get_pointer(gvc);
				if (NULL != nodeSRC)
				{
					priv_examap__merge_node_adj (nodeSRC, &exavtx(iVtx));
				}
				else
				{
					//il nodo e' davvero nuovo. Lo creo come nodo temporaneo
					temp_node_list.insertIfNotExists (gvc, exavtx(iVtx));
				}
			}
		}
	}


	//calcolo i quad center
	{
		const ExaInfo *ee = examap.query_pointer (coordIN);
		assert (NULL != ee);
		for (u32 iVtx = 0; iVtx < num_vtx_originali; iVtx++)
		{
			const GVC gvc = exavtx(iVtx).gvc;
			if (gvc.get_exa_coord() == coordIN)
				priv_node__update_quad_center (&ee->node_list[ gvc.get_vertex_idx()]);
			else
			{
				Node *node = priv_examap__get_nodePointer (gvc);
				if (NULL == node)
					node = temp_node_list.get_pointer (gvc);
				assert (NULL != node);
				priv_node__update_quad_center (node);
			}			
		}
	}
}


/************************************* 
 * fa il merge delle adiacenze di <nodeIN> con quelle di <temp_node> modificando <nodeIN>
 */
void Map2::priv_examap__merge_node_adj (Node *nodeIN, const Node *other_node)
{
	const u32 NN = other_node->num_adj_vtx;
	assert (NN <= 6);
	for (u32 i = 0; i < NN; i++)
	{
		const GVC gvc_connected = other_node->connected_vtx[i];
		
		bool bFound = false;
		for (u32 t = 0; t < nodeIN->num_adj_vtx; t++)
		{
			if (nodeIN->connected_vtx[t] == gvc_connected)
			{
				bFound = true;
				break;
			}
		}
		if (!bFound)
		{
			const u32 ii = nodeIN->num_adj_vtx++;
			assert (ii < 6);
			nodeIN->connected_vtx[ii] = gvc_connected;
		}
	}

	//ordino i vtx adiacenti in senso orario
	{
		vec2f vvv[8];
		for (u32 i = 0; i < nodeIN->num_adj_vtx; i++)
		{
			const Node *node2 = priv_examap__get_nodePointer (nodeIN->connected_vtx[i]);
			if (NULL == node2)
				node2 = temp_node_list.query_pointer (nodeIN->connected_vtx[i]);
			if (NULL != node2)
				vvv[i] = node2->pos;
			else
				vvv[i].set (0, 0);
		}

		u32 ordered_index[8];
		geom::point2D_order_clockwise (nodeIN->pos, vvv, nodeIN->num_adj_vtx, ordered_index, sizeof(ordered_index));

		GVC gvc_temp[8];
		for (u32 i = 0; i < nodeIN->num_adj_vtx; i++)
			gvc_temp[i] = nodeIN->connected_vtx[ordered_index[i]];
		for (u32 i = 0; i < nodeIN->num_adj_vtx; i++)
			nodeIN->connected_vtx[i] = gvc_temp[i];
	}
}

//************************************* 
void Map2::priv_node__update_quad_center (Node *nodeIN)
{
	if (nodeIN->num_adj_vtx < 3)
		return;

	for (u32 iQuad = 0; iQuad < nodeIN->num_adj_vtx; iQuad++)
	{
		const Node *node1 = priv_examap__get_nodePointer (nodeIN->connected_vtx[iQuad]);
		if (NULL == node1)
		{
			node1 = temp_node_list.query_pointer (nodeIN->connected_vtx[iQuad]);
			if (NULL == node1)
			{
				DBGBREAK;
				continue;
			}
		}

		u32 ii = iQuad+1;
		if (ii == nodeIN->num_adj_vtx)
			ii=0;
		const Node *node2 = priv_examap__get_nodePointer (nodeIN->connected_vtx[ii]);
		if (NULL == node2)
		{
			node2 = temp_node_list.query_pointer (nodeIN->connected_vtx[ii]);
			if (NULL == node2)
			{
				DBGBREAK;
				continue;
			}
		}

		//node3 e' il GVC che node1 e node2 hanno in comune (se escludo nodeIN.gvc)
		const Node *node3 = NULL;
		bool bFound = false;
		for (u32 i2 = 0; i2 < node1->num_adj_vtx; i2++)
		{
			if (node1->connected_vtx[i2] == nodeIN->gvc)
				continue;
			for (u32 i3 = 0; i3 < node2->num_adj_vtx; i3++)
			{
				if (node1->connected_vtx[i2] == node2->connected_vtx[i3])
				{
					bFound = true;
					node3 = priv_examap__get_nodePointer (node1->connected_vtx[i2]);
					if (NULL == node3)
					{
						node3 = temp_node_list.query_pointer (node1->connected_vtx[i2]);
						if (NULL == node3)
						{
							DBGBREAK;
							continue;
						}
					}
					break;
				}
			}

			if (bFound)
				break;
		}


		//ho tutti e 4 i nodi del quad, posso determinare il centro
		if (bFound)
		{
			nodeIN->other_vtx[iQuad] = node3->gvc;
			nodeIN->quad_center[iQuad] = (nodeIN->pos + node1->pos + node2->pos + node3->pos) * 0.25f;
		}
	}
}

//************************************* 
void Map2::priv_node_to_vtx (const Node *node, Vtx *out) const
{
	out->pos.set (node->pos.x, (f32)node->height * EXA_HEIGHT_MUL, node->pos.y);
	out->material_index = node->material_index;
	out->num_adj_vtx = node->num_adj_vtx;
	out->height = node->height;
	out->gvc = node->gvc;
}

//************************************* 
bool Map2::world_coord_to_GVC  (const gos::vec3f &world_coord, GVC *out) const
{
	examap::Coord exa_coord = exacc.world_coord_to_exa (world_coord);
	
	const ExaInfo *exaInfo = examap.query_pointer (exa_coord);
	if (NULL == exaInfo)
		return false;


	//const vec2f p (world_coord.x, world_coord.z);
	const vec3f p (world_coord.x, world_coord.y, world_coord.z);
	f32 best_d = 1e36f;
	for (u32 iVtx = 0; iVtx < exaInfo->num_node; iVtx++)
	{
		const Node *node = &exaInfo->node_list[iVtx];
		if (node->gvc.get_exa_coord() != exaInfo->coord)
			continue;
			
		const vec3f nodepos (node->pos.x, (f32)node->height * EXA_HEIGHT_MUL, node->pos.y);

		const f32 d = math::distance2(nodepos, p);
		if (d < best_d)
		{
			best_d = d;
			(*out) = node->gvc;
		}			
	}

	return true;
}

//************************************* 
bool Map2::world_ray_to_GVC  (const gos::vec3f &world_o, const gos::vec3f &world_dir, GVC *out) const
{
	//TODO
	DBGBREAK;
	return false;
}

//************************************* 
bool Map2::GVC_to_world_coord  (const GVC gvc, gos::vec3f *out_world_coord) const
{
	const Node *node;
	const ExaInfo *exaInfo = examap.query_pointer (gvc.get_exa_coord());
	if (NULL != exaInfo)
	{
		const u32 idx = gvc.get_vertex_idx();
		assert (idx < exaInfo->num_node);
		assert (exaInfo->node_list[idx].gvc == gvc);
		node = &exaInfo->node_list[idx];
	}
	else
	{
		node = temp_node_list.query_pointer (gvc);
	}

	if (NULL == node)
		return false;

	out_world_coord->set (node->pos.x, (f32)node->height * EXA_HEIGHT_MUL, node->pos.y);
	return true;
}

//************************************* 
bool Map2::GVC_to_node (const GVC gvc, Node *out_node) const
{
	const Node *node = priv_examap__get_nodePointer (gvc);
	if (NULL == node)
		return false;
	assert (node->gvc == gvc);
	*out_node = *node;
	return true;
}

//************************************* 
void Map2::set_node_material_index (const GVC gvc, u8 material_index)
{
	Node *node = priv_examap__get_nodePointer (gvc);
	if (NULL != node)
	{
		if (node->material_index != material_index)
			node->material_index = material_index;
	}
}

//************************************* 
void Map2::priv_do_set_node_height (const GVC gvc, Node *node, u16 height)
{
	if (node->height == height)
		return;

	node->height = height;
	priv_calc_mesh_type (gvc);

	for (u8 iQuad=0; iQuad<node->num_adj_vtx; iQuad++)
	{
		priv_calc_mesh_type (node->connected_vtx[iQuad]);
		priv_calc_mesh_type (node->other_vtx[iQuad]);
	}
}

//************************************* 
void Map2::set_node_height (const GVC gvc, u16 height)
{
	Node *node = priv_examap__get_nodePointer (gvc);
	if (NULL != node)
		priv_do_set_node_height (gvc, node, height);
}

//************************************* 
void Map2::inc_node_height (const GVC gvc, u16 h)
{
	Node *node = priv_examap__get_nodePointer (gvc);
	if (NULL != node)
	{
		const u16 height = node->height + h;
		priv_do_set_node_height (gvc, node, height);
	}
}

//************************************* 
void Map2::dec_node_height (const GVC gvc, u16 h)
{
	Node *node = priv_examap__get_nodePointer (gvc);
	if (NULL != node)
	{
		u16 height = 0;
		if (node->height >= h)
			height = node->height - h;
		priv_do_set_node_height (gvc, node, height);
	}
}

//************************************* 
void Map2::priv_calc_mesh_type (const GVC gvc)
{
	Node *node = priv_examap__get_nodePointer (gvc);
	if (NULL == node)
		return;

//	bool bModified = false;
	for (u8 iQuad=0; iQuad<node->num_adj_vtx; iQuad++)
	{
		GVC gvc3;
		if (iQuad + 1 == node->num_adj_vtx)
			gvc3 = node->connected_vtx[0];
		else
			gvc3 = node->connected_vtx[iQuad+1];

		const Node *node1 = priv_examap__get_nodePointer (node->connected_vtx[iQuad]);
		const Node *node2 = priv_examap__get_nodePointer (node->other_vtx[iQuad]);
		const Node *node3 = priv_examap__get_nodePointer (gvc3);

		u8 mask = 0;
		if (node1->height < node->height) 	mask |= 0x01;
		if (node2->height < node->height)	mask |= 0x02;
		if (node3->height < node->height)	mask |= 0x04;

		eMeshType mt;
		switch (mask)
		{
		case 0x00:	mt = eMeshType::full; break;
		case 0x01:	mt = eMeshType::bordo_singolo_su; break;
		case 0x02:	mt = eMeshType::angolo_interno; break;
		case 0x03:	mt = eMeshType::bordo_singolo_su; break;
		case 0x04:	mt = eMeshType::bordo_singolo_dx; break;
		case 0x05:	mt = eMeshType::bordo_strano; break;
		case 0x06:	mt = eMeshType::bordo_singolo_dx; break;
		case 0x07:	mt = eMeshType::angolo; break;
		}


		if (mt != node->mesh_type[iQuad])
		{
			node->mesh_type[iQuad] = mt;
	//		bModified = true;
		}
	}
}

//************************************* 
u32 Map2::get_exa_vtxList (const gos::examap::Coord &exa_coord, FastArray<Vtx> &outList, bool bClear_outList) const
{
	if (bClear_outList)
		outList.reset();
	if (!examap.exists(exa_coord))
		return 0;


	//metto in <outlist> l'elenco dei vtx di exa_coord
	const ExaInfo *exaInfo = examap.query_pointer (exa_coord);
	if (NULL == exaInfo)
		return 0;

	gos::FastHashMap<GVC, u32>	map (gos::getScrapAllocator(), 256);
	for (u32 i=0; i<exaInfo->num_node; i++)
	{
		const Node *node = &exaInfo->node_list[i];
		if (node->gvc.is_valid())
		{
			Vtx vtx;
			priv_node_to_vtx (node, &vtx);

			const u32 index = outList.getNElem ();
			map.insertIfNotExists (node->gvc, index);
			outList.append(vtx);
		}
	}

	const u32 ret = outList.getNElem();

	//sistemo le adiacenze
	for (u32 iVtx = 0; iVtx < ret; iVtx++)
	{
		Vtx *v = &outList[iVtx];
		const Node *node = &exaInfo->node_list[ v->gvc.get_vertex_idx() ];

		for (u32 t = 0; t < v->num_adj_vtx; t++)
		{
			const GVC gvc = node->connected_vtx[t];
			
			u32 index;
			//stiamo puntando ad un vtx situato su un altro exa
			if (!map.find (gvc, &index))
			{
				const Node *node2 = priv_examap__get_nodePointer(node->connected_vtx[t]);
				if (NULL == node2)
					node2 = temp_node_list.query_pointer(node->connected_vtx[t]);

				assert (NULL != node2);
				Vtx vtx;
				priv_node_to_vtx (node2, &vtx);
				vtx.num_adj_vtx=0;

				index = outList.getNElem ();
				map.insertIfNotExists (gvc, index);
				outList.append(vtx);
			}
			
			assert (t<6);
			v->adj_vtx_list[t] = (u16)index;
		}
	}

	return ret;
}


//************************************* 
static u64 Land1_map__make_key_1 (const GVC gvc1, const GVC gvc2)
{
	const u64 a = gvc1.get_as_u32();
	const u64 b = gvc2.get_as_u32();
	if (a < b)
		return ( (a << 32)| b );
	return ( (b << 32)| a );
}

static gos::Key128 Land1_map__make_key_2 (const GVC gvc1, const GVC gvc2, const GVC gvc3, const GVC gvc4)
{
	u64 k[4] = { gvc1.get_as_u32(), gvc2.get_as_u32(), gvc3.get_as_u32(),gvc4.get_as_u32() };

	bool bEsci = false;
	u8 n = 4;
	while (bEsci == false)
	{
		bEsci = true;
		n--;
		for (u8 i=0; i<n; i++)
		{
			if (k[i] < k[i+1])
			{
				bEsci = false;
				GOSSWAP(k[i], k[i+1]);
			}
		}
	}

	Key128 ret;
	ret.high = (k[0] << 32) | k[1];
	ret.low  = (k[2] << 32) | k[3];
	return ret;
}

//************************************* 
Land1::ExaR* Map2::calc_exaR (gos::Allocator *allocatorIN, const gos::examap::Coord &exa_coord) const
{
	const ExaInfo *exaInfo = examap.query_pointer (exa_coord);
	if (NULL == exaInfo)
		return NULL;

	//mappa d'appoggio per la creazione dei vtx dei quad
	FastHashMap<u64, u16> edge_vtx_map(gos::getScrapAllocator(), exaInfo->num_node);
	FastHashMap<Key128, u16> qc_vtx_map(gos::getScrapAllocator(), exaInfo->num_node);
		


	//i vtx utili al rendering sono:
	//	- tutti i vtx originali
	//	- per ogni vtx originale, tutti i vtx degli N quad che lo interessano
	const u32 estimated_num_vtx = exaInfo->num_node	//vtx originali
								+exaInfo->num_node 	//quad center
								+exaInfo->num_node;	//vtx addizionali per i quad

	FastArray<vec2f> vtx_list (gos::getScrapAllocator(), estimated_num_vtx);
	FastArray<ExaR::VtxInfo> vtxinfo_list (gos::getScrapAllocator(), exaInfo->num_node);

	for (u32 iNode=0; iNode<exaInfo->num_node; iNode++)
	{
		const Node *node = &exaInfo->node_list[iNode];
		if (!node->gvc.is_valid())
			continue;
		if (node->num_adj_vtx < 3)
			continue;


		//addo il vtx del centro
		const u16 node_center_idx = (u16)vtx_list.getNElem();
		vtx_list.append (node->pos);

		//addo i quad center
		u16 quad_center_idx[8];
		for (u32 iQuad=0; iQuad<node->num_adj_vtx; iQuad++)
		{
			GVC cc;
			if (iQuad + 1 == node->num_adj_vtx)
				cc = node->connected_vtx[0];
			else
				cc = node->connected_vtx[iQuad + 1];

			const Key128 key = Land1_map__make_key_2 (node->gvc, node->connected_vtx[iQuad], node->other_vtx[iQuad], cc);
			
			u16 idx;
			if (!qc_vtx_map.find(key, &idx))
			{
				idx = (u16)vtx_list.getNElem();
				qc_vtx_map.insertIfNotExists (key, idx);
				vtx_list.append (node->quad_center[iQuad]);
			}

			quad_center_idx[iQuad] = idx;
		}

		//addo gli edge vertex
		u16 edge_vtx_idx[8];
		for (u32 iQuad=0; iQuad<node->num_adj_vtx; iQuad++)
		{
			const u64 edge_key = Land1_map__make_key_1( node->gvc, node->connected_vtx[iQuad] );
			u16 idx;
			if (!edge_vtx_map.find(edge_key, &idx))
			{
				idx = (u16)vtx_list.getNElem();
				edge_vtx_map.insertIfNotExists (edge_key, idx);

				//calcolo la posizione del vtx che e' l'intersezione di node->coord, node->connected_vtx[iQuad] contro
				// quad_center[iQuad], quad_center[iQuad-1]
				vec3f p;
				const vec2f A = node->pos;

				GVC_to_world_coord (node->connected_vtx[iQuad], &p);
				const vec2f B (p.x, p.z);

				const vec2f C ( vtx_list(quad_center_idx[iQuad]) );

				u32 ii;
				if (0 == iQuad) ii = node->num_adj_vtx - 1;
				else ii = iQuad -1;
				const vec2f D ( vtx_list(quad_center_idx[ii]) );

				vec2f pp;
				if (geom::line2D__intersect (A, B, C, D, &pp))
					vtx_list.append (pp);
				else
				{
					DBGBREAK;
				}
			}

			edge_vtx_idx[iQuad] = idx;
		}

		//memmo tutto 
		ExaR::VtxInfo vtxinfo;
		vtxinfo.num_quad = node->num_adj_vtx;
		vtxinfo.material_index = node->material_index;
		vtxinfo.height = node->height;
		
		u32 num_idx = 0;
		vtxinfo.idx_list[num_idx++] = node_center_idx;
		for (u32 iQuad=0; iQuad<node->num_adj_vtx; iQuad++)
		{
			vtxinfo.mesh_type[iQuad] = node->mesh_type[iQuad];
			vtxinfo.idx_list[num_idx++] = edge_vtx_idx[iQuad];
			vtxinfo.idx_list[num_idx++] = quad_center_idx[iQuad];
		}
		assert (num_idx <= 13);

		vtxinfo_list.append (vtxinfo);
	}


	//alloco ExaR
	ExaR *exar = ExaR::alloc (allocatorIN, vtxinfo_list.getNElem(), vtx_list.getNElem());
	memcpy (exar->vtxList, vtx_list._queryPointer(), sizeof(vec2f) * vtx_list.getNElem());
	memcpy (exar->vtxInfoList, vtxinfo_list._queryPointer(), sizeof(ExaR::VtxInfo) * vtxinfo_list.getNElem());

	return exar;
}


