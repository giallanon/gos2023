#include "bigFile.h"
#include "gosImageBufferRGBA.h"
#include "gosGeomIntersect3D.h"


using namespace gos;
using namespace land;


//********************************
bool BigFile::create (const char *filename, u32 chunk_sizeIN, u32 num_chunk)
{
	gos::err::clear();

	gos::File f;
	if (!fs::fileOpenForW (&f, filename))
	{
		logger::err ("can't create file %s\n", filename);
		return false;
	}

	//ridimensiono il chunk size in modo che sia un multiplo di 64
	const u32 chunk_size = GOS_ALIGN_NUMBER_TO_POWER_OF_TWO(chunk_sizeIN, 64);

	//header
	{
		u8 buffer[256];
		memset (buffer, 0, sizeof(buffer));
		u32 ct = 0;
		
		ct += utils::bufferWriteU32 (&buffer[ct], BigFile::VERSION);
		ct += utils::bufferWriteU32 (&buffer[ct], chunk_size);
		ct += utils::bufferWriteU32 (&buffer[ct], num_chunk);
		ct += utils::bufferWriteU32 (&buffer[ct], SIZE_OF_HEADER);

		assert (ct <= SIZE_OF_HEADER);
		assert (ct <= sizeof(buffer));
		fs::fileWrite (f, buffer, SIZE_OF_HEADER);
	}

	//scrivo i chunk
	{
		u8 *buffer = GOSALLOCT(u8*, gos::getScrapAllocator(), chunk_size);
		memset (buffer, 0, chunk_size);
		for (u32 i=0; i<num_chunk; i++)
			fs::fileWrite (f, buffer, chunk_size);
		GOSFREE(gos::getScrapAllocator(), buffer);
	}

	fs::fileClose(f);

	//fine
	return !err::anyError();
}


//********************************
BigFile::BigFile()
{
	localAllocator = NULL;
}

//********************************
void BigFile::priv__free()
{
	if (NULL == localAllocator)
		return;

	fs::fileClose(hFile);

	if (NULL != cached_chunk_list)
		GOSFREE_AND_NULL(localAllocator, cached_chunk_list);

	if (NULL != cached_chunk_pt)
		GOSFREE_AND_NULL(localAllocator, cached_chunk_pt);
		

	localAllocator = NULL;
}

//********************************
bool BigFile::priv__open (gos::Allocator *allocator, const char *filename)
{
	priv__free();

	fs::extractFileNameWithExt (filename, debug_only_fname, sizeof(debug_only_fname));

	if (!fs::fileExists(filename))
	{
		logger::err ("BigFile => can't open %s\n", filename);
		return false;
	}
	if (!fs::fileOpenForRW (&hFile, filename))
	{
		logger::err ("BigFile => can't open %s\n", filename);
		return false;
	}


	//header
	GOS_DEBUG_ASSERT( fs::fileSeek (hFile, 0, eSeek::start) );
	{
		u8 buffer[SIZE_OF_HEADER];
		memset (buffer, 0, sizeof(buffer));
		fs::fileRead (hFile, buffer, SIZE_OF_HEADER);

		u32 ct = 0;
		u32 ver = utils::bufferReadU32 (&buffer[ct]);
		ct+=4;
		if (!magic::signatureMatch(ver, BigFile::VERSION) || !magic::versionMatch(ver, BigFile::VERSION))
		{
			logger::err ("Invalid magic or version [%s]\n", filename);
			fs::fileClose(hFile);
			return false;
		}
		chunk_size = utils::bufferReadU32 (&buffer[ct]);
		ct += 4;

		num_total_chunk_in_file = utils::bufferReadU32 (&buffer[ct]);
		ct += 4;

		start_of_chunk_data = utils::bufferReadU32 (&buffer[ct]);
		ct += 4;
	}

	localAllocator = allocator;
	num_cached_chunk = 0;
	return true;
}

//********************************
void BigFile::priv__alloc_cache (u32 num_max_cached_chunkIN)
{
	num_max_cached_chunk = num_max_cached_chunkIN;

	cached_chunk_list = GOSALLOCT(CachedChunk*, localAllocator, sizeof(CachedChunk) * num_max_cached_chunk);
	cached_chunk_pt = GOSALLOCT(u8*, localAllocator, chunk_size * num_max_cached_chunk);
	for (u32 i=0; i<num_max_cached_chunk; i++)
	{
		cached_chunk_list[i].lru = 0;
		cached_chunk_list[i].chunk_num = u32MAX;
		cached_chunk_list[i].p = &cached_chunk_pt[i * chunk_size];
	}
}

//********************************
bool BigFile::open_1 (gos::Allocator *allocator, const char *filename, u32 num_max_cached_chunkIN)
{
	if (!priv__open(allocator, filename))
		return false;

	priv__alloc_cache (num_max_cached_chunkIN);
	return true;
}

//********************************
bool BigFile::open_2 (gos::Allocator *allocator, const char *filename, u32 max_memory_for_cache)
{
	if (!priv__open(allocator, filename))
		return false;

	num_max_cached_chunk = max_memory_for_cache / chunk_size;
	if (num_max_cached_chunk * chunk_size < max_memory_for_cache)
		num_max_cached_chunk++;
	if (num_max_cached_chunk > num_total_chunk_in_file)
		num_max_cached_chunk = num_total_chunk_in_file;
	
	priv__alloc_cache (num_max_cached_chunk);
	return true;
}

//********************************
void BigFile::close()
{
	priv__free();
}

//********************************
void BigFile::priv__load_chunk (u32 chunk_num, u32 cache_index, u32 timenow_msec)
{
	assert (chunk_num < num_total_chunk_in_file);
	assert (cache_index < num_max_cached_chunk);
	CachedChunk *p = &cached_chunk_list[cache_index];
	
	p->lru = timenow_msec;
	p->chunk_num = chunk_num;

	GOS_DEBUG_ASSERT( fs::fileSeek (hFile, start_of_chunk_data + chunk_num * chunk_size, eSeek::start) );
	GOS_DEBUG_ASSERT( fs::fileRead (hFile, p->p, chunk_size) );


	logger::log_7 (eTextColor::cyan, "BigFile [%s] => load chunk %d\n", debug_only_fname, chunk_num);
}

//********************************
void BigFile::priv__save_chunk (u32 chunk_num, const void *src, u32 sizeof_chunk)
{
	assert (chunk_num < num_total_chunk_in_file);
	
	GOS_DEBUG_ASSERT( fs::fileSeek (hFile, start_of_chunk_data + chunk_num * chunk_size, eSeek::start) );
	GOS_DEBUG_ASSERT( fs::fileWrite (hFile, src, sizeof_chunk) );
}

//********************************
u32 BigFile::priv__is_already_cached (u32 chunk_num) const
{
	assert (chunk_num < num_total_chunk_in_file);
	for (u32 i=0; i<num_cached_chunk; i++)
	{
		if (cached_chunk_list[i].chunk_num == chunk_num)
			return i;
	}
	return u32MAX;
}

//********************************
const void* BigFile::get_chunk (u32 chunk_num)
{
	const u32 timenow_msec = (u32)gos::getTimeSinceStart_msec();
	u32 index = priv__is_already_cached(chunk_num);
	if (u32MAX != index)
	{
		cached_chunk_list[index].lru = timenow_msec;
		return cached_chunk_list[index].p;
	}

	if (num_cached_chunk < num_max_cached_chunk)
	{
		index = num_cached_chunk++;
		priv__load_chunk (chunk_num, index, timenow_msec);
		return cached_chunk_list[index].p;
	}

	//scanno alla ricerca di un chunk da discardare
	u32 worst_index = 0;
	u32 worst_lru = cached_chunk_list[0].lru;
	for (u32 i=1; i<num_cached_chunk; i++)
	{
		if (cached_chunk_list[i].lru < worst_lru)
		{
			worst_lru = cached_chunk_list[i].lru;
			worst_index = i;
		}
	}

	priv__load_chunk (chunk_num, worst_index, timenow_msec);
	return cached_chunk_list[worst_index].p;
}

//********************************
void BigFile::memcpy_chunk (u32 chunk_num, void *dest, u32 sizeof_dest)
{
	assert (NULL != dest);
	assert (sizeof_dest <= chunk_size);
	const void *p = get_chunk(chunk_num);
	memcpy (dest, p, sizeof_dest);
}

//********************************
void BigFile::write_chunk (u32 chunk_num, const void *src, u32 sizeof_chunk)
{
	assert (chunk_num < num_total_chunk_in_file);
	assert (NULL != src);
	assert (sizeof_chunk <= chunk_size);

	//se e' in cache, aggiorna anche quella
	u32 index = priv__is_already_cached(chunk_num);
	if (u32MAX != index)
	{
		cached_chunk_list[index].lru = (u32)gos::getTimeSinceStart_msec();
		memcpy (cached_chunk_list[index].p, src, sizeof_chunk);
	}

	//salvo su disco
	priv__save_chunk (chunk_num, src, sizeof_chunk);
}