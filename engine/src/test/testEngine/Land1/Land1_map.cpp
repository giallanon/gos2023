#include "Land1_map.h"
#include "gos.h"
#include "../PerlinNoise.hpp"

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
	
	to_alloc = utils::calcNextMultipleOf8(to_alloc);
	const u32 offset_to_vtx_list = to_alloc;
	to_alloc += sizeof(vec2f) * num_vtx;

	to_alloc = utils::calcNextMultipleOf8(to_alloc);
	const u32 offset_to_quad_list = to_alloc;
	to_alloc += sizeof(Exa::Quad) * num_quad;

	u8 *p = GOSALLOCT(u8*, localAllocator, to_alloc);
	ret = reinterpret_cast<Exa*>(p);
	memset (ret, 0, to_alloc);
	ret->num_vtx = (u16)num_vtx;
	ret->num_quad= (u16)num_quad;
	ret->vtxList = reinterpret_cast<vec2f*>( &p[offset_to_vtx_list] );
	ret->quadList = reinterpret_cast<Exa::Quad*>( &p[offset_to_quad_list] );


	//ora copio vtx e quadlist
	for (u32 i=0; i<num_vtx; i++)
		ret->vtxList[i].set (exagen.vtxList(i).x, exagen.vtxList(i).z);

	for (u32 i = 0; i < num_quad; i++)
	{
		ret->quadList[i].height = 0;
		ret->quadList[i].material_index = 0;
		ret->quadList[i].idx[0] = (u8)exagen.quadList(i).vtx_idx0;
		ret->quadList[i].idx[1] = (u8)exagen.quadList(i).vtx_idx1;
		ret->quadList[i].idx[2] = (u8)exagen.quadList(i).vtx_idx2;
		ret->quadList[i].idx[3] = (u8)exagen.quadList(i).vtx_idx3;
	}

	return ret;
}

//*****************************************
void Map::priv_exa_free (Exa *exa)
{
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
				vec2f c = exa->calc_quad_center(i);
				c /= (exa_radius_world);

				f32 h = (f32)perlin.octave2D_01(c.x, c.y, 2);
				//if (h > 0.8)	h = 4.0f;
				//else if (h > 0.5)	h = 2.0f;
				//else h = 0;

				h = 0;
				exa->quadList[i].height = h;
				exa->quadList[i].material_index = gos::randomU32(2);
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