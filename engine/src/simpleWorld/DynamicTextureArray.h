#ifndef _DynamicTextureArray_h_
#define _DynamicTextureArray_h_
#include "gosGPU.h"
#include "../gos/gosHashMap.h"
#include "BitmaskedFixedArray.h"

/**
 * @brief DynamicTextureArray
 * Mantiene una lista di GPUTextureHandle.
 * Ad ogni texture "added" viene associato un index, che e' il primo indice libero dell'array di texture.
 * Eventuali doppioni di GPUTextureHandle vengono mantenuti univoci all'interno dell'array (vedi addIfNotExists)
 * 
 * Durante "unsetup" fa il free delle texture.
 */
class DynamicTextureArray
{
public:
            DynamicTextureArray();
            ~DynamicTextureArray()                                          { unsetup(); }

    bool    setup (gos::Allocator *allocator, gos::GPU *gpu, u16 numMaxElements);
    void    unsetup ();

    /**
     * @brief inserisce [hTexture] nell'array.
     * Ritorna 0 se non c'è più spazio nell'array e quindi [hTexture] non e' stata inserita.
     * Ritorna 1 se [hTexture] non esisteva gia' nell'array.
     * Ritorna 2 se [hTexture] esisteva gia.
     * Se il codice di ritorno e' != 0, allora in out_index] ritorna la posizione di [hTexture] all'interno dell'array.
     */
    u16    addIfNotExists (const GPUTextureHandle &hTexture, u16 *out_index);


    /**
     * @brief rimuove l'elemento in posizione [index]
     */
    void    remove (u16 index, bool bAlsoDeleteTexture);
    
    
    /**
     * @brief ritorna true se [index] e' un valido elemento nel qual caso
     * filla [out_hTexture] con la texture relativa.
     * Ritorna false altrimenti
     */
    bool    getInfo (u16 index, const GPUTextureHandle *out_hTexture) const;

private:
    gos::Allocator                          *localAllocator;
    gos::GPU                                *gpu;
    BitmaskedFixedArray<GPUTextureHandle>   list;
    gos::HashMap<u32,u16>                   hashMap;
    u16                                     numMaxElem;
};



#endif //_DynamicTextureArray_h_
