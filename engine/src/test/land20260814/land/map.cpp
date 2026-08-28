#include "map.h"
#include "land.h"
#include "gosImageBufferRGBA.h"
#include "gosGeomIntersect3D.h"
#include "../gosGameUtils/gosGameUtils.h"

using namespace gos;
using namespace land;

typedef gos::AllocatorHeap<gos::AllocPolicy_Track_simple, gos::AllocPolicy_Thread_Unsafe>	LandMapMemAllocator;

//********************************
bool Map::create (const char *save_path, const CreateData &create)
{
	gos::Allocator *localAllocator = gos::getSysHeapAllocator();
	gos::err::clear();


	if (create.lod0__num_vtx_lato < 5)
	{
		logger::err ("lod0__num_vtx_lato must be at least 5\n");
		return false;
	}
	if (!GOS_IS_POWER_OF_TWO(create.lod0__num_vtx_lato -1))
	{
		logger::err ("lod0__num_vtx_lato must be equal to '1 + a power of 2 '\n");
		return false;
	}


	//la mappa e' quadrata di lato <map__border_size__m> con risoluzione <lod0__scala_xz__m>
	const u32 chunk__num_vtx_lato = create.lod0__num_vtx_lato;
	const f32 chunk__border_size__m = (chunk__num_vtx_lato -1) * create.lod0__scala_xz__m;
	
	//numero di chunk x (e y)
	//voglio che siano dispari in modo che dal chunk centrale (0,0) si estendano N chuml in ogni direzione
	u32 chunk__num = (u32)math::floor (create.map__border_size__m / chunk__border_size__m);
	if (chunk__num * chunk__border_size__m < create.map__border_size__m)
		chunk__num++;
	if (0 == chunk__num % 2) chunk__num++;

	const f32 map__border_size__m = chunk__num * chunk__border_size__m;

	//voglio che il chunk 0,0 abbia origine in world coordinate 0,0
	const u32 mid_chunk = chunk__num / 2;
	const f32 map__min = -(mid_chunk * chunk__border_size__m);
	const f32 map__max = -map__min + chunk__border_size__m;
	
	//calcolo la dimensione in byte di un chunk
	//per questioni di allineamento memoria, voglio che <sizeof__chunk> sia uym multiplo di 64
	u32 lod0__sizeof = chunk__num_vtx_lato * chunk__num_vtx_lato * sizeof(ChunkData);
	lod0__sizeof = GOS_ALIGN_NUMBER_TO_POWER_OF_TWO(lod0__sizeof, 64);



	//se esiste gia' un folder in <save_path>, elimino tutto e poi lo ricreo
	fs::folderDeleteAllFileRecursively (save_path, eFolderDeleteMode::deleteAlsoTheSubfolder);
	if (!fs::folderCreate (save_path))
	{
		logger::err ("Unable to create folder %s\n", save_path);
		return false;
	}

	//creo il file .map con le info sulla mappa
	char s[1024];
	{
		sprintf_s (s, sizeof(s), "%s/map", save_path);
		gos::File f;
		if (!fs::fileOpenForW (&f, s))
		{
			logger::err ("Unable to create file %s\n", s);
			return false;
		}

		u8 buffer[1024];
		u32 ct = 0;
		ct += utils::bufferWriteU32 (&buffer[ct], Map::VERSION);
		ct += utils::bufferWriteU32 (&buffer[ct], chunk__num_vtx_lato);
		ct += utils::bufferWriteU32 (&buffer[ct], chunk__num);
		ct += utils::bufferWriteU32 (&buffer[ct], mid_chunk);
		ct += utils::bufferWriteU32 (&buffer[ct], lod0__sizeof);

		ct += utils::bufferWriteF32 (&buffer[ct], create.lod0__scala_xz__m);
		ct += utils::bufferWriteF32 (&buffer[ct], chunk__border_size__m);
		ct += utils::bufferWriteF32 (&buffer[ct], map__border_size__m);
		ct += utils::bufferWriteF32 (&buffer[ct], map__min);
		ct += utils::bufferWriteF32 (&buffer[ct], map__max);

		fs::fileWrite (f, buffer, ct);
		fs::fileClose(f);
	}


	//preparo l'array che conterra' le info sui chunk
	const u32 sizeof__chunk_info = chunk__num * chunk__num * sizeof(ChunkInfo);
	ChunkInfo *chunk_info = GOSALLOCT(ChunkInfo*, localAllocator, sizeof__chunk_info);


	//genero i chunk
	//L'altezza nei chunk e' espressa in step da 0.1m
	{
		ChunkData *chunk = GOSALLOCT(ChunkData*, localAllocator, lod0__sizeof);
		//memset (chunk, 0, lod0__sizeof);

		const u16 default_h = (u16) (create.map__default_height__m * 10.0f);
		u32 ct_chunk = 0;
		for (u32 cy=0; cy<chunk__num; cy++)
		{
			for (u32 cx=0; cx<chunk__num; cx++)
			{
				u16 height_min = u16MAX;
				u16 height_max = 0;
				u32 ct = 0;
				for (u32 y=0; y<chunk__num_vtx_lato; y++)
				{
					for (u32 x=0; x<chunk__num_vtx_lato; x++)
					{
						chunk[ct].height = default_h;
						
						chunk[ct].ao = 0;
						chunk[ct].materialID = MATERIAL_ID__LUSH_GRASS;
						chunk[ct].encoded_norm = utils::normal_encode_octahedral(vec3f(0,1,0));

						if (chunk[ct].height < height_min)	height_min=chunk[ct].height;
						if (chunk[ct].height > height_max)	height_max=chunk[ct].height;
						ct++;
					}
				}

				priv__save_chunk_data (save_path, cx, cy, chunk, lod0__sizeof);

				if (height_max == height_min)
					height_max++;
				chunk_info[ct_chunk].min_height__m = 0.1f * height_min;
				chunk_info[ct_chunk].max_height__m = 0.1f * height_max;
				ct_chunk++;
			}
		}
		GOSFREE(localAllocator, chunk);
	}

	//salvo chunk info
	priv__save_chunk_info (save_path, chunk_info, sizeof__chunk_info);
	GOSFREE(localAllocator, chunk_info);


	//fine
	return !err::anyError();
}

//********************************
bool Map::priv__save_chunk_data (const char *folder, u32 cx, u32 cy, const ChunkData *chunkIN, u32 sizeof_chunk)
{
	assert (NULL != chunkIN);

	char s[1024];
	sprintf_s (s, sizeof(s), "%s/chunk_%03d_%03d", folder, cx, cy);
	return fs::fileSaveBuffer (s, chunkIN, sizeof_chunk);
}

//********************************
bool Map::priv__save_chunk_info (const char *folder, const ChunkInfo *ciIN, u32 sizeof__chunk_info)
{
	assert (NULL != ciIN);

	char s[1024];
	sprintf_s (s, sizeof(s), "%s/chunk_info", folder);
	return fs::fileSaveBuffer (s, ciIN, sizeof__chunk_info);
}

//********************************
bool Map::create_from_hmap (const char *path_to_hmap, f32 height_resolution)
{
	err::clear();

	image::BufferRGBA im;
	if (!im.loadFromFile (gos::getScrapAllocator(), path_to_hmap))
	{
		logger::err ("can't load %s\n", path_to_hmap);
		return false;
	}

	const f32 RESOLUTION = 0.5f;
	CreateData create;
	create.lod0__scala_xz__m = RESOLUTION;
	create.lod0__num_vtx_lato = 129;
	create.map__border_size__m = RESOLUTION * GOSMAX( im.getW(), im.getH());
	create.map__default_height__m = 0;


	char save_path[1024];
	sprintf_s (save_path, sizeof(save_path), "%s", path_to_hmap);
	fs::remove_ext_in_place (save_path);
	if (!Map::create (save_path, create))
	{
		im.free (gos::getScrapAllocator());
		return false;
	}

	//carico la mappa e centro la hmap
	Map map;
	map.load (save_path);
	const u32 map__num_vtx = 1 + map.chunk__num * (map.chunk__num_vtx_lato-1);
	const u32 x1 = (map__num_vtx - im.getW()) / 2;
	const u32 y1 = (map__num_vtx - im.getH()) / 2;

	Map::MapUpdate mu;
	mu.setup (gos::getScrapAllocator());
	map.begin_update(mu);
	{
		u32 hmap_ct = 0;
		const u8 *hmap_buffer = im.getBuffer();
		for (u32 y=0; y<im.getH(); y++)
		{
			for (u32 x=0; x<im.getW(); x++)
			{
				const u8 red = hmap_buffer[hmap_ct];
				hmap_ct+=4;
				map.set_height (mu, x1 + x, y1 + y, (f32)red * height_resolution);
			}
		}
	}
	map.end_update(mu);

	im.free (gos::getScrapAllocator());
	return true;
}

//********************************
Map::Map()
{
	LandMapMemAllocator *myAllocator = GOSNEW(gos::getSysHeapAllocator(), LandMapMemAllocator)("LandMap");
	myAllocator->setup (1024 * 1024 * 128); //128MB
	this->localAllocator = myAllocator;

	chunk_data = NULL;
	chunk_info = NULL;
	memset (path_to_folder, 0, sizeof(path_to_folder));
}

//********************************
Map::~Map()
{ 
	priv__free(); 
	GOSDELETE(gos::getSysHeapAllocator(), localAllocator);
	localAllocator = NULL;
}

//********************************
void Map::priv__free()
{
	if (NULL != chunk_data)			GOSFREE_AND_NULL (localAllocator, chunk_data);
	if (NULL != chunk_info)			GOSFREE_AND_NULL (localAllocator, chunk_info);
}

//********************************
bool Map::load (const char *path_to_folderIN)
{
	priv__free();
	
	char s[1024];
	fs::resolvePath (path_to_folderIN, path_to_folder, sizeof(path_to_folder));

	//carico il file <map>	
	{
		sprintf_s (s, sizeof(s), "%s/map", path_to_folder);
		u32 fsize;
		u8 *buffer = fs::fileLoadInMemory (gos::getScrapAllocator(), s, &fsize);
		if (NULL == buffer)
		{
			logger::err ("can't open %s\n", s);
			return false;
		}

		u32 ct = 0;
		u32 ver = utils::bufferReadU32 (&buffer[ct]);
		ct+=4;
		if (!magic::signatureMatch(ver, Map::VERSION) || !magic::versionMatch(ver, Map::VERSION))
		{
			logger::err ("Invalid magic or version [%s]\n", s);
			ver = 0;
		}
		else
		{
			chunk__num_vtx_lato = utils::bufferReadU32 (&buffer[ct]);	ct+=4;
			chunk__num = utils::bufferReadU32 (&buffer[ct]);	ct+=4;
			mid_chunk = utils::bufferReadU32 (&buffer[ct]);	ct+=4;
			lod0__sizeof = utils::bufferReadU32 (&buffer[ct]);	ct+=4;

			lod0__scala_xz__m = utils::bufferReadF32 (&buffer[ct]);		ct+=4;
			chunk__border_size__m = utils::bufferReadF32 (&buffer[ct]);		ct+=4;
			map__border_size__m = utils::bufferReadF32 (&buffer[ct]);		ct+=4;
			map__min = utils::bufferReadF32 (&buffer[ct]);	ct+=4;
			map__max = utils::bufferReadF32 (&buffer[ct]);	ct+=4;
		}

		GOSFREE(gos::getScrapAllocator(), buffer);
		if (0 == ver)
			return false;
	}

	//alloco e carico chunk_info
	sprintf_s (s, sizeof(s), "%s/chunk_info", path_to_folder);
	{
		u32 fsize;
		chunk_info = reinterpret_cast<ChunkInfo*> (fs::fileLoadInMemory (localAllocator, s, &fsize));
		if (NULL == chunk_info)
		{
			logger::err ("can't open %s\n");
			return false;
		}
	}


	//alloco e carico i chunk
	chunk_data = GOSALLOCT(u8*, localAllocator, lod0__sizeof * chunk__num * chunk__num);
	{
		u32 ct = 0;
		for (u32 cy=0; cy<chunk__num; cy++)
		{
			for (u32 cx=0; cx<chunk__num; cx++)
			{
				sprintf_s (s, sizeof(s), "%s/chunk_%03d_%03d", path_to_folder, cx, cy);
				gos::File f;
				if (!fs::fileOpenForR (&f, s))
				{
					logger::err ("can't open %s\n");
					return false;
				}

				fs::fileRead (f, &chunk_data[ct], lod0__sizeof);
				ct += lod0__sizeof;
				fs::fileClose(f);
			}
		}
	}
	
	return true;
}

//********************************
bool Map::priv__world_to_chunk (f32 wx, f32 wz, u32 *out__cx, u32 *out__cy) const
{
	assert (NULL != out__cx);
	assert (NULL != out__cy);

	//la mappa e' un quadrato (map__min, map__min) (map__max, map__max)
	//Il chunk (0,0) e' il primo in alto a sinistra
	//il chunk (mid_chunk, mid_chunk) e' quello che ha origine in woorld coord (0,0)
	if (wx < map__min || wx >= map__max) return false;
	if (wz < map__min || wz >= map__max) return false;
	wx -= map__min;
	wz -= map__min;

	*out__cx = (u32) math::floor(wx / chunk__border_size__m);
	assert (*out__cx >= 0 && *out__cx < chunk__num);

	*out__cy = (chunk__num - 1) - (u32)math::floor(wz / chunk__border_size__m);
	assert (*out__cy >= 0 && *out__cy < chunk__num);
	return true;
}

//********************************
bool Map::priv__chunk_to_world (u32 cx, u32 cy, f32 *out__wx, f32 *out__wz) const
{
	if (cx >= chunk__num)	return false;
	if (cy >= chunk__num)	return false;

	*out__wx = map__min + cx * chunk__border_size__m;
	*out__wz = map__max - (cy+1) * chunk__border_size__m;

	assert (*out__wx >= map__min);
	assert (*out__wx < map__max);
	assert (*out__wz >= map__min);
	assert (*out__wz < map__max);
	return true;
}

//********************************
const Map::ChunkInfo* Map::priv__chunk_get_info (u32 cx, u32 cy) const
{
	if (cx >= chunk__num)	return NULL;
	if (cy >= chunk__num)	return NULL;;
	return &chunk_info[cy * chunk__num + cx];
}

//********************************
Map::ChunkInfo* Map::priv__chunk_get_info (u32 cx, u32 cy)
{
	if (cx >= chunk__num)	return NULL;
	if (cy >= chunk__num)	return NULL;;
	return &chunk_info[cy * chunk__num + cx];
}

//***********************************
const Map::ChunkData* Map::chunk__get (u32 cx, u32 cy) const
{
	if (cx >= chunk__num)	return NULL;
	if (cy >= chunk__num)	return NULL;
	return reinterpret_cast<const ChunkData*>( &chunk_data[lod0__sizeof * (cy * chunk__num + cx)] );
}

//***********************************
Map::ChunkData* Map::priv_chunk__get (u32 cx, u32 cy)
{
	if (cx >= chunk__num)	return NULL;
	if (cy >= chunk__num)	return NULL;
	return reinterpret_cast<ChunkData*>( &chunk_data[lod0__sizeof * (cy * chunk__num + cx)] );
}


//***********************************
u32 Map::calc_visible_chunk (gos::geom::Camera3 *cam, gos::FastArray<ChunkCoord> *out) const
{
	assert (NULL != out);
	out->reset();

	const geom::Frustum3 fr = cam->get_frustumWC();
	const f32 VIEW_DISTANCE_SQUARED = fr.get_far_distance() * fr.get_far_distance();

	geom::AABB3 aabb;
	{
		geom::AABB3 fr_aabb;
		fr.calc_AABB (&fr_aabb);

		geom::AABB3 map_aabb ( vec3f(map__min, -1e36f, map__min), vec3f(map__max - lod0__scala_xz__m, 1e36f, map__max - lod0__scala_xz__m) );
		if (!geom::AABB3::clip (fr_aabb, map_aabb, &aabb))
			return 0;

		assert (aabb.vmin.x >= map__min);
		assert (aabb.vmin.z >= map__min);
		assert (aabb.vmax.x <= map__max);
		assert (aabb.vmax.z <= map__max);
	}

	u32 cx_min, cx_max, cy_min, cy_max;
	GOS_DEBUG_ASSERT(  priv__world_to_chunk (aabb.vmin.x, aabb.vmin.z, &cx_min, &cy_max)   );
	GOS_DEBUG_ASSERT(  priv__world_to_chunk (aabb.vmax.x, aabb.vmax.z, &cx_max, &cy_min)   );


	for (u32 cy = cy_min; cy<=cy_max; cy++)
	{
		for (u32 cx = cx_min; cx<=cx_max; cx++)
		{
			const ChunkInfo *ci = priv__chunk_get_info(cx, cy);
			assert (ci);

			f32 x,z;
			GOS_DEBUG_ASSERT(  priv__chunk_to_world (cx, cy, &x, &z)   );
			aabb.vmin.set (x, ci->min_height__m, z);
			aabb.vmax.set (x + chunk__border_size__m, ci->max_height__m, z + chunk__border_size__m);

			if (eClipResult::outside != geom::AABB3__intersect_frustum3 (aabb, fr) )
			{
				const f32 height_mid = ci->min_height__m + (ci->max_height__m - ci->min_height__m) * 0.5f;
				ChunkCoord cc;
				cc.originWC.set (x, height_mid, z);
				cc.centerWC.set (x + chunk__border_size__m*0.5f, height_mid, z + chunk__border_size__m*0.5f);
				
				cc.distance2_from_pov = math::distance2 (cam->pos.o, cc.centerWC);
				if (cc.distance2_from_pov < VIEW_DISTANCE_SQUARED)
				{
					cc.chunk_x = (u16)cx;
					cc.chunk_y = (u16)cy;
					out->append (cc);
				}
			}

		}
	}

	return out->getNElem();
}


//********************************
bool Map::priv_chunk__set_height (u32 cx, u32 cy, u32 vtx_x, u32 vtx_y, f32 height_m)
{
	ChunkData *c = priv_chunk__get(cx, cy);
	if (NULL == c)
		return false;

	assert (vtx_x < chunk__num_vtx_lato);
	assert (vtx_y < chunk__num_vtx_lato);
	c[vtx_x + vtx_y * chunk__num_vtx_lato].height = (u16)math::floor( 10.0f * height_m );
	return true;
}

//********************************
void Map::begin_update(MapUpdate &mu)
{
	mu.list_of_modified_chunk.reset();

}

//********************************
void Map::set_height (MapUpdate &mu, u32 vtx_x, u32 vtx_y, f32 height_m)
{
#define STORE_IN_MODIFIED_LIST(cx,cy)		mu.list_of_modified_chunk.insertIfNotExists (  ((u64)(cx) << 32) | (cy)  );


	const u32 cx = vtx_x / (chunk__num_vtx_lato - 1);
	const u32 cy = vtx_y / (chunk__num_vtx_lato - 1);
	const u32 vx = vtx_x - cx * (chunk__num_vtx_lato - 1);
	const u32 vy = vtx_y - cy * (chunk__num_vtx_lato - 1);

	if (priv_chunk__set_height (cx, cy, vx, vy, height_m))
		STORE_IN_MODIFIED_LIST(cx,cy);

	if (vx == 0)
	{
		//sto modificando un vtx che e' sharato anche dal chunk alla mia sx
		if (priv_chunk__set_height (cx-1, cy, (chunk__num_vtx_lato - 1), vy, height_m))
			STORE_IN_MODIFIED_LIST(cx-1, cy);
			

		if (vy == 0)
		{
			//sto modificando un vtx che e' sharato anche dal chunk sotto di me e da quello sotto a destra
			if (priv_chunk__set_height (cx, cy-1, vx, (chunk__num_vtx_lato - 1), height_m))
				STORE_IN_MODIFIED_LIST(cx, cy-1);
			if (priv_chunk__set_height (cx-1, cy-1, (chunk__num_vtx_lato - 1), (chunk__num_vtx_lato - 1), height_m))
				STORE_IN_MODIFIED_LIST(cx-1, cy-1);
		}
	}
	else if (vy == 0)
	{
		//sto modificando un vtx che e' sharato anche dal chunk sopra di me
		if (priv_chunk__set_height (cx, cy-1, vx, (chunk__num_vtx_lato - 1), height_m))
			STORE_IN_MODIFIED_LIST(cx, cy-1);
	}

#undef STORE_IN_MODIFIED_LIST
}

//********************************
void Map::end_update(MapUpdate &mu)
{
	const u32 NUM_VTX_IN_CHUNK = chunk__num_vtx_lato * chunk__num_vtx_lato;

	const f32 HMAP_NUM_VTX_PER_LATO = chunk__num_vtx_lato + 2;
	f32	*hmap = GOSALLOCT(f32*, gos::getScrapAllocator(), sizeof(f32) * HMAP_NUM_VTX_PER_LATO * HMAP_NUM_VTX_PER_LATO);
	memset (hmap, 0, sizeof(f32) * HMAP_NUM_VTX_PER_LATO * HMAP_NUM_VTX_PER_LATO);


	//per tutti i chunk modificati, devo:
	//	- ricalcolare hmin hmax
	//	- ricalcolare le normali
	const FastArray<u64> *list = mu.list_of_modified_chunk._queryList();
	for (u32 i=0; i<list->getNElem(); i++)
	{
		const u64 key = list->queryElem(i);
		const u32 cx = (u32)((key >> 32) & 0x00000000FFFFFFFF);
		const u32 cy = (u32)(key & 0x00000000FFFFFFFF);

		ChunkData *chunk = priv_chunk__get(cx, cy);
		assert (NULL != chunk);

		//copio in hmap le altezze del chunk
		u32 ct = 0;
		for (u32 y=0; y<chunk__num_vtx_lato; y++)
		{
			u32 ct_hmap = 1 + (y+1) * HMAP_NUM_VTX_PER_LATO;
			for (u32 x=0; x<chunk__num_vtx_lato; x++)
			{
				hmap[ct_hmap++] = 0.1f * (f32)chunk[ct++].height;
			}
		}

		//fillo i bordi di hmap con info relative ai chunk adiacenti
		{
			ChunkData *c2 = priv_chunk__get(cx, cy-1);
			if (NULL != c2)
			{
				u32 ct = chunk__num_vtx_lato * (chunk__num_vtx_lato-1);
				u32 ct_hmap = 1;
				for (u32 x=0; x<chunk__num_vtx_lato; x++)
					hmap[ct_hmap++] = 0.1f * (f32)c2[ct++].height;
			}

			c2 = priv_chunk__get(cx, cy+1);
			if (NULL != c2)
			{
				u32 ct = 0;
				u32 ct_hmap = 1 + HMAP_NUM_VTX_PER_LATO * (HMAP_NUM_VTX_PER_LATO-1);
				for (u32 x=0; x<chunk__num_vtx_lato; x++)
					hmap[ct_hmap++] = 0.1f * (f32)c2[ct++].height;
			}			

			c2 = priv_chunk__get(cx+1, cy);
			if (NULL != c2)
			{
				u32 ct = 0;
				u32 ct_hmap = 2 * HMAP_NUM_VTX_PER_LATO -1;
				for (u32 x=0; x<chunk__num_vtx_lato; x++)
				{
					hmap[ct_hmap] = 0.1f * (f32)c2[ct].height;
					ct_hmap += HMAP_NUM_VTX_PER_LATO;
					ct+=chunk__num_vtx_lato;
				}
			}

			c2 = priv_chunk__get(cx-1, cy);
			if (NULL != c2)
			{
				u32 ct = chunk__num_vtx_lato - 1;
				u32 ct_hmap = HMAP_NUM_VTX_PER_LATO;
				for (u32 x=0; x<chunk__num_vtx_lato; x++)
				{
					hmap[ct_hmap] = 0.1f * (f32)c2[ct].height;
					ct_hmap += HMAP_NUM_VTX_PER_LATO;
					ct+=chunk__num_vtx_lato;
				}
			}

			//i 4 pixel ai corner
			c2 = priv_chunk__get(cx-1, cy-1);
			if (NULL != c2)
			{
				u32 ct = chunk__num_vtx_lato * chunk__num_vtx_lato - 1;
				u32 ct_hmap = 0;
				hmap[ct_hmap] = 0.1f * (f32)c2[ct].height;
			}

			c2 = priv_chunk__get(cx+1, cy-1);
			if (NULL != c2)
			{
				u32 ct = chunk__num_vtx_lato * (chunk__num_vtx_lato -1);
				u32 ct_hmap = HMAP_NUM_VTX_PER_LATO -1;
				hmap[ct_hmap] = 0.1f * (f32)c2[ct].height;
			}

			c2 = priv_chunk__get(cx-1, cy+1);
			if (NULL != c2)
			{
				u32 ct = chunk__num_vtx_lato - 1;
				u32 ct_hmap = HMAP_NUM_VTX_PER_LATO * (HMAP_NUM_VTX_PER_LATO-1);
				hmap[ct_hmap] = 0.1f * (f32)c2[ct].height;
			}
			
			c2 = priv_chunk__get(cx+1, cy+1);
			if (NULL != c2)
			{
				u32 ct = 0;
				u32 ct_hmap = HMAP_NUM_VTX_PER_LATO * HMAP_NUM_VTX_PER_LATO -1;
				hmap[ct_hmap] = 0.1f * (f32)c2[ct].height;
			}				
		}

		//calcolo delle normali
		ct = 0;
		for (u32 y=1; y<HMAP_NUM_VTX_PER_LATO-1; y++)
		{
			u32 ct_hmap = 1+ y * HMAP_NUM_VTX_PER_LATO;
			u32 ct_hmap_su = ct_hmap - HMAP_NUM_VTX_PER_LATO;
			u32 ct_hmap_giu = ct_hmap + HMAP_NUM_VTX_PER_LATO;
			for (u32 x=1; x<HMAP_NUM_VTX_PER_LATO-1; x++)
			{
				const f32 h[3][3] {
					hmap[ct_hmap_su - 1], 	hmap[ct_hmap_su], 	hmap[ct_hmap_su + 1],
					hmap[ct_hmap - 1], 		hmap[ct_hmap], 		hmap[ct_hmap + 1],
					hmap[ct_hmap_giu - 1], 	hmap[ct_hmap_giu], 	hmap[ct_hmap_giu + 1]
				};

				const f32 x0 = 0;
				const f32 x1 = lod0__scala_xz__m;
				const f32 x2 = x1 + lod0__scala_xz__m;

				const f32 z2 = 0;
				const f32 z1 = lod0__scala_xz__m;
				const f32 z0 = z1 + lod0__scala_xz__m;

				vec3f nn[4];
				geom::Plane3 pl1;
				geom::Plane3 pl2;
				pl1.set_from_3points ( vec3f(x0, h[0][0], z0), vec3f(x1, h[1][0], z0), vec3f(x1, h[1][1], z1));
				pl2.set_from_3points ( vec3f(x0, h[0][0], z0), vec3f(x1, h[1][1], z1), vec3f(x0, h[0][1], z1));
				nn[0] = (pl1.n + pl2.n);

				pl1.set_from_3points ( vec3f(x1, h[1][0], z0), vec3f(x2, h[2][0], z0), vec3f(x2, h[2][1], z1));
				pl2.set_from_3points ( vec3f(x1, h[1][0], z0), vec3f(x2, h[2][1], z1), vec3f(x1, h[1][1], z1));
				//nn[1] = (pl1.n + pl2.n);
				nn[1] = pl2.n;

				pl1.set_from_3points ( vec3f(x0, h[0][1], z1), vec3f(x1, h[1][1], z1), vec3f(x1, h[1][2], z2));
				pl2.set_from_3points ( vec3f(x0, h[0][1], z1), vec3f(x1, h[1][2], z2), vec3f(x0, h[0][2], z2));
				//nn[2] = (pl1.n + pl2.n);
				nn[2] = pl1.n;

				pl1.set_from_3points ( vec3f(x1, h[1][1], z1), vec3f(x2, h[2][1], z1), vec3f(x2, h[2][2], z2));
				pl2.set_from_3points ( vec3f(x1, h[1][1], z1), vec3f(x2, h[2][2], z2), vec3f(x1, h[1][2], z2));
				nn[3] = (pl1.n + pl2.n);

				vec3f norm = (nn[0] + nn[1] +nn[2] +nn[3]) / 6.0f;
				norm.normalize();
				//chunk[ct].norm = norm;
				chunk[ct].encoded_norm = utils::normal_encode_octahedral(norm);

				//poor man AO
				{
					geom::Pos3 pos;
					pos.identity();
					pos.o.set (0, h[1][1], 0);
					pos.alignAsseY (norm);


					vec3f vIN[8];
					vIN[0].set (-lod0__scala_xz__m, h[0][0],  lod0__scala_xz__m);	
					vIN[1].set ( 0, 				h[1][0],  lod0__scala_xz__m);	
					vIN[2].set ( lod0__scala_xz__m, h[2][0],  lod0__scala_xz__m);
					
					vIN[3].set (-lod0__scala_xz__m, h[0][1],  0);									
					vIN[4].set ( lod0__scala_xz__m, h[2][1],  0);
					
					vIN[5].set (-lod0__scala_xz__m, h[0][2], -lod0__scala_xz__m);	
					vIN[6].set ( 0, 				h[1][2], -lod0__scala_xz__m);	
					vIN[7].set ( lod0__scala_xz__m, h[2][2], -lod0__scala_xz__m);

					vec3f vOUT[8];
					pos.vect_ToWorld (vIN, vOUT, 8);


					u8 ao = 0;					
					pl1.set_from_point_and_normal (vec3f(0, h[1][1], 0), norm);
					for (u8 i=0; i<8; i++)
					{
						if (pl1.signed_distance (vOUT[i]) > 0.2f)
							ao++;
					}

					//ao va da 0 a 8.  Se == 8 e' super occulso
					chunk[ct].ao = 31*ao;
				}

				ct_hmap++;
				ct_hmap_su++;
				ct_hmap_giu++;

				ct++;
			}
		}



		//altezza min/max
		u16 hmin = u16MAX;
		u16 hmax = 0;
		for (u32 i2=0; i2<NUM_VTX_IN_CHUNK; i2++)
		{
			if (chunk[i2].height < hmin)	hmin = chunk[i2].height;
			if (chunk[i2].height > hmax)	hmax = chunk[i2].height;
		}
		

		//salvo chunkinfo con le altezze min-max
		ChunkInfo *ci = priv__chunk_get_info(cx, cy);
		assert (NULL != ci);
		ci->min_height__m = 0.1f * (f32)hmin;
		ci->max_height__m = 0.1f * (f32)hmax;


		priv__save_chunk_data (path_to_folder, cx, cy, chunk, sizeof(ChunkData) * NUM_VTX_IN_CHUNK);
	}

	GOSFREE(gos::getScrapAllocator(), hmap);

	priv__save_chunk_info (path_to_folder, chunk_info, sizeof(ChunkInfo) * chunk__num * chunk__num);
}
