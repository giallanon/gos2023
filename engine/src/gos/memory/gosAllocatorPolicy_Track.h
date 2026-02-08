#ifndef _gosAllocatorPolicy_Track_h_
#define _gosAllocatorPolicy_Track_h_
#include "../gosEnumAndDefine.h"
#include "../gosString.h"
#include "../gos.h"

namespace gos
{
    /***********************************************************************
     * AllocPolicy_Track_none
     */
    class AllocPolicy_Track_none
    {
    public:
                    AllocPolicy_Track_none (UNUSED_PARAM(const char *nameIN))                       							{ }
        void        printReport (UNUSED_PARAM(u64 totMemoryReserved))                               							{ }
        bool		anyMemLeaks() const                                                                							{ return false; }
        void		onAlloc (UNUSED_PARAM(const void *p), UNUSED_PARAM(size_t size), UNUSED_PARAM(const char *debug_filename))	{ }
        void		onDealloc (UNUSED_PARAM(const void *p), UNUSED_PARAM(size_t size))              							{ }
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

        void            printReport (u64 totMemoryReserved)
        {
                            char sTotAllocated[16];
                            gos::string::format::memoryToKB_MB_GB (totMemoryReserved, sTotAllocated, sizeof(sTotAllocated));

                            char sMaxUsed[16];
                            gos::string::format::memoryToKB_MB_GB (maxMemalloc, sMaxUsed, sizeof(sMaxUsed));

                            gos::logger::log (eTextColor::blue, "AllocatorTrackSimple: final report for [%s] => max mem allocated: %s/%s, num tot allocation: %d\n", allocatorName, sMaxUsed, sTotAllocated, nTotAlloc);
        }

        bool			anyMemLeaks() const						{ return !(nCurAlloc==0 && curMemalloc==0); }

        void			onAlloc (UNUSED_PARAM(const void *p), size_t size, UNUSED_PARAM(const char *debug_filename))
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


    /***********************************************************************
     * AllocPolicy_Track_hard
     */
    class AllocPolicy_Track_hard
    {
    public:
                        AllocPolicy_Track_hard (const char *nameIN);

        void            printReport (u64 totMemoryReserved);
        bool			anyMemLeaks() const			{ return !(nCurAlloc==0 && curMemalloc==0); }

        void			onAlloc (const void *p, size_t size, const char *debug_filename);
        void			onDealloc (const void *p, size_t size);

    public:
        const char*     allocatorName;
        u32				nCurAlloc;          //numero di allocazioni vive al momento
        u32				nTotAlloc;          //numero totale di volte che e' stata richiesta una allocazione
        u64				curMemalloc;        //memoria allocata al momento
        u64				maxMemalloc;        //massimo picco della memoria allocata
    };
} //namespace gos

#endif //_gosAllocatorPolicy_Track_h_
