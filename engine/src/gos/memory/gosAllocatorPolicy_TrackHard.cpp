#include "gosAllocatorPolicy_Track.h"
#include "gosAllocator.h"
#include "../gosHashMap.h"
#include "../gosUtils.h"
#include "../gosString.h"

/***
 * 
 * 
 */
class DebugAllocator : public gos::Allocator
{
public:
	DebugAllocator() : gos::Allocator(NULL)			{ }
	~DebugAllocator()								{ }
	bool		isThreadSafe() const				{ return false; }
	size_t		getAllocatedSize (const void *p) 	{ return 0; }

protected:
	void*		virt_do_alloc (size_t sizeInBytes, UNUSED_PARAM(u8 alignPowerOfTwo), UNUSED_PARAM(const char *debug_filename))			{ return malloc(sizeInBytes); }
	void		virt_do_dealloc (void *p)																								{ free(p); }
};


static DebugAllocator								*debug_allocator = NULL;
static gos::FastHashMap<u32, const char*>			*hashof_filename = NULL;
static gos::FastHashMap<const void*, const char*>	*hashof_allocation = NULL;


using namespace gos;

//***********************************
AllocPolicy_Track_hard::AllocPolicy_Track_hard (const char *nameIN)
{
	allocatorName = nameIN;
	nCurAlloc = 0;
	nTotAlloc = 0;
	curMemalloc = 0;
	maxMemalloc = 0;
	if (NULL == debug_allocator)
	{
		debug_allocator = new DebugAllocator();
		hashof_filename = new gos::FastHashMap<u32, const char*>();
		hashof_filename->setup (debug_allocator, 1024);

		hashof_allocation = new gos::FastHashMap<const void*, const char*>();
		hashof_allocation->setup (debug_allocator, 1024);
	}
}


//***********************************
void AllocPolicy_Track_hard::onAlloc (const void *p, size_t size, const char *debug_filename)
{
	++nCurAlloc;
	++nTotAlloc;
	curMemalloc += size;
	if (curMemalloc >= maxMemalloc)
		maxMemalloc = curMemalloc;

	//cerco <debug_filename> e, se non esiste, lo creo nuovo
	const u32 len = (u32) strlen(debug_filename);
	const u32 key = utils::crc32 (debug_filename, len);

	const char *out;
	gos::FastHashMap<u32, const char*>::Position pos;
	if (!hashof_filename->findWithPos (key, &out, &pos))
	{
		out = string::utf8::allocStr (debug_allocator, debug_filename, len+1);
		hashof_filename->insertInPosition (pos, out);	}


	hashof_allocation->insertIfNotExists (p, out);
}

//***********************************
void AllocPolicy_Track_hard::onDealloc (const void *p, size_t size)
{
	assert (nCurAlloc>0 && curMemalloc >= size);
	--nCurAlloc;
	curMemalloc -= size;

	hashof_allocation->remove (p);
}

//***********************************
void AllocPolicy_Track_hard::printReport (u64 totMemoryReserved)
{
	char sTotAllocated[16];
	gos::string::format::memoryToKB_MB_GB (totMemoryReserved, sTotAllocated, sizeof(sTotAllocated));

	char sMaxUsed[16];
	gos::string::format::memoryToKB_MB_GB (maxMemalloc, sMaxUsed, sizeof(sMaxUsed));

	gos::logger::log (eTextColor::blue, "AllocatorTrackHard: final report for [%s] => max mem allocated: %s/%s, num tot allocation: %d\n", allocatorName, sMaxUsed, sTotAllocated, nTotAlloc);

	if (anyMemLeaks())
	{
		gos::logger::inc_indent();
		hashof_allocation->forEach ( [](const void *p, const char *filename)
		{
			u32 *p32 = (u32*)p;
			const u32 line_number = p32[0];
			logger::log ("%s %d\n", filename, line_number);
			return true;
		});
		gos::logger::dec_indent();
	}
}