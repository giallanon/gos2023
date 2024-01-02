#include "gosMemory.h"
#include "gosAllocatorHeap.h"


u16 gos::Allocator::allocatorID = 1;

typedef gos::AllocatorHeap<gos::AllocPolicy_Track_simple, gos::AllocPolicy_Thread_Safe>		GOSMemGlobalHeapAllocatorTS;
static GOSMemGlobalHeapAllocatorTS	*systemHeapAllocator = NULL;
static GOSMemGlobalHeapAllocatorTS	*systemScrapAllocator = NULL;

using namespace gos;


//***************************************************
bool mem::priv_init (const gos::sGOSInit &init)
{
	systemHeapAllocator = new GOSMemGlobalHeapAllocatorTS("sysHeap");
	systemHeapAllocator->setup (init._memory.startingSizeOfDefaultHeapAllocator_MB * 1024 * 104);

	systemScrapAllocator = new GOSMemGlobalHeapAllocatorTS("sysScrap");
	systemScrapAllocator->setup (init._memory.startingSizeOfScrapAllocator_MB * 1024 * 104);
	
	return true;
}

//***************************************************
void mem::priv_deinit()
{
	delete systemHeapAllocator;
	delete systemScrapAllocator;
}

//***************************************************
gos::Allocator* mem::getSysHeapAllocator()	{ return systemHeapAllocator; }
gos::Allocator* mem::getScrapAllocator()	{ return systemScrapAllocator; }

//***************************************************
u32 mem::calcAlignAdjustment(const void *mem, u32 alignmentPowerOfTwo)
{
	if (alignmentPowerOfTwo == 0)
		return 0;
    assert(GOS_IS_POWER_OF_TWO(alignmentPowerOfTwo));

	const u32 adjustment = alignmentPowerOfTwo - ( ((u64)mem & 0xFF) & (alignmentPowerOfTwo-1) );
	if (adjustment == alignmentPowerOfTwo) 
		return 0;
	return adjustment;
}

//***************************************************
u32 mem::calcAlignAdjustmentWithHeader (const void *mem, u32 alignmentPowerOfTwo, u32 headerSize)
{
    assert(GOS_IS_POWER_OF_TWO(alignmentPowerOfTwo));

	u32 adjustment = calcAlignAdjustment (mem, alignmentPowerOfTwo);
	while (adjustment < headerSize)
		adjustment += alignmentPowerOfTwo;

	assert (isPointerAligned((void*)((uintptr_t)mem + adjustment), alignmentPowerOfTwo));


	return adjustment;
}