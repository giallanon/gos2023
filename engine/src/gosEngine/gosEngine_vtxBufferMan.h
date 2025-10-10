#ifndef _gosEngine_vtxBufferMan_h_
#define _gosEngine_vtxBufferMan_h_
#include "gosEngineEnumAndDefine.h"
#include "../gos/gosFastArray.h"
#include "../gos/gosFreespaceTracker.h"


namespace gos
{
    namespace engine
    {
        class VtxBufferMan
        {
        public:
                    VtxBufferMan();
                    ~VtxBufferMan()                                                                                     { }

            void    setup (gos::Allocator *allocator, GPU *gpu);
            void    unsetup();

                    /* ritorna in <out_offset> l'offset a partire da quale e' stato allocato un blocco con dimensioni
                     * pari ad almeno <sizeInByte> byte */
            bool    reserve (u32 sizeInByte, u32 *out_offset, u32 *out_size, GPUVtxBufferHandle *out_handle);
            void    release (GPUVtxBufferHandle handle, u32 offset, u32 size);

        private:
            struct sElem
            {
                GPUVtxBufferHandle  handle;
                FreespaceTracker    *tracker;
            };

        private:
            bool    priv_allocNewBuffer();

        private:
            gos::Allocator      *allocator;
            GPU                 *gpu;
            FastArray<sElem>    list;
        };
    } //namespace engine
} //namespace gos


#endif //_gosEngine_vtxBufferMan_h_

