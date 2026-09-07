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
		mapInfo[num_map_info].num_point_per_row = create.default_map__border_size__point;
		num_map_info++;

		Resol res = mapInfo[0].resolution;
		while (res != create.resolution_min)
		{
			res = land::resolution_prev(res);
			mapInfo[num_map_info].resolution = res;
			mapInfo[num_map_info].num_point_per_row = mapInfo[num_map_info-1].num_point_per_row * 2;
			num_map_info++;
		}

		//info addizionali sulla mappa
		for (u32 i=0; i<num_map_info; i++)
		{
			mapInfo[i].border_size__m = (mapInfo[i].num_point_per_row - 1) * land::resolution_to_m(mapInfo[i].resolution);
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
			m->num_chunk_per_row = m->num_point_per_row / num_point_per_chunk_lato;
			m->chunk__num_point_per_row = num_point_per_chunk_lato;
			const u32 num_tot_chunk = m->num_chunk_per_row * m->num_chunk_per_row;
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
			ct += utils::bufferWriteU32 (&buffer[ct], mapInfo[mm].num_point_per_row);
			ct += utils::bufferWriteU32 (&buffer[ct], mapInfo[mm].num_chunk_per_row);
			ct += utils::bufferWriteU32 (&buffer[ct], mapInfo[mm].chunk__num_point_per_row);
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
			mapInfo[i].num_point_per_row = utils::bufferReadU32 (&buffer[ct]);
			ct += 4;

			mapInfo[i].num_chunk_per_row = utils::bufferReadU32 (&buffer[ct]);
			ct += 4;
			
			mapInfo[i].chunk__num_point_per_row = utils::bufferReadU32 (&buffer[ct]);
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
		const u32 num_max_cached_chunk = m->num_chunk_per_row * m->num_chunk_per_row;
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
	for (u32 i=0; i<mapInfo[0].num_chunk_per_row * mapInfo[0].num_chunk_per_row; i++)
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
	if (dimx > upd.mi->num_point_per_row)	dimx = upd.mi->num_point_per_row;
	if (dimy > upd.mi->num_point_per_row)	dimy = upd.mi->num_point_per_row;

	const u32 px = (upd.mi->num_point_per_row - dimx) / 2;
	const u32 py = (upd.mi->num_point_per_row - dimy) / 2;
	assert (px + dimx <= upd.mi->num_point_per_row);
	assert (py + dimy <= upd.mi->num_point_per_row);

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
void Map::priv__point_to_chunk (const MapInfo *mi, u32 px, u32 py, u32 *out_cx, u32 *out_cy) const
{
	assert (NULL != mi);
	assert (NULL != out_cx);
	assert (NULL != out_cy);
	*out_cx = px / mi->chunk__num_point_per_row;
	*out_cy = py / mi->chunk__num_point_per_row;
}

//********************************
void Map::priv__chunk_to_point (const MapInfo *mi, u32 cx, u32 cy, u32 *out_px, u32 *out_py) const
{
	assert (NULL != out_px);
	assert (NULL != out_py);
	*out_px = priv__chunk_to_point (mi, cx);
	*out_py = priv__chunk_to_point (mi, cy);
}

//********************************
u32 Map::priv__chunk_to_point (const MapInfo *mi, u32 cx_or_cy) const
{
	assert (NULL != mi);
	return cx_or_cy * mi->chunk__num_point_per_row;
}

/********************************
 * In <out> copio tutti i punti del quadrato definito da (px,py) - (px+num_point_per_latoIN-1, py+num_point_per_latoIN-1)
 */
bool Map::priv__map_get_data (u32 px, u32 py, MapInfo *mi, u32 num_point_per_latoIN, PointData *out, u32 sizeof_out)
{
	assert (NULL != mi);
	assert (NULL != out);
	assert (num_point_per_latoIN > 0);

	const u32 x1 = px;
	const u32 y1 = py;
	if (x1 >= mi->num_point_per_row || y1 >= mi->num_point_per_row)
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
	u32 cx1, cy1;
	priv__point_to_chunk (mi, x1, y1, &cx1, &cy1);

	u32 cx2, cy2;
	const u32 x2 = x1 + num_point_per_latoIN -1;
	const u32 y2 = y1 + num_point_per_latoIN -1;
	priv__point_to_chunk (mi, x2, y2, &cx2, &cy2);
	if (cx2 >= mi->num_chunk_per_row)
		cx2 = mi->num_chunk_per_row -1;
	if (cy2 >= mi->num_chunk_per_row)
		cy2 = mi->num_chunk_per_row -1;

	//i 4 chunk ai bordi del quadrato probabilmente non sono da copiare interamente in out
	gos::Array2D dst;
	dst.set (num_point_per_latoIN, num_point_per_latoIN, sizeof(PointData));
	u32 dstY = 0;

	for (u32 cy=cy1; cy<=cy2; cy++)
	{
		//il chunk a coordinata <cy> copre i punti 
		const u32 orig_py_top = priv__chunk_to_point (mi, cy);
		
		u32 py_top = orig_py_top;
		if (py_top < y1) 	py_top = y1;
		
		u32 py_bottom = orig_py_top + mi->chunk__num_point_per_row -1;
		if (py_bottom > y2) py_bottom = y2;
		
		const u32 dimy = (py_bottom - py_top) +1;
		assert (dimy > 0);
		assert (dimy <= num_point_per_latoIN);

		py_top -= orig_py_top;
		py_bottom -= orig_py_top;

		u32 dstX = 0;
		for (u32 cx=cx1; cx<=cx2; cx++)
		{
			const u32 orig_px_left = priv__chunk_to_point (mi, cx);			
			
			u32 px_left = orig_px_left;
			if (px_left < x1) 	px_left = x1;
			
			u32 px_right = orig_px_left + mi->chunk__num_point_per_row -1;
			if (px_right > x2) 	px_right = x2;

			const u32 dimx = (px_right - px_left) +1;
			assert (dimx > 0);
			assert (dimx <= num_point_per_latoIN);
			
			px_left -= orig_px_left;
			px_right -= orig_px_left;

			gos::Array2D src;
			src.set (mi->chunk__num_point_per_row, mi->chunk__num_point_per_row, sizeof(PointData));

			const PointData *psrc = (const PointData*) mi->chunkData->get_chunk(cx + cy * mi->num_chunk_per_row);
			array2DUtils_copy (psrc, src, px_left, py_top, dimx, dimy, 
							   out, dst, dstX, dstY);
			dstX += dimx;
		}

		if (dstX < num_point_per_latoIN)
		{
			u32 ct = dstX + dstY * num_point_per_latoIN;
			while (dstX < num_point_per_latoIN)
			{
				out[ct].set_default();
				ct++;
				dstX++;
			}
		}

		dstY += dimy;
	}

	if (dstY < num_point_per_latoIN)
	{
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
	
	dst->resolution = resolution;

	const u32 mapIndex = priv__from_resol_to_mapInfoIndex(resolution);
	assert (u32MAX != mapIndex);
	dst->mi = &mapInfo[mapIndex];
	
	dst->updated_chunk_list = list;
	dst->updated_chunk_list->reset();
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
	if (px >= upd->mi->num_point_per_row || py >= upd->mi->num_point_per_row)
	{
		DBGBREAK;
		return;
	}

	u32 cx, cy;
	priv__point_to_chunk (upd->mi, px, py, &cx, &cy);
	upd->updated_chunk_list->insertIfNotExists (ChunkCoord(cx, cy));

	const u32 orig_px_left = priv__chunk_to_point (upd->mi, cx);
	const u32 orig_py_top = priv__chunk_to_point (upd->mi, cy);
	px -= orig_px_left;
	py -= orig_py_top;

	PointData *p = static_cast<PointData*>( upd->mi->chunkData->get_chunk_for_update (cx + cy * upd->mi->num_chunk_per_row) );
	const u32 offset = px + py * upd->mi->chunk__num_point_per_row;
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


	MapInfo *mi = upd->mi;
	const u32 mi_lod = priv__from_resol_to_mapInfoIndex (mi->resolution);
	const u32 sizeof_chunk_data = sizeof(PointData) * mi->chunk__num_point_per_row * mi->chunk__num_point_per_row;
	PointData *chunk_data = GOSALLOCT(PointData*, gos::getScrapAllocator(), sizeof_chunk_data);


	const FastArray<ChunkCoord> *ccList = upd->updated_chunk_list->_queryList();
	for (u32 chunk=0; chunk<ccList->getNElem(); chunk++)
	{
		ChunkCoord cc = ccList->queryElem(chunk);

		//di questo chunk devo calcolare le normali e AO
		const u32 cxSRC = cc.get_cx();
		const u32 cySRC = cc.get_cy();
		const u32 px = priv__chunk_to_point (mi, cxSRC);
		const u32 py = priv__chunk_to_point (mi, cySRC);
		GOS_DEBUG_ASSERT( map__get_data (px, py, mi->resolution, mi->chunk__num_point_per_row, chunk_data, sizeof_chunk_data) );

		//aggiorno i LOD a risoluzione piu' dettagliata
		for (u32 lod=mi_lod+1; lod < num_mapInfo; lod++)
		{
		}
	}

	//fine
	upd->mi = NULL;
}

//********************************
void Map::priv__map_update_nextres_chunk (const MapInfo *miSRC, u32 cxSRC, u32 cySRC)
{
	assert (NULL != miSRC);
	assert (cxSRC < miSRC->num_chunk_per_row);
	assert (cySRC < miSRC->num_chunk_per_row);

	u32 lodSRC = priv__from_resol_to_mapInfoIndex(miSRC->resolution);
	if (0 == lodSRC)
		return;

	MapInfo *miDST = &mapInfo[lodSRC-1];
	const u32 cxDST = cxSRC << 1;
	const u32 cyDST = cySRC << 1;
	for (u32 ccy=0; ccy<2; ccy++)
	{
		for (u32 ccx=0; ccx<2; ccx++)
		{
		}
	}
}