#ifndef _gosStringList_h_
#define _gosStringList_h_
#include "../gosBufferLinear.h"

namespace gos
{
    /*************************************************
     * StringList
     * 
     * Semplice buffer contiguo autoespandibile che contiene un elenco
     * di stringhe
    */
    class StringList
    {
    public:
                    StringList ()                                           { cursor=0; count=0; }
                    StringList (gos::Allocator *allocatorIN, u32 size=512)  { setup (allocatorIN, size); }
                    ~StringList ()                                          { unsetup(); }

        void        setup (gos::Allocator *allocatorIN, u32 size)           { buffer.setup(allocatorIN, size); reset(); }
        void        unsetup()                                               { buffer.unsetup(); }

        void        reset()                                                 { cursor=0; count=0; buffer.zero(); }
        void        add (const char *m);
        void        add (const u8 *m);

        u32         getNumString() const                                    { return count; }
        void        toStart (u32 *iter) const                               { (*iter) = 0;};
        const char* nextAsChar(u32 *iter) const;
        const u8*   nextAsU8(u32 *iter) const;

    private:
        void        priv_doAdd (const void *m, u32 sizeInByte);

    private:
        gos::BufferLinear   buffer;
        u32                 cursor;
        u32                 count;
    };
} //namespace gos

#endif //_gosStringList_h_