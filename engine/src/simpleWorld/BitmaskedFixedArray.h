#ifndef _BitmaskedFixedArray_h_
#define _BitmaskedFixedArray_h_
#include "../gos/gosHashMap.h"
#include "../gos/gosBit.h"

/**
 * @brief BitmaskedFixedArray
 * 
 * E' un array di <NELEM> elementi di tipo <OBJECT>
 * Mantiene un bitfield per sapere quali di questi elementi e' attualmente allocato o libero
 *  
 */
template<class OBJECT>
class BitmaskedFixedArray
{
public:
            BitmaskedFixedArray()       { localAllocator = NULL; }
            ~BitmaskedFixedArray()      { }

    bool    setup (gos::Allocator *allocator, u16 numMaxElements)
    {
        assert (NULL == localAllocator);
        localAllocator = allocator;
        
        list = GOSALLOCT(OBJECT*, localAllocator, sizeof(OBJECT) * numMaxElements);
        bitfield.setup (localAllocator, numMaxElements);
        
        reset();
        return true;
    }

    void    unsetup ()
    {
        if (NULL != localAllocator)
        {
            GOSFREE (localAllocator, list);
            bitfield.unsetup (localAllocator);
        }
        localAllocator = NULL;    
    }    

    void    reset()                                         { bitfield.zero(); }

    /**
     * @brief aggiuge [obj] all'array nel primo elemento libero.
     * Ritorna true in caso di successo e filla [out_index] con la posizione che e' stata assegnata a [obj] all'interno dell'array
     * Ritorna false altrimenti
     */    
    bool    add (const OBJECT &obj, u16 *out_index)
    {
        u32 index;
        if (bitfield.findAndSetFirstFreeBit(&index))
        {            
            list[index] = obj;
            *out_index = index;
            return true;
        }
            
        DBGBREAK
        return false;
    }

    /**
     * @brief libera l'elemento in posizione [index]
     */
    bool    remove (u16 index)
    {
        if (bitfield.isBitSet(index))
        {
            bitfield.clear (index);
            return true;
        }
        return false;
    }
        
    
    bool    get (const u16 index, const OBJECT **out_obj) const
            {
                if (index >= bitfield.getNumMaxBit())
                    return false;
                
                *out_obj = &list[index];
                return true;
            }

   bool     get (const u16 index, OBJECT **out_obj)
            {
                if (index >= bitfield.getNumMaxBit())
                    return false;
                
                *out_obj = &list[index];
                return true;
            }            

    void    toStart (u32 *iter) const                                   { *iter = 0; }

    bool    next (u32 *iter, const OBJECT **out_obj) const
    {
        u32 index;
        if (!bitfield.findFirstSetBit (*iter, &index))
            return false;

        *iter = index+1;
        *out_obj = &list[index];
        return true;
    }

private:
    gos::Allocator          *localAllocator;
    OBJECT                  *list;
    gos::Bitfield           bitfield;
};



#endif //_DynamicTextureArray_h_
