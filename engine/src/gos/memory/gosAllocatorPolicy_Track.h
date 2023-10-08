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
                    AllocPolicy_Track_none (UNUSED_PARAM(const char *nameIN))		{ }

        bool		anyMemLeaks()										            { return false; }
        void		onAlloc (const void *p, size_t size)				            { }
        void		onDealloc (const void *p, size_t size)				            { }
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

        void			onAlloc (const void *p, size_t size)
                        {
                            ++nalloc;
                            curMemalloc += size;
                            if (curMemalloc >= maxMemalloc)
                                maxMemalloc = curMemalloc;
                        }

        void			onDealloc (const void *p, size_t size)
                        {
                            assert (nalloc>0 && curMemalloc >= size);
                            --nalloc;
                            curMemalloc -= size;
                        }

    public:
        u32				nalloc;
        u64				curMemalloc;
        u64				maxMemalloc;
    };    
} //namespace gos

#endif //_gosAllocatorPolicy_Track_h_
