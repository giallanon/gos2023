#include "map.h"
#include "land.h"
#include "gosGeomIntersect3D.h"


using namespace gos;
using namespace land;


#define DEBUG__MIN_HEIGHT 0
#define DEBUG__MAX_HEIGHT 50.0f

//********************************
Map::MapQTree::MapQTree()
{
	localAllocator = NULL;
	num_vtx_per_chunk_side = 1+ 64;
}

//********************************
void Map::MapQTree::unsetup()
{
	if (NULL == localAllocator)
		return;
	tmp_ccList1.unsetup();
	tmp_ccList2.unsetup();
	localAllocator = NULL;
}

//********************************
void Map::MapQTree::setup (gos::Allocator *allocator, const Map *map, u32 num_vtx_per_chunk_sideIN)
{
	assert (GOS_IS_POWER_OF_TWO(num_vtx_per_chunk_sideIN-1) );
	unsetup();

	localAllocator = allocator;
	tmp_ccList1.setup (localAllocator, 1024);
	tmp_ccList2.setup (localAllocator, 1024);
	num_vtx_per_chunk_side = num_vtx_per_chunk_sideIN;


	const u32 map_num_points_per_lato = map->map__get_num_points_per_lato();
	assert (map_num_points_per_lato > num_vtx_per_chunk_side);
	assert (GOS_IS_POWER_OF_TWO(map_num_points_per_lato));

	num_lod = (u8)map->map__get_num_lod();
	assert (num_lod > 0);
	assert (num_lod <= NUM_MAX_LOD);
	
	//divido la mappa in chunk da (num_vtx_per_chunk_side+1) vtx
	const u32 num_chunk_per_lato = map_num_points_per_lato / (num_vtx_per_chunk_side-1);
	assert (GOS_IS_POWER_OF_TWO(num_chunk_per_lato));
		

	const f32 r = land::resolution_to_m(map->map__get_worst_resolution());
	const f32 border_size = r * (f32) ((num_vtx_per_chunk_side-1) * num_chunk_per_lato);


	const vec2f map_top_left_WC = map->map__get_topLeft_WC();
	const f32 x1 = map_top_left_WC.x;
	const f32 z2 = map_top_left_WC.y;
	const f32 x2 = x1 + border_size;
	const f32 z1 = z2 - border_size;

	map_aabb.setByMinMax (vec3f(x1, DEBUG__MIN_HEIGHT, z1), vec3f(x2, DEBUG__MAX_HEIGHT, z2) );

	lodInfo[0].num_chunk_per_lato = num_chunk_per_lato;
	lodInfo[0].chunk_border_size__m = (num_vtx_per_chunk_side - 1) * r;
	lodInfo[0].min_visible_dist_squared__m = lodInfo[0].chunk_border_size__m * 2.0f;
	lodInfo[0].resol = map->map__get_worst_resolution();

	for (u8 lod=1; lod<num_lod; lod++)
	{
		lodInfo[lod].num_chunk_per_lato = lodInfo[lod-1].num_chunk_per_lato * 2;
		lodInfo[lod].chunk_border_size__m = lodInfo[lod-1].chunk_border_size__m * 0.5f;
		lodInfo[lod].min_visible_dist_squared__m = lodInfo[lod].chunk_border_size__m * 2.0f;
		lodInfo[lod].resol = land::resolution_prev( lodInfo[lod-1].resol );
	}

	for (u8 lod=0; lod<num_lod; lod++)
		lodInfo[lod].min_visible_dist_squared__m *= lodInfo[lod].min_visible_dist_squared__m;
}

//********************************
void Map::MapQTree::aabb_from_coord (const QTreeCoord cc, gos::geom::AABB3 *out) const
{
	assert (NULL != out);
	const u8 lod = cc.get_lod();
	const u32 cx = cc.get_cx();
	const u32 cy = cc.get_cy();	

	assert (lod < num_lod);
	assert (cx < lodInfo[lod].num_chunk_per_lato);
	assert (cy < lodInfo[lod].num_chunk_per_lato);

	const f32 border = lodInfo[lod].chunk_border_size__m;
	const f32 x1 = map_aabb.vmin.x + cx * border;
	const f32 x2 = x1 + border;
	const f32 z2 = map_aabb.vmax.z - cy * border;
	const f32 z1 = z2 - border;
	out->setByMinMax ( vec3f(x1, DEBUG__MIN_HEIGHT, z1), vec3f(x2, DEBUG__MAX_HEIGHT, z2) );
}

//********************************
u32 Map::MapQTree::calc_visibility (gos::geom::Camera3 *cam, QTreeCoordList *out)
{
	out->reset();

	geom::Frustum3 fr = cam->get_frustumWC();
	geom::AABB3 frAABB;
	fr.calc_AABB(&frAABB);

	if (eClipResult::outside == AABB3__intersect_AABB3(map_aabb, frAABB))
		return 0;

	QTreeCoordList	*ccList1 = &tmp_ccList1;
	QTreeCoordList	*ccList2 = &tmp_ccList2;


	ccList1->reset();
	ccList2->reset();
	for (u32 cy=0; cy<lodInfo[0].num_chunk_per_lato; cy++)
	{
		for (u32 cx=0; cx<lodInfo[0].num_chunk_per_lato; cx++)
		{
			QTreeCoord cc;
			cc.set (0, cx, cy);

			geom::AABB3 aabb;
			aabb_from_coord (cc, &aabb);

			if (eClipResult::outside == AABB3__intersect_AABB3(aabb, frAABB))
				continue;
			if (eClipResult::outside == AABB3__intersect_frustum3(aabb, fr))
				continue;


			vec3f v;
			aabb.calcCenter(&v);
			if (math::distance2 (cam->pos.o, v) < lodInfo[0].min_visible_dist_squared__m)
			{
				ccList1->append (cc);
			}
			else
			{
				out->append (cc);
			}
		}
	}


	for (u8 lod=1; lod<num_lod; lod++)
	{
		ccList2->reset();
		const u32 nChunk = ccList1->getNElem();
		for (u32 i=0; i<nChunk; i++)
		{
			QTreeCoord cc = ccList1->queryElem(i);
			assert (lod == cc.get_lod() + 1);
			const u32 cx = cc.get_cx() << 1;
			const u32 cy = cc.get_cy() << 1;
			
			//il chunk cc a livello lod devo splittarlo in 4 a livello lod +1 
			//e poi cullare quelli
			for (u8 y=0; y<2; y++)
			{
				for (u8 x=0; x<2; x++)
				{
					cc.set (lod, cx+x, cy+y);

					geom::AABB3 aabb;
					aabb_from_coord (cc, &aabb);

					if (eClipResult::outside == AABB3__intersect_AABB3(aabb, frAABB))
						continue;
					if (eClipResult::outside == AABB3__intersect_frustum3(aabb, fr))
						continue;

					if (lod == num_lod-1)
						out->append (cc);
					else
					{
						vec3f v;
						aabb.calcCenter(&v);
						if (math::distance2 (cam->pos.o, v) < lodInfo[lod].min_visible_dist_squared__m)
						{
							ccList2->append (cc);
						}
						else
						{
							out->append (cc);
						}
					}
				}
			}
		}
		ccList1->reset();

		GOSSWAP(ccList1, ccList2);
	}
	


	return out->getNElem();
}

