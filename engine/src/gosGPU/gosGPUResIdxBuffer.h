#ifndef _gosGPUResIdxBuffer_h_
#define _gosGPUResIdxBuffer_h_
#include "gosGPUEnumAndDefine.h"


namespace gos
{
    namespace gpu
    {
        /****************************************************
         * IdxBuffer
         * 
         * struttura interna accessibile tramite i metodi di GPU utilizzando uno GPUIdxBufferHandle
         */
        struct IdxBuffer
        {
        public:
                            IdxBuffer()                        { reset(); }

            void            reset ()
                            {
                                vkHandle = VK_NULL_HANDLE;
                                vkMemHandle = VK_NULL_HANDLE;
                                mode = eVIBufferMode::unknown;
                            }


        public:
            VkBuffer        vkHandle;
            VkDeviceMemory  vkMemHandle;
            eVIBufferMode   mode;
        };

    } //namespace gpu
} //namespace gos


#endif //_gosGPUResIdxBuffer_h_