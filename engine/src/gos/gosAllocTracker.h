#ifndef _gosAllocTracker_h_
#define _gosAllocTracker_h_
#include "gosFastArray.h"

namespace gos
{
    /**
     * @brief AllocTracker
     * Dato un ipotetico spazio grosso <totAvailMemory>, le funzioni alloc() e dealloc() riservano spazio
     * in questo buffer e ritornano il byte di start dello spazio libero.
     * Il buffer in memoria non viene davvero allocato, questa classe si occupa solo di gestire gli spazi liberi e gli
     * spazi occupati di un ipotetico buffer esterno
     */
    class AllocTracker
    {
    public:
        struct sHandle
        {
            u32 start;
            u32 len;
        };

        typedef sHandle Handle;

    public:
                AllocTracker ()                     { localAllocator = NULL; }
                ~AllocTracker()                     { unsetup(); }

        void    setup (u32 totAvailMemoryToHandle);
        void    unsetup();

        bool    alloc (u32 howMuch, bool bFindBest, Handle *out);
        void    dealloc (Handle &h);

        u32     getMemSize() const                          { return memSize; }
        u32     getMemAllocated() const                     { return memAllocated; }
        u32     getMemLeft() const                          { return memSize - memAllocated; }

        void    DEBUG_sanityCheck() const;

    private:
        struct sInfo
        {
            u32 startByte;
            u32 freeByte;
        };

    private:
        void    priv_tryMerge (u32 iEntry);

    private:
        gos::Allocator      *localAllocator;
        FastArray<sInfo>    freeBlockList;
        u32                 memSize;
        u32                 memAllocated;
    };
} //namespace gos


#endif // _gosAllocTracker_h_
