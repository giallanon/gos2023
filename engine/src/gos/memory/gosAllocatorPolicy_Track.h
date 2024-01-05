#ifndef _gosAllocatorPolicy_Track_h_
#define _gosAllocatorPolicy_Track_h_
#include "../gosEnumAndDefine.h"
#include "../gosString.h"

namespace gos
{
    /***********************************************************************
     * AllocPolicy_Track_none
     */
    class AllocPolicy_Track_none
    {
    public:
                    AllocPolicy_Track_none (UNUSED_PARAM(const char *nameIN))                       { }
        bool		anyMemLeaks()                                                                   { return false; }
        void		onAlloc (UNUSED_PARAM(const void *p), UNUSED_PARAM(size_t size))                { }
        void		onDealloc (UNUSED_PARAM(const void *p), UNUSED_PARAM(size_t size))              { }
    };


    /***********************************************************************
     * AllocPolicy_Track_simple
     */
    class AllocPolicy_Track_simple
    {
    public:
                        AllocPolicy_Track_simple (const char *nameIN) :
                            allocatorName(nameIN), nCurAlloc(0), nTotAlloc(0), curMemalloc(0), maxMemalloc(0)
                        {
                        }

        bool			anyMemLeaks()
                        {
                            char s[16];
                            gos::string::format::memoryToKB_MB_GB (maxMemalloc, s, sizeof(s));

                            printf ("AllocatorTrakSimple: final report for [%s] => max mem allocated: %s, num tot allocation: %d\n", allocatorName, s, nTotAlloc);
                            return !(nCurAlloc==0 && curMemalloc==0); 
                        }

        void			onAlloc (UNUSED_PARAM(const void *p), size_t size)
                        {
                            ++nCurAlloc;
                            ++nTotAlloc;
                            curMemalloc += size;
                            if (curMemalloc >= maxMemalloc)
                                maxMemalloc = curMemalloc;

                            //printf ("PROFILE: Allocator [%s] %" PRIu64 " B, max %" PRIu64 " B\n", allocatorName, curMemalloc, maxMemalloc);
                        }

        void			onDealloc (UNUSED_PARAM(const void *p), size_t size)
                        {
                            assert (nCurAlloc>0 && curMemalloc >= size);
                            --nCurAlloc;
                            curMemalloc -= size;

                            //printf ("PROFILE: Allocator [%s] %" PRIu64 " B, max %" PRIu64 " B\n", allocatorName, curMemalloc, maxMemalloc);
                        }

    public:
        const char*     allocatorName;
        u32				nCurAlloc;          //numero di allocazioni vive al momento
        u32				nTotAlloc;          //numero totale di volte che e' stata richiesta una allocazione
        u64				curMemalloc;        //memoria allocata al momento
        u64				maxMemalloc;        //massimo picco della memoria allocata
    };    
} //namespace gos

#endif //_gosAllocatorPolicy_Track_h_
