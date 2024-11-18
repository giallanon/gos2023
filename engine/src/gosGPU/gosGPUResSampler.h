#ifndef _gosGPUResVSampler_h_
#define _gosGPUResVSampler_h_
#include "gosGPUEnumAndDefine.h"
#include "../gos/gosUtils.h"

namespace gos
{
    namespace gpu
    {
        class SamplerDesc
        {
        public:
            SamplerDesc()   { reset(); }


            bool    operator== (const SamplerDesc &b) const
            {
                if (minFilter != b.minFilter) return false;
                if (magFilter != b.magFilter) return false;
                if (mipFilter != b.mipFilter) return false;
                if (compareFn != b.compareFn) return false;
                if (addressModeU != b.addressModeU) return false;
                if (addressModeV != b.addressModeV) return false;
                if (addressModeW != b.addressModeW) return false;
                if (bAnisotropic != b.bAnisotropic) return false;
                return true;
            }

            void reset()
            {
                minFilter = eSamplerFilter::linear;
                magFilter = eSamplerFilter::linear;
                mipFilter = eSamplerMipFilter::linear;
                compareFn = eSamplerCompFunc::DISABLED;
                addressModeU = eSamplerAddressMode::REPEAT;
                addressModeV = eSamplerAddressMode::REPEAT;
                addressModeW = eSamplerAddressMode::REPEAT;
                bAnisotropic = true;
            }

            u32     toU32() const
            {
                assert ((u32)minFilter < 4);
                assert ((u32)magFilter < 4);
                assert ((u32)mipFilter < 4);
                assert ((u32)compareFn < 16);
                assert ((u32)addressModeU < 8);
                assert ((u32)addressModeV < 8);
                assert ((u32)addressModeW < 8);

                u32 ret = (u32)minFilter;
                ret |= ((u32)magFilter << 2);
                ret |= ((u32)mipFilter << 4);
                ret |= ((u32)compareFn << 6);
                ret |= ((u32)addressModeU << 10);
                ret |= ((u32)addressModeV << 13);
                ret |= ((u32)addressModeW << 16);
                if (bAnisotropic)
                    ret |= (0x00000001 << 19);
                return ret;
            }

        public:
            eSamplerFilter      minFilter;          //2bit
            eSamplerFilter      magFilter;          //2bit
            eSamplerMipFilter   mipFilter;          //2bit
            eSamplerCompFunc    compareFn;          //4bit
            eSamplerAddressMode addressModeU;       //3bit
            eSamplerAddressMode addressModeV;       //3bit
            eSamplerAddressMode addressModeW;       //3bit
            bool                bAnisotropic;       //1bit
        };

        /**
         * @brief Sampler
         * 
         * Struttura interna accessibile tramite i metodi di GPU utilizzando uno GPUSamplerlHandle
         */
        class Sampler
        {
        public:
            void                reset()     { vkHandle = VK_NULL_HANDLE; }
            

        public:
            VkSampler           vkHandle;
            SamplerDesc         desc;
        };        

        

    } //namespace gpu
} //namespace gos


#endif //_gosGPUResVSampler_h_
