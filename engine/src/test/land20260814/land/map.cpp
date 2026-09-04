#include "map.h"
#include "land.h"
#include "gosImageBufferRGBA.h"
#include "gosGeomIntersect3D.h"
#include "Array2DUtils.h"


using namespace gos;
using namespace land;

typedef gos::AllocatorHeap<gos::AllocPolicy_Track_simple, gos::AllocPolicy_Thread_Unsafe>	LandMapMemAllocator;

//********************************
bool Map::create (const char *save_path, const CreateData &create)
{
	assert (GOS_IS_POWER_OF_TWO(create.default_map__border_size__point));
	assert (create.default_map__resolution > create.resolution_min);
	
	char s[1024];
	gos::Allocator *localAllocator = gos::getSysHeapAllocator();
	gos::err::clear();


	//se esiste gia' un folder in <save_path>, elimino tutto e poi lo ricreo
	fs::folderDeleteAllFileRecursively (save_path, eFolderDeleteMode::deleteAlsoTheSubfolder);
	if (!fs::folderCreate (save_path))
	{
		logger::err ("Unable to create folder %s\n", save_path);
		return false;
	}



	//voglio creare una mappa quadrata di <map_border_size_point> x <map_border_size_point>
	//a risoluzione <default_resolution>.
	//Questa e' la mappa di default, perennemente storata in RAM
	//Sotto a questa mappa, ne esistono altre + grandi a risoluzione + dettaglita che vengono cachate alla bisogna
	MapInfo mapInfo[32];
	u32 num_map_info = 0;
	{
		mapInfo[num_map_info].resolution = create.default_map__resolution;
		mapInfo[num_map_info].num_point_per_lato = create.default_map__border_size__point;
		num_map_info++;

		Resol res = mapInfo[0].resolution;
		while (res != create.resolution_min)
		{
			res = land::resolution_prev(res);
			mapInfo[num_map_info].resolution = res;
			mapInfo[num_map_info].num_point_per_lato = mapInfo[num_map_info-1].num_point_per_lato * 2;
			num_map_info++;
		}

		//info addizionali sulla mappa
		for (u32 i=0; i<num_map_info; i++)
		{
			mapInfo[i].border_size__m = (mapInfo[i].num_point_per_lato - 1) * land::resolution_to_m(mapInfo[i].resolution);
		}
	}


	//cerco di creare mappe con chunk che siano grossi circa 8-10MB
	constexpr u32 MAX_CHUNK_SIZE_IN_BYTE = 10 * 1024 * 1024;
	PointData *chunk_data = NULL;
	u32 num_point_per_chunk_lato = 1024;
	u32 sizeof_chunk = 0;
	{
		u32 num_tot_point_per_chunk = 0;
		while (1)
		{
			num_tot_point_per_chunk = num_point_per_chunk_lato * num_point_per_chunk_lato;
			sizeof_chunk = num_tot_point_per_chunk * sizeof(PointData);
			if (sizeof_chunk <= MAX_CHUNK_SIZE_IN_BYTE)
				break;
			if (num_point_per_chunk_lato <= 8)
				break;
			num_point_per_chunk_lato >>= 1;
		}

		chunk_data = GOSALLOCT(PointData*, localAllocator, sizeof_chunk);
		for (u32 i=0; i<num_tot_point_per_chunk; i++)
		{
			chunk_data[i].height.set (create.default_height__m);
			chunk_data[i].norm.set (vec3f(0,1,0));
			chunk_data[i].ao = 0;
			chunk_data[i].materialID = 0;
		}
	}

	//creo e salvo le mappe
	for (u32 mm=0; mm<num_map_info; mm++)
	{
		MapInfo *m = &mapInfo[mm];

		sprintf_s (s, sizeof(s), "%s/lod%d", save_path, land::resolution_to_u8(m->resolution));
		{
			m->num_chunk_per_lato = m->num_point_per_lato / num_point_per_chunk_lato;
			const u32 num_tot_chunk = m->num_chunk_per_lato * m->num_chunk_per_lato;
			land::BigFile::create (s, sizeof_chunk, num_tot_chunk);

			land::BigFile bf;
			bf.open_1 (localAllocator, s, 1);
			for (u32 i=0; i<num_tot_chunk; i++)
				bf.update_whole_chunk (i, chunk_data, sizeof_chunk);
			bf.close();		
		}
	}
	GOSFREE_AND_NULL(localAllocator, chunk_data);

	//creo il file .map con le info sulla mappa generata
	{
		sprintf_s (s, sizeof(s), "%s/map", save_path);
		gos::File f;
		if (!fs::fileOpenForW (&f, s))
		{
			logger::err ("Unable to create file %s\n", s);
			return false;
		}

		u8 buffer[512];
		u32 ct = 0;

		ct += utils::bufferWriteU32 (&buffer[ct], Map::VERSION);
		ct += utils::bufferWriteU32 (&buffer[ct], num_map_info);

		for (u32 mm=0; mm<num_map_info; mm++)
		{
			ct += utils::bufferWriteU32 (&buffer[ct], mapInfo[mm].num_point_per_lato);
			ct += utils::bufferWriteU32 (&buffer[ct], mapInfo[mm].num_chunk_per_lato);
			ct += utils::bufferWriteF32 (&buffer[ct], mapInfo[mm].border_size__m);
			ct += utils::bufferWriteU8 (&buffer[ct], (u8)mapInfo[mm].resolution);
		}

		assert (ct <= sizeof(buffer));
		fs::fileWrite (f, buffer, ct);
		fs::fileClose(f);
	}
	
	

	//fine
	return !err::anyError();
}


//********************************
Map::Map()
{
	LandMapMemAllocator *myAllocator = GOSNEW(gos::getSysHeapAllocator(), LandMapMemAllocator)("LandMap");
	myAllocator->setup (1024 * 1024 * 128); //128MB
	this->localAllocator = myAllocator;

	mapInfo = NULL;
	num_mapInfo = 0;
	
	ccList.setup (localAllocator, 256);
	upd.mi = NULL;
	upd.updated_chunk_list = &ccList;
}

//********************************
Map::~Map()
{ 
	priv__free();
	ccList.unsetup();

	GOSDELETE(gos::getSysHeapAllocator(), localAllocator);
	localAllocator = NULL;
}

//********************************
void Map::priv__free()
{
	if (NULL == mapInfo)
		return;
	for (u32 mm=0; mm<num_mapInfo; mm++)
	{
		if (NULL != mapInfo[mm].chunkData)
		{
			mapInfo[mm].chunkData->close();
			GOSDELETE(localAllocator, mapInfo[mm].chunkData);
		}
	}

	GOSFREE_AND_NULL(localAllocator, mapInfo);
	num_mapInfo = 0;
	qtree.unsetup();
}

//********************************
u32 Map::priv__from_resol_to_mapInfoIndex (land::Resol res) const
{
	for (u32 i = 0; i < num_mapInfo; i++)
	{
		if (mapInfo[i].resolution == res)
			return i;
	}
	return u32MAX;
}

//********************************
bool Map::open (const char *folder_path)
{
	priv__free();

	char s[1024];

	//header
	sprintf_s (s, sizeof(s), "%s/map", folder_path);
	{
		u32 fsize;
		u8 *buffer = fs::fileLoadInMemory (gos::getScrapAllocator(), s, &fsize);
		if (NULL == buffer)
		{
			logger::err ("Map => can't open %s\n", s);
			return false;
		}

		u32 ct = 0;
		u32 ver = utils::bufferReadU32 (&buffer[ct]);
		ct+=4;
		if (!magic::signatureMatch(ver, Map::VERSION) || !magic::versionMatch(ver, Map::VERSION))
		{
			GOSFREE(gos::getScrapAllocator(), buffer);
			logger::err ("Map => Invalid magic or version [%s]\n", s);
			return false;
		}


		num_mapInfo = utils::bufferReadU32 (&buffer[ct]);
		ct+=4;

		mapInfo = GOSALLOCT(MapInfo*, localAllocator, sizeof(MapInfo) * num_mapInfo);
		for (u32 i=0; i<num_mapInfo; i++)
		{
			mapInfo[i].num_point_per_lato = utils::bufferReadU32 (&buffer[ct]);
			ct += 4;

			mapInfo[i].num_chunk_per_lato = utils::bufferReadU32 (&buffer[ct]);
			ct += 4;
			
			mapInfo[i].border_size__m = utils::bufferReadF32 (&buffer[ct]);
			ct += 4;

			mapInfo[i].resolution = (land::Resol)buffer[ct++];
			
			mapInfo[i].chunkData = GOSNEW(localAllocator, BigFile)();
		}

		GOSFREE(gos::getScrapAllocator(), buffer);
	}

	//centro la mappa
	map_border_size__m = mapInfo[0].border_size__m;
	map_topLeft_WC.set (-map_border_size__m * 0.5f, map_border_size__m * 0.5f);

	//la mappa 0 la voglio sempre tutta in RAM, quindi apro il bigfile dandogli una cache suff a caricare tutta la
	//mappa in RAM. Le altre mappe usando la stessa quantita' di cache
	{
		MapInfo *m = &mapInfo[0];
		const u32 num_max_cached_chunk = m->num_chunk_per_lato * m->num_chunk_per_lato;
		sprintf_s (s, sizeof(s), "%s/lod%d", folder_path, land::resolution_to_u8(m->resolution));
		if (!m->chunkData->open_1 (localAllocator, s, num_max_cached_chunk))
		{
			logger::err ("Map => can't open chunk data [%s]\n", s);
			return false;
		}
	}

	//per le altre mappe, tengo un cache di 128MB che sembra essere un buon numero
	constexpr u32 CACHE_SIZE = 128 * 1024 * 1024;
	for (u32 mm=1; mm<num_mapInfo; mm++)
	{
		MapInfo *m = &mapInfo[mm];

		//chunk data
		sprintf_s (s, sizeof(s), "%s/lod%d", folder_path, land::resolution_to_u8(m->resolution));
		if (!m->chunkData->open_2 (localAllocator, s, CACHE_SIZE))
		{
			logger::err ("Map => can't open chunk data [%s]\n", s);
			return false;
		}
	}


	//debug info
	logger::log ("MAP DEBUG INFO\n");
	logger::inc_indent();

	logger::log ("map border size: %.3fm\n", map_border_size__m);
	for (u32 mm=0; mm<num_mapInfo; mm++)
	{
		MapInfo *m = &mapInfo[mm];

		string::format::memoryToKB_MB_GB (m->chunkData->get_sizeof_cache(), s, sizeof(s));
		logger::log ("size of cache for resolution %.3f = %s\n", land::resolution_to_m(m->resolution), s);
	}
	logger::dec_indent();


	//carico tutti i chunk della mappa0
	for (u32 i=0; i<mapInfo[0].num_chunk_per_lato * mapInfo[0].num_chunk_per_lato; i++)
		mapInfo[0].chunkData->get_chunk(i);


	//istanzio il QTREE
	qtree.setup (localAllocator, this, QTREE__NUM_VTX_PER_CHUNK_SIDE);
	return true;
}

//********************************
void Map::apply_heightmap (const char *filename, land::Resol resol, f32 scaleY__m)
{
	if (!map__begin_update(resol))
	{
		DBGBREAK;
		return;
	}

	image::BufferRGBA image;
	if (!image.loadFromFile (gos::getScrapAllocator(), filename))
	{
		DBGBREAK;
		return;
	}

	u32 dimx = image.getW();
	u32 dimy = image.getH();
	if (dimx > upd.mi->num_point_per_lato)	dimx = upd.mi->num_point_per_lato;
	if (dimy > upd.mi->num_point_per_lato)	dimy = upd.mi->num_point_per_lato;

	const u32 px = (upd.mi->num_point_per_lato - dimx) / 2;
	const u32 py = (upd.mi->num_point_per_lato - dimy) / 2;
	assert (px + dimx <= upd.mi->num_point_per_lato);
	assert (py + dimy <= upd.mi->num_point_per_lato);

	const u8 *rgba = image.getBuffer();
	const u32 rgba_size_of_a_row = image.getW() * 4;
	for (u32 y = 0; y < dimy; y++)
	{
		u32 rgba_ct = y * rgba_size_of_a_row;
		for (u32 x = 0; x < dimx; x++)
		{
			const f32 h = scaleY__m * (f32)rgba[rgba_ct];
			rgba_ct += 4;

			map__update (px + x, py + y, h);
		}
	}
	image.free (gos::getScrapAllocator());
	map__end_update();
}

//********************************
bool Map::map__get_data (const QTreeCoord cc, PointData *out, u32 sizeof_out)
{
	assert (cc.get_lod() < num_mapInfo);

	const u32 cx = cc.get_cx();
	const u32 cy = cc.get_cy();
	MapInfo *mi = &mapInfo[cc.get_lod()];

	const u32 px = cx * (QTREE__NUM_VTX_PER_CHUNK_SIDE-1);
	const u32 py = cy * (QTREE__NUM_VTX_PER_CHUNK_SIDE-1);

	return priv__map_get_data (px, py, mi, QTREE__NUM_VTX_PER_CHUNK_SIDE, out, sizeof_out);
}

//********************************
bool Map::map__get_data (u32 px, u32 py, land::Resol resolution, u32 num_point_per_latoIN, PointData *out, u32 sizeof_out)
{
	for (u32 i=0; i<num_mapInfo; i++)
	{
		if (mapInfo[i].resolution == resolution)
		{
			return priv__map_get_data (px, py, &mapInfo[i], num_point_per_latoIN, out, sizeof_out);
		}
	}

	logger::err ("Map::get_map_data() => resolution [%.3f] is not supported\n", land::resolution_to_m(resolution));
	return false;
}

//********************************
bool Map::priv__map_get_data (u32 px, u32 py, MapInfo *mi, u32 num_point_per_latoIN, PointData *out, u32 sizeof_out)
{
	assert (NULL != mi);
	assert (NULL != out);
	assert (num_point_per_latoIN > 0);

	const u32 x1 = px;
	const u32 y1 = py;
	if (x1 >= mi->num_point_per_lato || y1 >= mi->num_point_per_lato)
	{
		logger::err ("Map::get_map_data() => invalid coordinate or size:  px(%d,%d)  size(%d,%d)\n", px, py, num_point_per_latoIN, num_point_per_latoIN);
		return false;
	}

	const u32 size_needed = sizeof(PointData) * num_point_per_latoIN * num_point_per_latoIN;
	if (sizeof_out < size_needed)
	{
		logger::err ("Map::get_map_data() => out is not big enough!\n");
		return false;
	}

	//la mappa <mi> e' divisa in chunk.
	//Devo determinare quali chunk mi servono per fillare <out>
	const u32 chunk__num_point_per_lato = mi->num_point_per_lato / mi->num_chunk_per_lato;
	const u32 cx1 = x1 / chunk__num_point_per_lato;
	const u32 cy1 = y1 / chunk__num_point_per_lato;

	const u32 x2 = px + num_point_per_latoIN -1;
	u32 cx2 = x2 / chunk__num_point_per_lato;
	if (cx2 >= mi->num_chunk_per_lato)
		cx2 = mi->num_chunk_per_lato -1;

	const u32 y2 = py + num_point_per_latoIN -1;
	u32 cy2 = y2 / chunk__num_point_per_lato;
	if (cy2 >= mi->num_chunk_per_lato)
		cy2 = mi->num_chunk_per_lato -1;

	//i 4 chunk ai bordi del quadrato probabilmente non sono da copiare interamente in out
	gos::Array2D dst;
	dst.set (num_point_per_latoIN, num_point_per_latoIN, sizeof(PointData));
	u32 dstY = 0;

	for (u32 cy=cy1; cy<=cy2; cy++)
	{
		//il chunk a coordinata <cy> copre i punti 
		const u32 orig_py_top = cy * chunk__num_point_per_lato;
		
		u32 py_top = orig_py_top;
		if (py_top < y1) 	py_top = y1;
		
		u32 py_bottom = orig_py_top + chunk__num_point_per_lato -1;
		if (py_bottom > y2) py_bottom = y2;
		
		const u32 dimy = (py_bottom - py_top) +1;
		assert (dimy > 0);
		assert (dimy <= num_point_per_latoIN);

		py_top -= orig_py_top;
		py_bottom -= orig_py_top;

		u32 dstX = 0;
		for (u32 cx=cx1; cx<=cx2; cx++)
		{
			const u32 orig_px_left = cx * chunk__num_point_per_lato;
			
			u32 px_left = orig_px_left;
			if (px_left < x1) 	px_left = x1;
			
			u32 px_right = orig_px_left + chunk__num_point_per_lato -1;
			if (px_right > x2) 	px_right = x2;

			const u32 dimx = (px_right - px_left) +1;
			assert (dimx > 0);
			assert (dimx <= num_point_per_latoIN);
			
			px_left -= orig_px_left;
			px_right -= orig_px_left;

			gos::Array2D src;
			src.set (chunk__num_point_per_lato, chunk__num_point_per_lato, sizeof(PointData));

			const PointData *psrc = (const PointData*) mi->chunkData->get_chunk(cx + cy * mi->num_chunk_per_lato);
			array2DUtils_copy (psrc, src, px_left, py_top, dimx, dimy, 
							   out, dst, dstX, dstY);
			dstX += dimx;
		}

		dstY += dimy;
	}


	return true;

}

//********************************
bool Map::map__begin_update (land::Resol resolution)
{ 
	if (NULL != upd.mi)
	{
		DBGBREAK;
		return false;
	}

	const u32 lod = priv__from_resol_to_mapInfoIndex(resolution);
	if (u32MAX == lod)
	{
		DBGBREAK;
		return false;
	}

	priv__setup_updateInfo (&upd, resolution, &ccList);
	return priv__map_begin_update (&upd); 
}

//********************************
void Map::priv__setup_updateInfo (UpdateInfo *dst, land::Resol resolution, CCList *list) const
{
	assert (NULL != dst);
	assert (NULL == dst->mi);
	
	dst->resolution = resolution;

	const u32 mapIndex = priv__from_resol_to_mapInfoIndex(resolution);
	assert (u32MAX != mapIndex);
	dst->mi = &mapInfo[mapIndex];
	
	dst->updated_chunk_list = list;
	dst->updated_chunk_list->reset();
	
	dst->chunk__num_point_per_lato = dst->mi->num_point_per_lato / dst->mi->num_chunk_per_lato;
}

//********************************
bool Map::priv__map_begin_update (UpdateInfo *upd)
{
	assert (NULL != upd->mi);
	assert (upd->mi == &mapInfo[priv__from_resol_to_mapInfoIndex(upd->resolution)]);
	logger::log ("======= MAP begin update resol=%.3f =======\n", land::resolution_to_m(upd->resolution));
	return true;
}

//********************************
void Map::priv__map_update (UpdateInfo *upd, u32 px, u32 py, f32 height__m)
{
	assert (NULL != upd->mi);
	if (px >= upd->mi->num_point_per_lato || py >= upd->mi->num_point_per_lato)
	{
		DBGBREAK;
		return;
	}

	const u32 cx = px / upd->chunk__num_point_per_lato;
	const u32 cy = py / upd->chunk__num_point_per_lato;
	upd->updated_chunk_list->insertIfNotExists (ChunkCoord(cx, cy));

	const u32 orig_px_left = cx * upd->chunk__num_point_per_lato;
	const u32 orig_py_top = cy * upd->chunk__num_point_per_lato;
	px -= orig_px_left;
	py -= orig_py_top;

	PointData *p = static_cast<PointData*>( upd->mi->chunkData->get_chunk_for_update (cx + cy * upd->mi->num_chunk_per_lato) );
	const u32 offset = px + py * upd->chunk__num_point_per_lato;
	p[offset].height.set (height__m);
}

//********************************
void Map::priv__map_end_update(UpdateInfo *upd, bool bPropagaPrevResolution, bool bPropagaNextResolution)
{
	if (NULL == upd->mi)
	{
		DBGBREAK;
		return;
	}
	upd->mi->chunkData->save_all_updated_chunk();

	CCList ccList (gos::getScrapAllocator(), 1024);
	const u32 lod = priv__from_resol_to_mapInfoIndex(upd->resolution);

	//propago verso LOD a risoluzione maggiore
	if (bPropagaPrevResolution && lod < num_mapInfo-1)
	{
		const land::Resol r = land::resolution_prev(upd->resolution);
		if (r != upd->resolution)
		{
			UpdateInfo upd2;
			priv__setup_updateInfo (&upd2, r, &ccList);
			GOS_DEBUG_ASSERT(priv__map_begin_update(&upd2));
			priv__map_update_propagate_down (*upd, upd2);
			priv__map_end_update(&upd2, true, false);
		}
	}

	//propago verso a LOD a risoluzione inferiore
	if (bPropagaNextResolution)
	{
		const land::Resol r = land::resolution_next(upd->resolution);
		if (u32MAX != priv__from_resol_to_mapInfoIndex(r))
		{
			UpdateInfo upd2;
			priv__setup_updateInfo (&upd2, r, &ccList);
			GOS_DEBUG_ASSERT(priv__map_begin_update(&upd2));
			//priv__map_update_propagate_up (*upd, upd2);
			priv__map_end_update(&upd2, false, true);
		}
	}

	//fine
	upd->mi = NULL;
}

//********************************
void Map::priv__map_update_propagate_down (const UpdateInfo &src, UpdateInfo &dst)
{
	assert (NULL != src.mi);
	assert (NULL != src.updated_chunk_list);
	assert (NULL != dst.mi);
	assert (NULL != dst.updated_chunk_list);
	assert (0 == dst.updated_chunk_list->getNElem());

	assert (land::resolution_prev(src.resolution) == dst.resolution);

	//per ogni chunk modificato in src, devo lavorare 4 chunk di dst
	const FastArray<ChunkCoord> *ccListSRC = src.updated_chunk_list->_queryList();
	for (u32 i = 0; i < ccListSRC->getNElem(); i++)
	{
		const ChunkCoord ccSRC = ccListSRC->queryElem(i);
		const u32 cxSRC = ccSRC.get_cx();
		const u32 cySRC = ccSRC.get_cy();

		const u32 cxDST = ccSRC.get_cx() * 2;
		const u32 cyDST = ccSRC.get_cy() * 2;
		//dato che sto andando in un LOD a piu' alta risoluzione, ad ogni cc SRC corrispondono 4 cc DST
		dst.updated_chunk_list->insertIfNotExists (ChunkCoord(cxDST,    cyDST));
		dst.updated_chunk_list->insertIfNotExists (ChunkCoord(cxDST +1, cyDST));
		dst.updated_chunk_list->insertIfNotExists (ChunkCoord(cxDST,    cyDST +1));
		dst.updated_chunk_list->insertIfNotExists (ChunkCoord(cxDST +1, cyDST +1));

		const u32 px = cxDST * dst.chunk__num_point_per_lato;
		const u32 py = cyDST * dst.chunk__num_point_per_lato;
		const PointData *psrc = (const PointData*) src.mi->chunkData->get_chunk(cxSRC + cySRC * src.mi->num_chunk_per_lato);
		u32 ctSRC = 0;
		for (u32 y = 0; y < src.chunk__num_point_per_lato; y++)
		{
			for (u32 x = 0; x < src.chunk__num_point_per_lato; x++)
			{
				const f32 height__m = psrc[ctSRC].height.decode();
				priv__map_update (&dst, px+x, py+y, height__m);
				priv__map_update (&dst, px+x+1, py+y, height__m);
				priv__map_update (&dst, px+x, py +y+1, height__m);
				priv__map_update (&dst, px+x+1, py+y+1, height__m);
			}
		}
	}
}