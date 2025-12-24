#ifndef _gosUniqueStringList_h_
#define _gosUniqueStringList_h_
#include "gosStringList.h"
#include "../gosHashMap.h"
#include "../gosUtils.h"

namespace gos
{
    /**
     * @brief UniqueStringList
     * 
     * Semplice buffer contiguo autoespandibile che contiene un elenco di stringhe
     * che viene mantenuto univoco grazie ad una HashMap.
     * Provare ad inserire N volte la stessa stringa risulta sempre nello stesso offset ritornato
    */
    class UniqueStringList
    {
    public:
                    UniqueStringList () : list()                                                { }
                    UniqueStringList (gos::Allocator *allocatorIN, u32 size=512)  : list()      { setup (allocatorIN, size); }
                    ~UniqueStringList ()                                                        { unsetup(); }

        void        setup (gos::Allocator *allocatorIN, u32 size)                               { list.setup (allocatorIN, size); hashMap.setup (allocatorIN, 256); reset(); }
        void        unsetup()                                                                   { list.unsetup(); hashMap.unsetup(); }

        void        reset()                                                                     { list.reset(); hashMap.reset(); }

                    /**
                     * @brief aggiunge una stringa (solo se non esiste gia') e ritorna un offset utilizzabile per puntare alla stringa
                     * e recuperarla con getStringAtOffset()
                     */
        u32         add (const char *m)
                    {
                        const u32 key = utils::crc32 (m);

                        FastHashMap<u32,u32>::Position pos;
                        u32 offset;
                        if (!hashMap.findWithPos (key, &offset, &pos))
                        {
                            offset = list.add (m);
                            hashMap.insertInPosition (pos, offset);
                        }

                        return offset;
                    }

        const char* getStringAtOffset (u32 offset) const                    { return list.getStringAtOffset(offset); }

        u32         getNumString() const                                    { return list.getNumString(); }
        void        toStart (u32 *iter) const                               { list.toStart (iter); }
        const char* next (u32 *iter) const                                  { return list.next(iter); }

    private:
        StringList              list;
        FastHashMap<u32,u32>    hashMap;
    };
} //namespace gos

#endif //_gosUniqueStringList_h_