#include "gosFreespaceTracker.h"
#include "gos.h"

using namespace gos;


//**************************************
FreespaceTracker::FreespaceTracker()
{
	localAllocator = NULL;
    sizeof_block = 0;
    num_max_blocks = 0;
	num_allocated_block = 0;
}

//**************************************
void FreespaceTracker::unsetup()
{
	if (NULL == localAllocator)
		return;
    freerangeList.unsetup ();
    sizeof_block = 0;
    num_max_blocks = 0;
	num_allocated_block = 0;
	localAllocator = NULL;
}

//**************************************
void FreespaceTracker::setup (u32 totalBufferSize, u32 sizeofABlockIN, gos::Allocator *allocatorIN)
{
    unsetup();

	if (NULL == allocatorIN)
		localAllocator = gos::getSysHeapAllocator();
	else
		localAllocator = allocatorIN;
		
	freerangeList.setup (localAllocator, 256);

    sizeof_block = sizeofABlockIN;
    num_max_blocks = totalBufferSize / sizeof_block;
	num_allocated_block = 0;
    
    freerangeList[0].startingBlock = 0;
    freerangeList[0].numBlock = num_max_blocks;
}

//**********************************************************
void FreespaceTracker::priv_removeEntry (u32 at)
{
	freerangeList.remove(at);
}

//**********************************************************
void FreespaceTracker::priv_tryMerge (u32 iEntry)
{
	if (iEntry > 0)
	{
		//posso fare il merge col precedente?
		const u32 e = freerangeList(iEntry-1).startingBlock + freerangeList(iEntry-1).numBlock;
		if (freerangeList[iEntry].startingBlock == e)
		{
			freerangeList[iEntry-1].numBlock += freerangeList(iEntry).numBlock;
			priv_removeEntry (iEntry);
			priv_tryMerge(iEntry-1);
			return;
		}
	}

	//posso fare il merge con il successivo?
	if (iEntry < freerangeList.getNElem()-1)
	{
		const u32 e = freerangeList(iEntry).startingBlock + freerangeList(iEntry).numBlock;
		if (freerangeList(iEntry+1).startingBlock == e)
		{
			freerangeList[iEntry].numBlock += freerangeList(iEntry+1).numBlock;
			priv_removeEntry(iEntry+1);
			priv_tryMerge(iEntry);
			return;
		}
	}
}

//**************************************
bool FreespaceTracker::alloc (u32 sizeInByte, AllocInfo *out)
{
    assert (NULL != out);
    u32 nSlotToAlloc = sizeInByte / sizeof_block;
    if (nSlotToAlloc * sizeof_block < sizeInByte)
        nSlotToAlloc++;

    const u32 n = freerangeList.getNElem();
    for (u32 i=0; i<n; i++)
    {
        if (freerangeList(i).numBlock >= nSlotToAlloc)
        {
            out->offset = freerangeList(i).startingBlock * sizeof_block;
            out->size = nSlotToAlloc * sizeof_block;

            freerangeList[i].startingBlock += nSlotToAlloc;
            freerangeList[i].numBlock -= nSlotToAlloc;
            if (0 == freerangeList(i).numBlock)
                priv_removeEntry (i);

			num_allocated_block += nSlotToAlloc;
			assert (num_allocated_block <= num_max_blocks);
            return true;
        }
    }    

    out->offset = u32MAX;
    out->size = 0;
    return false;
}

//**************************************
void FreespaceTracker::free (AllocInfo &info)
{
	const u32 num_freed_bloks = priv__free(info);
	assert (num_allocated_block >= num_freed_bloks);
	num_allocated_block -= num_freed_bloks;
}

//**************************************
u32 FreespaceTracker::priv__free (AllocInfo &info)
{
	if (0 == info.size || u32MAX==info.offset)
		return 0;

    //e' garantito da alloc() che info.offset sia un multiplo di <sizeofABlock>
    const u32 firstBlockToFree = info.offset / sizeof_block;
    const u32 numBlockToFree = info.size / sizeof_block;

	//cerco la prima entry che inizia dopo firstBlock
	const u32 lastBlock = firstBlockToFree + numBlockToFree;

	assert (lastBlock <= num_max_blocks);

    const u32 n = freerangeList.getNElem();
	for (u32 i=0; i<n; i++)
	{
		if (freerangeList(i).startingBlock == lastBlock)
		{
			//caso fortunato
			freerangeList[i].startingBlock = firstBlockToFree;
			freerangeList[i].numBlock += numBlockToFree;
			priv_tryMerge (i);
			return numBlockToFree;
		}
		if (freerangeList(i).startingBlock > lastBlock)
		{
			//vedo se l'entry precedente è ok per il merge
			if (i>0)
			{
				if (freerangeList(i-1).startingBlock + freerangeList(i-1).numBlock == firstBlockToFree)
				{
					freerangeList[i-1].numBlock += numBlockToFree;
					priv_tryMerge (i-1);
					return numBlockToFree;
				}
			}

			//devo inserire un nuovo record prima di i
            sRange range;
            range.startingBlock = firstBlockToFree;
            range.numBlock = numBlockToFree;
            freerangeList.insertAt(i, range);
			return numBlockToFree;
		}
	}

	//se arrivo qui vuol dire che devo aggiungere una entry a fine buffer
	if (n>0)
	{
		if (freerangeList(n-1).startingBlock + freerangeList(n-1).numBlock == firstBlockToFree)
		{
			freerangeList[n-1].numBlock += numBlockToFree;
			priv_tryMerge (n-1);
			return numBlockToFree;
		}
	}
	
	freerangeList[n].startingBlock = firstBlockToFree;
	freerangeList[n].numBlock = numBlockToFree;
	return numBlockToFree;
}

