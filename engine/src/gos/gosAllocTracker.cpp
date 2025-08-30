#include "gosAllocTracker.h"
#include "gos.h"

using namespace gos;


//****************************************************
void AllocTracker::unsetup()
{
    if (NULL == localAllocator)
        return;

    freeBlockList.unsetup();
    memAllocated = memSize = 0;
    localAllocator = NULL;
}

//****************************************************
void AllocTracker::setup (u32 totAvailMemory)
{
    localAllocator = gos::getSysHeapAllocator();
    memSize = totAvailMemory;
    memAllocated = 0;

    freeBlockList.setup (localAllocator, 16);
    freeBlockList[0].startByte = 0;
    freeBlockList[0].freeByte = memSize;
}

//****************************************************
bool AllocTracker::alloc (u32 howMuch, bool bFindBest, Handle *out)
{
    assert (NULL != out);
    assert (howMuch > 0);

    out->start = out->len = 0;    
    if (howMuch > getMemLeft())
        return false;

    const u32 numEntry = freeBlockList.getNElem();
    u32 iBest = u32MAX;
    if (bFindBest)
	{
		//alloca spazio dall'entry piu' piccolo che trova
		u32 iBestSize = u32MAX;
		for (u32 i=0; i<numEntry; i++)
		{
			if (freeBlockList[i].freeByte >= howMuch)
			{
				if (freeBlockList[i].freeByte == howMuch)
				{
					iBest = i;
					break;
				}
				if (freeBlockList[i].freeByte < iBestSize)
				{
					iBestSize = freeBlockList[i].freeByte;
					iBest = i;
				}
			}
		}
	}
	else
	{
		//alloca spazio dal primo entry buono che incontra
		for (u32 i=0; i<numEntry; i++)
		{
			if (freeBlockList[i].freeByte >= howMuch)
			{
				iBest = i;
                break;
			}
		}
	}

    if (u32MAX == iBest)
        return false;

    out->start = freeBlockList[iBest].startByte;
    out->len = howMuch;
    freeBlockList[iBest].startByte += howMuch;
    freeBlockList[iBest].freeByte -= howMuch;
    if (freeBlockList[iBest].freeByte == 0)
        freeBlockList.remove (iBest);
    memAllocated += howMuch;
    return true;
}

//****************************************************
void AllocTracker::dealloc (Handle &h)
{
	if (h.len == 0)
		return;
	if (h.start >= memSize)
		return;

	//cerco la prima entry che inizia dopo h.start
	const u32 blockEnd = h.start + h.len;
    const u32 numEntry = freeBlockList.getNElem();
	for (u32 i=0; i<numEntry; i++)
	{
		if (freeBlockList[i].startByte == blockEnd)
		{
			//caso fortunato
			freeBlockList[i].startByte = h.start;
			freeBlockList[i].freeByte += h.len;
			priv_tryMerge (i);
			memAllocated -= h.len;
			return;
		}
		if (freeBlockList[i].startByte > blockEnd)
		{
			//vedo se l'entry precedente e' ok per il merge
			if (i>0)
			{
				if (freeBlockList[i-1].startByte + freeBlockList[i-1].freeByte == h.start)
				{
					freeBlockList[i-1].freeByte += h.len;
					priv_tryMerge (i-1);
					memAllocated -= h.len;;
					return;
				}
			}

			//devo inserire un nuovo record prima di i
            sInfo info;
            info.startByte = h.start;
			info.freeByte = h.len;
            freeBlockList.insertAt(i, info);
			memAllocated -= h.len;
			return;
		}
	}

	//se arrivo qui vuol dire che devo aggiungere una entry a fine freeBlockList
	sInfo info;
	info.startByte = h.start;
	info.freeByte = h.len;
    freeBlockList.append (info);
    memAllocated -= h.len;
	priv_tryMerge (numEntry);
}

//**********************************************************
void AllocTracker::priv_tryMerge (u32 iEntry)
{
	if (iEntry > 0)
	{
		//posso fare il merge col precedente?
		const u32 e = freeBlockList[iEntry-1].startByte + freeBlockList[iEntry-1].freeByte;
		if (freeBlockList[iEntry].startByte == e)
		{
			freeBlockList[iEntry-1].freeByte += freeBlockList[iEntry].freeByte;
			freeBlockList.remove (iEntry);
			priv_tryMerge(iEntry-1);
			return;
		}
	}

	//posso fare il merge con il successivo?
	if (iEntry < freeBlockList.getNElem()-1)
	{
		const u32 e = freeBlockList[iEntry].startByte + freeBlockList[iEntry].freeByte; 
		if (freeBlockList[iEntry+1].startByte == e)
		{
			freeBlockList[iEntry].freeByte += freeBlockList[iEntry+1].freeByte;
			freeBlockList.remove (iEntry+1);
			priv_tryMerge(iEntry);
			return;
		}
	}
}

#ifdef _DEBUG
//**********************************************************
void AllocTracker::DEBUG_sanityCheck() const
{
	//coerenza delle entry
	u32 totFree = 0;
	u32 prevEnd = 0;
	for (u32 i=0; i<freeBlockList.getNElem(); i++)
	{
		assert (freeBlockList(i).startByte >= prevEnd);
		assert (freeBlockList(i).freeByte > 0);
        assert (freeBlockList(i).startByte + freeBlockList(i).freeByte <= getMemSize());
		
        prevEnd = freeBlockList(i).startByte + freeBlockList(i).freeByte;
		totFree += freeBlockList(i).freeByte;
	}
	assert (totFree <= getMemLeft());

	//compattezza delle entry
	for (u32 i=1; i<freeBlockList.getNElem(); i++)
	{
		const u32 end = freeBlockList(i-1).startByte + freeBlockList(i-1).freeByte;
		assert ( freeBlockList(i).startByte >= end );
	}
}
#endif