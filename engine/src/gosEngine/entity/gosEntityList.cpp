#include "gosEntityList.h"

using namespace gos;
using namespace gos::ent;

//***************************************
List::List()
{
	allocator = NULL;
	pageList = NULL;
}

//***************************************	
void List::priv_free()
{
	if (NULL == allocator)
		return;

	u32 n = allocatedPage_indexList.getNElem();
	for (u32 i = 0; i < n; i++)
	{
		const u32 index = allocatedPage_indexList(i);
		assert (NULL != pageList[index].bitmask);
		GOSFREE(allocator, pageList[index].bitmask);
	}
	GOSFREE(allocator, pageList);
	pageList = NULL;
	
	allocatedPage_indexList.unsetup ();
	entList.unsetup ();
}

//***************************************
void List::setup (gos::Allocator *allocatorIN)
{
	priv_free();
	allocator = allocatorIN;

	pageList = GOSALLOCT(sPage*, allocator, sizeof(sPage) * NUM_PAGES);
	memset (pageList, 0, sizeof(sPage) * NUM_PAGES);

	entList.setup (allocator, 1024);
	allocatedPage_indexList.setup (allocator, 256);
	memset (allocatedPage_bitmask, 0, sizeof(allocatedPage_bitmask));
}

//***************************************
void List::priv_allocPage (u32 pageIndex)
{
	assert (pageIndex < NUM_PAGES);

	//non dovrebbe essere gia' allocata
	const u32 byte = pageIndex >> 3;
	const u32 bitmask = 0x01 << (pageIndex & 0x07);

	assert (byte < 1024);
	assert (0 == (allocatedPage_bitmask[byte] & bitmask) );
	assert (NULL == pageList[pageIndex].bitmask);
	assert (!priv_pageExists(pageIndex));

	allocatedPage_bitmask[byte] |= bitmask;
	pageList[pageIndex].bitmask = GOSALLOCT(u8*, allocator, PAGE_SIZE);
	pageList[pageIndex].reset();

	allocatedPage_indexList.append (pageIndex);
}

//***************************************
void List::priv_freePage (u32 pageIndex)
{
	assert (pageIndex < NUM_PAGES);

	//dovrebbe essere gia' allocata
	const u32 byte = pageIndex >> 3;
	const u32 bitmask = 0x01 << (pageIndex & 0x07);

	assert (byte < 1024);
	assert (0 != (allocatedPage_bitmask[byte] & bitmask) );
	assert (NULL != pageList[pageIndex].bitmask);
	assert (priv_pageExists(pageIndex));

	allocatedPage_bitmask[byte] &= (0xFF - bitmask);
	GOSFREE(allocator, pageList[pageIndex].bitmask);
	pageList[pageIndex].bitmask = NULL;
	pageList[pageIndex].numElement = 0;

	const u32 i = allocatedPage_indexList.simpleSearch(pageIndex);
	assert (u32MAX != i);
	allocatedPage_indexList.removeAndSwapWithLast(i);
}

//***************************************
bool List::priv_pageExists (u32 pageIndex) const
{
	assert (pageIndex < NUM_PAGES);

	const u32 byte = pageIndex >> 3;
	const u32 bitmask = 0x01 << (pageIndex & 0x07);
	assert (byte < 1024);
	return (allocatedPage_bitmask[byte] & bitmask) != 0;
}


//***************************************
void List::priv_calcAddress (const Entity ent, u32 *out_pageIndex, u32 *out_byte, u32 *out_bitmask) const
{
	const u32 entID = ent.id;
	(*out_pageIndex) = entID >> 16;	//  page = id / 65536
	
	const u32 address = entID - ((*out_pageIndex) << 16);
	(*out_byte) = address >> 3;
	(*out_bitmask) = 0x01 << (address & 0x07);
}

//***************************************
bool List::addIfNotExists (const Entity ent)
{
	u32 pageIndex;
	u32 byte;
	u32 bitmask;
	priv_calcAddress (ent, &pageIndex, &byte, &bitmask);

	if (!priv_pageExists(pageIndex))
		priv_allocPage(pageIndex);

	if (0x00 == (pageList[pageIndex].bitmask[byte] & bitmask))
	{
		pageList[pageIndex].bitmask[byte] |= bitmask;
		pageList[pageIndex].numElement++;
		pageList[pageIndex].lastTimeUsed = gos::getTimeSinceStart_msec();
		entList.append (ent);
		return true;
	}

	return false;
}

//***************************************
void List::reset()
{
	entList.reset();

	const u64 timenow_msec = gos::getTimeSinceStart_msec();
	u32 n = allocatedPage_indexList.getNElem();
	for (u32 i = 0; i < n; i++)
	{
		const u32 pageIndex = allocatedPage_indexList(i);
		
		if (0 != pageList[pageIndex].numElement)
			pageList[pageIndex].reset();

		if (timenow_msec - pageList[pageIndex].lastTimeUsed > 5000)
		{
			priv_freePage(pageIndex);
			i--;
			n--;
		}
	}
}
