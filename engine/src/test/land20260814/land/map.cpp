#include "map.h"
#include "land.h"
#include "gosImageBufferRGBA.h"
#include "gosGeomIntersect3D.h"


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

	//creo e salvo le mappe
	for (u32 mm=0; mm<num_map_info; mm++)
	{
		MapInfo *m = &mapInfo[mm];

		//cerco di creare mappe con chunk che siano grossi circa 8-10MB
		constexpr u32 MAX_CHUNK_SIZE_IN_BYTE = 10 * 1024 * 1024;
		u32 num_point_per_chunk_lato = 1024;
		u32 num_tot_point_per_chunk = 0;
		u32 sizeof_chunk = 0;
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

		sprintf_s (s, sizeof(s), "%s/lod%d", save_path, land::resolution_to_u8(m->resolution));
		{
			m->num_chunk_per_lato = m->num_point_per_lato / num_point_per_chunk_lato;
			const u32 num_tot_chunk = m->num_chunk_per_lato * m->num_chunk_per_lato;
			land::BigFile::create (s, sizeof_chunk, num_tot_chunk);


			PointData *chunk_data = GOSALLOCT(PointData*, localAllocator, sizeof_chunk);
				chunk_data->height.set (create.default_height__m);
				chunk_data->norm.set (vec3f(0,1,0));
				chunk_data->ao = 0;
				chunk_data->materialID = 0;


			land::BigFile bf;
			bf.open (localAllocator, s, 1);
			for (u32 i=0; i<num_tot_chunk; i++)
				bf.write_chunk (i, chunk_data, sizeof_chunk);
			bf.close();		

			GOSFREE_AND_NULL(localAllocator, chunk_data);
		}
	}


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
	const u32 num_max_cached_chunk = mapInfo[0].num_chunk_per_lato * mapInfo[0].num_chunk_per_lato;

	//per le altre mappe, tengo un cache del 25% del totale della mappa
	for (u32 mm=0; mm<num_mapInfo; mm++)
	{
		MapInfo *m = &mapInfo[mm];

		//chunk data
		sprintf_s (s, sizeof(s), "%s/lod%d", folder_path, land::resolution_to_u8(m->resolution));
		if (!m->chunkData->open (localAllocator, s, num_max_cached_chunk))
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
	return true;
}

