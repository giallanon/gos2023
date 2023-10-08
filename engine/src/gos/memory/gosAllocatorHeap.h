#ifndef _gosAllocatorHeap_h_
#define _gosAllocatorHeap_h_
#include "gosAllocator.h"
#include "gosAllocatorPolicy_Thread.h"
#include "gosAllocatorPolicy_Track.h"
#include "Doug_Lea_malloc.h"


namespace gos
{
    /*********************************************************************
    * AllocatorHeap
    *
    * Usa la DLMalloc (Doug Lea malloc)
    */
    template <class TrackingPolicy, class ThreadPolicy>
    class AllocatorHeap : public Allocator
    {
    public:
                        AllocatorHeap (const char *nameIN=NULL) : Allocator(nameIN), ms(NULL), track(nameIN)
                        {
                        }

                        ~AllocatorHeap()
                        {
#ifdef _DEBUG
                            if (track.anyMemLeaks())
                                DBGBREAK;
#endif
                            if (NULL != ms)
                                destroy_mspace(ms);
                        }

    void				setup (size_t startSizeInBytes)
                        {
                            /*  Costruisce un heap di dimensioni iniziali = startSizeInBytes.
                                Usa DLmalloc per allocare la memoria inziale ed ulteriori blocchi di memoria in caso di espansione*/
                            assert (startSizeInBytes > 0);
                            assert (NULL == ms);
                            ms = create_mspace (startSizeInBytes, 0);
                            assert(NULL != ms);
                        }

    void				setup (void *baseMemory, size_t sizeOfMemoryInBytes)
                        {
                            /* Memory deve essere una valida zona di memoria, e sizeOfMemory deve essere la sua dimensione
                                Costruisce un heap su "memory". Ulteriori blocchi di memoria vengono allocati alla bisogna via DLmalloc (in caso di espansione).
                                Non fa il free del blocco iniziale "baseMemory".
                                Fa il free di eventuali ulteriori blocchi allocati autonomamente*/
                            assert (NULL == ms);
                            assert (sizeOfMemoryInBytes>0 && NULL != baseMemory);
                            ms = create_mspace_with_base (baseMemory, sizeOfMemoryInBytes, 0);
                            assert(NULL != ms);
                        }

    bool				isThreadSafe() const								{ return thread.isThreadSafe(); }
    size_t				getAllocatedSize (const void *p)					{ return mspace_usable_size (p); }


    private:
        void*			virt_do_alloc (size_t sizeInBytes, u8 align)
                        {
                            assert (NULL != ms);
                            thread.lock();
                            void *ret = mspace_memalign (ms, align, sizeInBytes);
                            assert (ret);
                            track.onAlloc (ret, getAllocatedSize(ret));
                            thread.unlock();
                            return ret;
                        }

        void			virt_do_dealloc (void *p)
                        {
                            if (!p)
                                return;
                            assert (NULL!=ms);
                            thread.lock();
                            track.onDealloc (p, getAllocatedSize(p));
                            mspace_free (ms, p);
                            thread.unlock();
                        }


    private:
        mspace			ms;
        ThreadPolicy	thread;
        TrackingPolicy	track;

    };
} //namespace gos
#endif //_gosAllocatorHeap_h_
