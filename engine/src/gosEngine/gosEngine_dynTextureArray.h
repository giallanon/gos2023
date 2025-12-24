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
                    DynamicTextureArray();
                    ~DynamicTextureArray()                                      { unsetup(); }

            void    setup (gos::Allocator *allocator, u32 num_max_texture);
            void    unsetup();

            u32     addIfNotExitst (GPUTextureHandle texHandle, bool *out_canBeNULL_wasNew);
            void    remove (GPUTextureHandle texHandle);

            bool    find (GPUTextureHandle texHandle, u32 *out_index) const;

        private:
            gos::Bitfield                           bitmask;
            gos::FastHashMap<GPUTextureHandle, u32> hashMap;
            u32                                     num_max_texture;
        };

    } //namespace engine
} //namespace gos



#endif //_gosEngine_dynTextureArray_h_
