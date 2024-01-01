#ifndef _gosAllocatorPolicy_Track_h_
#define _gosAllocatorPolicy_Track_h_
#include "../gosEnumAndDefine.h"

namespace gos
{
    /***********************************************************************
     * AllocPolicy_Track_none
     */
    class AllocPolicy_Track_none
    {
    public:
                    AllocPolicy_Track_none (UNUSED_PARAM(const char *nameIN))		            { }
        bool		anyMemLeaks()										                        { return false; }
        void		onAlloc (UNUSED_PARAM(const void *p), UNUSED_PARAM(size_t size), UNUSED_PARAM(const char *allocatorName), UNUSED_PARAM(u32 allocatorID))			{ }
        void		onDealloc (UNUSED_PARAM(const void *p), UNUSED_PARAM(size_t size), UNUSED_PARAM(const char *allocatorName), UNUSED_PARAM(u32 allocatorID))	    { }
    };


    /***********************************************************************
     * AllocPolicy_Track_simple
     */
    class AllocPolicy_Track_simple
    {
    public:
                        AllocPolicy_Track_simple (UNUSED_PARAM(const char *nameIN)) :
                            nalloc(0), curMemalloc(0), maxMemalloc(0)
                        {
                        }

        bool			anyMemLeaks()															{ return !(nalloc==0 && curMemalloc==0); }

        void			onAlloc (UNUSED_PARAM(const void *p), size_t size, UNUSED_PARAM(const char *allocatorName), UNUSED_PARAM(u32 allocatorID))
                        {
                            ++nalloc;
                            curMemalloc += size;
                            if (curMemalloc >= maxMemalloc)
                                maxMemalloc = curMemalloc;

                            //printf ("PROFILE: Allocator [%s] %" PRIu64 " B, max %" PRIu64 " B\n", allocatorName, curMemalloc, maxMemalloc);
                        }

        void			onDealloc (UNUSED_PARAM(const void *p), size_t size, UNUSED_PARAM(const char *allocatorName), UNUSED_PARAM(u32 allocatorID))
                        {
                            assert (nalloc>0 && curMemalloc >= size);
                            --nalloc;
                            curMemalloc -= size;

                            //printf ("PROFILE: Allocator [%s] %" PRIu64 " B, max %" PRIu64 " B\n", allocatorName, curMemalloc, maxMemalloc);
                        }

    public:
        u32				nalloc;
        u64				curMemalloc;
        u64				maxMemalloc;
    };    
} //namespace gos

#endif //_gosAllocatorPolicy_Track_h_
