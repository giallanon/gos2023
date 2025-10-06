#include "gosFreespaceTracker.h"
#include "gos.h"

using namespace gos;


//**************************************
FreespaceTracker::FreespaceTracker()
{
    sizeofABlock = 0;
    numMaxBlocks = 0;
}

//**************************************
FreespaceTracker::~FreespaceTracker()
{
    freerangeList.unsetup ();
}

//**************************************
void FreespaceTracker::setup (u32 totalBufferSize, u32 sizeofABlockIN)
{
    if (0 == sizeofABlock)
        freerangeList.setup (gos::getSysHeapAllocator(), 256);
    else
        freerangeList.reset();

    sizeofABlock = sizeofABlockIN;
    numMaxBlocks = totalBufferSize / sizeofABlock;
    
    freerangeList[0].startingBlock = 0;
    freerangeList[0].numBlock = numMaxBlocks;
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
    u32 nSlotToAlloc = sizeInByte / sizeofABlock;
    if (nSlotToAlloc * sizeofABlock < sizeInByte)
        nSlotToAlloc++;

    const u32 n = freerangeList.getNElem();
    for (u32 i=0; i<n; i++)
    {
        if (freerangeList(i).numBlock >= nSlotToAlloc)
        {
            out->offset = freerangeList(i).startingBlock * sizeofABlock;
            out->size = nSlotToAlloc * sizeofABlock;

            freerangeList[i].startingBlock += nSlotToAlloc;
            freerangeList[i].numBlock -= nSlotToAlloc;
            if (0 == freerangeList(i).numBlock)
                priv_removeEntry (i);
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
	if (0 == info.size || u32MAX==info.offset)
		return;

    //e' garantito da alloc() che info.offset sia un multiplo di <sizeofABlock>
    const u32 firstBlockToFree = info.offset / sizeofABlock;
    const u32 numBlockToFree = info.size / sizeofABlock;

	//cerco la prima entry che inizia dopo firstBlock
	const u32 lastBlock = firstBlockToFree + numBlockToFree;

    const u32 n = freerangeList.getNElem();
	for (u32 i=0; i<n; i++)
	{
		if (freerangeList(i).startingBlock == lastBlock)
		{
			//caso fortunato
			freerangeList[i].startingBlock = firstBlockToFree;
			freerangeList[i].numBlock += numBlockToFree;
			priv_tryMerge (i);
			return;
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
					return;
				}
			}

			//devo inserire un nuovo record prima di i
            sRange range;
            range.startingBlock = firstBlockToFree;
            range.numBlock = numBlockToFree;
            freerangeList.insertAt(i, range);
			return;
		}
	}

	//se arrivo qui vuol dire che devo aggiungere una entry a fine buffer
	freerangeList[n].startingBlock = firstBlockToFree;
	freerangeList[n].numBlock = numBlockToFree;
}

