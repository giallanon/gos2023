#ifndef _gosEngine_dynTextureArray_h_
#define _gosEngine_dynTextureArray_h_
#include "gosEngineEnumAndDefine.h"
#include "../gos/gosBit.h"

namespace gos
{
    namespace engine
    {
        class DynamicTextureArray
        {
        public:
            static constexpr u8 NUM_RESERVED    = 16;

        public:
                    DynamicTextureArray();
                    ~DynamicTextureArray()                                      { unsetup(); }

            void    setup (gos::Allocator *allocator, u32 num_max_texture);
            void    unsetup();

                    //l'id ritornato da questa fn e' sempre >= <NUM_RESERVED>
            u32     add_if_dont_exists (GPUTextureHandle texHandle, bool *out_canBeNULL_wasNew);
            
            void    remove (GPUTextureHandle texHandle);

                    //le prime <NUM_RESERVED> sono speciali e possono essere settate solo usando
                    //questa fn.
            bool    add_reserved (GPUTextureHandle texHandle, u32 reserved_index);

            bool    find (GPUTextureHandle texHandle, u32 *out_index) const;

        private:
            gos::Bitfield                           bitmask;
            gos::FastHashMap<GPUTextureHandle, u32> hashMap;
            u32                                     num_max_texture;
        };

    } //namespace engine
} //namespace gos



#endif //_gosEngine_dynTextureArray_h_
