#ifndef _gosGPUResTexture_h_
#define _gosGPUResTexture_h_
#include "gosGPUEnumAndDefine.h"


namespace gos
{
    namespace gpu
    {
        /**
         * @brief Texture
         * 
         * struttura interna accessibile tramite i metodi di GPU utilizzando uno GPUTextureHandle
         */
        struct Texture
        {
        public:
                            Texture()                        { reset(); }

            void            reset ()
                            {
                                vkHandle = VK_NULL_HANDLE;
                                vkMemHandle = VK_NULL_HANDLE;
                                view = VK_NULL_HANDLE;
                                dimx = dimy = 0;
                                nMipMap = 0;
                                nArray = 0;
                                memoryAllocated = 0;
                            }


        public:
            VkImage         vkHandle;
            VkDeviceMemory  vkMemHandle;
            VkImageView     view;
            u32             memoryAllocated;
            u16             dimx;
            u16             dimy;
            u8              nMipMap;
            u8              nArray;
        };

    } //namespace gpu
} //namespace gos


#endif //_gosGPUResTexture_h_