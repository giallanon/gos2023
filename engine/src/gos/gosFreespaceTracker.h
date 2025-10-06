#ifndef _gosFreespaceTracker_h_
#define _gosFreespaceTracker_h_
#include "gosFastArray.h"


namespace gos
{
    /*******************************
     * @brief   FreespaceTracker
     *          Gestisce un ipotetico buffer di dimensioni pari a <totalSizeToHandle> byte e lo suddivide in slota
     *          da <sizeofABlock> byte ciascuno.
     *          Tramite alloc() e' possibile riservare un certo numero di byte (internamente riserva un certo numero di slot)
     */
    class FreespaceTracker
    {
    public:
        struct AllocInfo
        {
            u32 offset;
            u32 size;
        };

    public:
                FreespaceTracker();
                ~FreespaceTracker();

        void    setup (u32 totalSizeToHandle, u32 sizeofABlock);
        
        bool    alloc (u32 sizeInByte, AllocInfo *out);
        void    free (AllocInfo &info);

    private:
        struct sRange
        {
            u32 startingBlock;
            u32 numBlock;
        };

    private:
        void    priv_removeEntry (u32 at);
        void    priv_tryMerge (u32 iEntry);

    private:
        gos::FastArray<sRange>  freerangeList;
        u32                     sizeofABlock;
        u32                     numMaxBlocks;

    }; //FreespaceTracker

} //namespace gos

#endif //_gosFreespaceTracker_h_

