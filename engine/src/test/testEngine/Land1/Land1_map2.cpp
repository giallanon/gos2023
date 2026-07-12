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
	vtxmap.reset();
	hexmap.reset();
}

//************************************* 
void Map2::setup (gos::Allocator *allocator)
{
	localAllocator = allocator;
	vtxmap.setup (localAllocator, 1024);
	hexmap.setup (localAllocator, 1024);
}

//************************************* 
bool Map2::priv_vtxmap__add_vtx (const GVC gvc, const Vtx &vtxIN)
{
	const u32 key = gvc.get_as_u32();
	return vtxmap.insertIfNotExists (key, vtxIN);
}

//************************************* 
bool Map2::priv_vtxmap__get_vtx (const GVC gvc, Vtx *out) const
{
	return vtxmap.find (gvc.get_as_u32(), out);
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
void Map2::exa__add (const gos::examap::Coord coordIN)
{
	if (hexmap.exists (coordIN.pack_coord_u32()))
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


	//addo l'exa alla mappa di hex
	HexInfo hexinfo;
	hexinfo.coord = coordIN;
	hexmap.insertIfNotExists (coordIN.pack_coord_u32(), hexinfo);

	//Creo un elenco di vtx dell'exa e dtermino GVC per ciascuno
	FastArray<Vtx> exavtx (gos::getScrapAllocator(), num_vtx_originali);
	for (u32 iVtx=0; iVtx<num_vtx_originali; iVtx++)
	{
		const ExaGenerator::VtxInfo *vi = &exagen.vtxList[iVtx];

		//creo il vtx
		Vtx vv;
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
		
		exavtx[iVtx].num_adj_vtx = (u8)nquad;
		for (u32 t=0; t<nquad; t++)
		{
			const u32 quad_index = adj_quad_list[t];
			const u32 B_index = exagen.get_index_of_vtx_in_uscita_da (quad_index, iVtx);

			exavtx[iVtx].connected_vtx[t] = exavtx[B_index].coord;
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
			//delle sue adj integrandolo con quelle di <vv>
			Vtx vReal;
			priv_vtxmap__get_vtx (gvc, &vReal);

			const u32 n = exavtx(iVtx).num_adj_vtx;
			for (u32 i=0; i<n; i++)
			{
				bool bFound = false;
				for (u32 t=0; t<vReal.num_adj_vtx; t++)
				{
					if (vReal.connected_vtx[t] == gvc)
					{
						bFound = true;
						break;
					}
					if (!bFound)
					{
						const u32 n = vReal.num_adj_vtx++;
						vReal.connected_vtx[n] = gvc;
					}
				}
			}

			vtxmap.insertOrReplaceValue (gvc.get_as_u32(), vReal);
		}
	}

}

//************************************* 
bool Map2::get_list_of_vtx_by_exa (const gos::examap::Coord &exa_coord, FastArray<Vtx> &out_list, bool bClearOut) const
{
	if (bClearOut)
		out_list.reset();

	if (!hexmap.exists (exa_coord.pack_coord_u32()))
	{
		return false;
	}

	vtxmap.forEach ( [exa_coord, &out_list](u32 key, const Vtx vtx) {

//		if (vtx.coord.get_exa_coord() == exa_coord)
		{
			out_list.append (vtx);
		}
		return true;
	});

	return true;
}