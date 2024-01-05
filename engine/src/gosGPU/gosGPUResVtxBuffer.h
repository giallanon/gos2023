#ifndef _gosGPUResVtxBuffer_h_
#define _gosGPUResVtxBuffer_h_
#include "gosGPUEnumAndDefine.h"


namespace gos
{
    namespace gpu
    {
        /****************************************************
         * VtxBuffer
         * 
         * struttura interna accessibile tramite i metodi di GPU utilizzando uno GPUVtxBufferHandle
         */
        struct VtxBuffer
        {
        public:
                            VtxBuffer()                        { reset(); }

            void            reset ()
                            {
                                vkHandle = VK_NULL_HANDLE;
                                vkMemHandle = VK_NULL_HANDLE;
                            }


        public:
            VkBuffer        vkHandle;
            VkDeviceMemory  vkMemHandle;
        };

    } //namespace gpu
} //namespace gos


#endif //_gosGPUResVtxBuffer_h_