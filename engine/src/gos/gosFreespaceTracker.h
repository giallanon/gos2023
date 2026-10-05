#ifndef _gosFreespaceTracker_h_
#define _gosFreespaceTracker_h_
#include "gosFastArray.h"


namespace gos
{
    /*******************************
     * @brief   FreespaceTracker
     *          Gestisce un ipotetico buffer di dimensioni pari a <totalSizeToHandle> byte e lo suddivide in slota
     *          da <sizeof_block> byte ciascuno.
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
                ~FreespaceTracker()									{ unsetup(); }

        void    setup (u32 totalSizeToHandle, u32 sizeof_block, gos::Allocator *allocator = NULL);
		void	unsetup();
        
        bool    alloc (u32 sizeInByte, AllocInfo *out);
        void    free (AllocInfo &info);

		u32		get_num_free_bytes() const 								{ return (num_max_blocks - num_allocated_block) * sizeof_block; }
		u32		get_num_allocated_bytes() const 						{ return num_allocated_block * sizeof_block; }

    private:
        struct sRange
        {
            u32 startingBlock;
            u32 numBlock;
        };

    private:
        void    priv_removeEntry (u32 at);
        void    priv_tryMerge (u32 iEntry);
		u32    	priv__free (AllocInfo &info);

    private:
		gos::Allocator			*localAllocator;
        gos::FastArray<sRange>  freerangeList;
        u32                     sizeof_block;
        u32                     num_max_blocks;
		u32						num_allocated_block;

    }; //FreespaceTracker

} //namespace gos

#endif //_gosFreespaceTracker_h_

