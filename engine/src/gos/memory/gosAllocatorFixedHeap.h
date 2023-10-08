#ifndef _gosAllocatorFixedHeap_h_
#define _gosAllocatorFixedHeap_h_
#include "gosAllocator.h"
#include "gosAllocatorPolicy_Thread.h"
#include "gosAllocatorPolicy_Track.h"
#include "gosMemory.h"

namespace gos
{
    /*********************************************************************
    * AllocatorFixedHeap
    *
    *	Prende in input un blocco di memoria preallocato e non espandibile e lo gestisce come un heap, il che vuol
    *	dire che supporta allocazioni di dimensione variabile all'interno del blocco di memoria impostato.
    *
    *	Il blocco da gestire va impostato usando la setup()
    *
    *	L'Allocator e' threadsafe oppure no, a seconda della ThreadPolicy indicata
    *
    */
    template <class ThreadPolicy>
    class AllocatorFixedHeap : public Allocator
    {
    public:
                        AllocatorFixedHeap (const char *nameIN=NULL) : Allocator(nameIN)
                        { 
                            memory = NULL; 
                            freeBlock = NULL;
                            sentinelOffset = sizeOfMemory = 0;
                        }

        void			setup (void *memoryIN, size_t sizeOfMemoryIN)
                        {
                            assert (memoryIN != NULL);
                            assert (sizeOfMemoryIN > sizeof(sBlock));
                            assert (sizeOfMemoryIN <= u32MAX);

                            memory = (u8*)memoryIN;
                            sizeOfMemory = (u32)sizeOfMemoryIN;

                            sentinelOffset = sizeOfMemory - sizeof(sBlock);
                            sBlock *sentinel = (sBlock*)&memory[sentinelOffset];
                            sentinel->next = u32MAX;
                            sentinel->bytesFree = sizeof(sBlock);

                            freeBlock = (sBlock*)memory;
                            freeBlock->bytesFree = sentinelOffset;
                            freeBlock->next = sentinelOffset;
                        }

        size_t			getAllocatedSize (const void *pIN)
                        {
                            u32 allocatedSize, alignOffset;
                            priv_getPointerInfo (pIN, &allocatedSize, &alignOffset);
                            return allocatedSize -alignOffset;
                        }

        bool			wasAllocatedByMe (const void *pIN) const
                        {
                            //verifica se pIN e' stato allocato da this
                            u8 *p8 = (u8*)pIN;
                            p8 -=  sizeof(u32);
                            if (p8 < memory)
                                return false;
                            if ((u32)(p8-memory) >= sizeOfMemory)
                                return false;
                            return true;
                        }

        bool			isThreadSafe() const														{ return thread.isThreadSafe(); }
        void*			getMemBlock() const															{ return memory; }
        u32				getSizeOfMemBlock() const													{ return sizeOfMemory; }

        void			SanityCheck1 ()																{ assert (freeBlock->bytesFree == sizeOfMemory - sizeof(sBlock) && freeBlock->next == sentinelOffset); }

    private:
        struct sBlock
        {
            u32	bytesFree;
            u32	next;
        };

    private:
        void*			virt_do_alloc (size_t sizeIN, u8 alignPowerOfTwo)
                        {
                            thread.lock();
                            if (align==0)
                                align=1;
                            assert (sizeIN <= 0x00FFFFFF);
                            u32 size = (u32)sizeIN;
                            const u32 minBytesNeeded = size + sizeof(sBlock);

                            sBlock *q = NULL;
                            sBlock *p = freeBlock;
                            while (p)
                            {
                                if (p->bytesFree >= minBytesNeeded)
                                {
                                    u32 alignOffset = mem::calcAlignAdjustment (p, alignPowerOfTwo); 

                                    u32 realSizeToalloc = size +alignOffset;
                                    while (alignOffset < sizeof(u32))
                                    {
                                        realSizeToalloc += align;
                                        alignOffset += align;
                                    }
                                    //if (realSizeToalloc < sizeof(sBlock))	realSizeToalloc = sizeof(sBlock);

                                    if (p->bytesFree >= realSizeToalloc + sizeof(sBlock))
                                    {
                                        const u32 bytesLeft = p->bytesFree - realSizeToalloc;
                                        if (bytesLeft > 0)
                                        {
                                            u8 *pp = (u8*)p;
                                            pp += realSizeToalloc;
                                            ((sBlock*)pp)->bytesFree = bytesLeft;
                                            ((sBlock*)pp)->next = p->next;

                                            if (NULL == q)
                                                freeBlock = (sBlock*)pp;
                                            else
                                                q->next = (u32)(pp - memory);
                                        }
                                        else
                                        {
                                            if (NULL == q)
                                                freeBlock = priv_getBlockAt( p->next );
                                            else
                                                q->next = p->next;
                                        }
                                        thread.unlock();
                                        return priv_MarkallocationAndReturn (p, realSizeToalloc, alignOffset);
                                    }
                                }

                                if (p->next != sentinelOffset)
                                {
                                    q = p;
                                    p = priv_getBlockAt(p->next);
                                }
                                else
                                {
                                    thread.unlock();
                                    return NULL;
                                }
                            }
                            thread.unlock();
                            return NULL;
                        }

        void			virt_do_dealloc (void *pIN)
                        {
                            if (!pIN)
                                return;
                            assert (wasAllocatedByMe(pIN));
                            thread.lock();
                            u32 allocatedSize, alignOffset;
                            u8	*pBlock = priv_getPointerInfo (pIN, &allocatedSize, &alignOffset);

                            sBlock	*q = NULL;
                            sBlock	*p = freeBlock;
                            while (1)
                            {
                                if ((u8*)p > pBlock)
                                {
                                    //il nuovo blocco, va messo tra q e p
                                    sBlock	*newBlock = (sBlock*)pBlock;
                                    newBlock->bytesFree = allocatedSize;
                                    if (NULL == q)
                                        newBlock->next = (u32)((u8*)p - memory);
                                    else
                                    {
                                        newBlock->next = q->next;
                                        q->next = (u32)(pBlock - memory);
                                    }

                                    //vediamo se posso mergiare i 3 blocchi in qualche modo

                                    //posso fare merge con il blocco precedente?
                                    if (NULL != q)
                                    {
                                        if (priv_MergeWithPrevious (newBlock, q))	//se riesco a mergiarmi col precedente
                                            priv_MergeWithNext (q, p);				//allora vedo se q (che � stato modificato) pu� mergiarsi con il successivo
                                        else
                                            priv_MergeWithNext (newBlock, p);		//altimenti vedo se posso mergiarmi col successivo
                                    }
                                    else
                                    {
                                        priv_MergeWithNext (newBlock, p);
                                        freeBlock = newBlock;
                                    }
                                    thread.unlock();
                                    return;
                                }

                                if (p->next != sentinelOffset)
                                {
                                    q = p;
                                    p = priv_getBlockAt(p->next);
                                }
                                else
                                {
                                    assert ((u8*)p +p->bytesFree == pBlock);
                                    p->bytesFree += allocatedSize;
                                    thread.unlock();
                                    return;
                                }
                            }
                        }

        sBlock*			priv_getBlockAt (u32 offset)												{ assert (offset<sizeOfMemory); return (sBlock*)&memory[offset]; }

        bool			priv_MergeWithPrevious (const sBlock *p, sBlock *prev)
                        {
                            assert (prev->next == (u8*)p - memory);
                            if ((u8*)prev + prev->bytesFree == (u8*)p)
                            {
                                prev->bytesFree += p->bytesFree;
                                prev->next = p->next;
                                return true;
                            }
                            return false;
                        }

        bool			priv_MergeWithNext (sBlock *p, const sBlock *pNext)
                        {
                            assert (p->next == (u8*)pNext - memory);
                            if ((u8*)p + p->bytesFree == (u8*)pNext)
                            {
                                p->bytesFree += pNext->bytesFree;
                                p->next = pNext->next;
                                return true;
                            }
                            return false;
                        }

        void*			priv_MarkallocationAndReturn (void *p, u32 bytesallocated, u32 alignOffset)
                        {
                            assert (alignOffset <= 0xff);
                            u32 *pp = (u32*)((u8*)p +alignOffset - 4);
                            pp[0] = (bytesallocated & 0x00FFFFFF) | ((alignOffset<<24) & 0xFF000000);
                            return (u8*)p +alignOffset;
                        }
        
        u8*				priv_getPointerInfo (const void *pIN, u32 *out_allocatedSize, u32 *out_alignOffset)
                        {
                            //ritorna il pt al blocco allocato, e le info su quanti byte sono stati allocati e i byte di offset dell'allineamento
                            u8 *p8 = (u8*)pIN;
                            p8 -=  sizeof(u32);
                            assert (p8 >= memory);
                            assert ((u32)(p8-memory) < sizeOfMemory);

                            (*out_allocatedSize) = (((u32*)p8)[0] & 0x00FFFFFF);
                            (*out_alignOffset)   = ((((u32*)p8)[0] & 0xFF000000) >> 24);
                            return (u8*)pIN - (*out_alignOffset);
                        }


    private:
        ThreadPolicy	thread;
        u8				*memory;
        u32				sizeOfMemory;
        u32				sentinelOffset;
        sBlock			*freeBlock;
    };
} //namespace gos
#endif //_gosAllocatorFixedHeap_h_
