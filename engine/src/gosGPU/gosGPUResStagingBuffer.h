#ifndef _gosGPUResStagingBuffer_h_
#define _gosGPUResStagingBuffer_h_
#include "gosGPUEnumAndDefine.h"


namespace gos
{
    namespace gpu
    {
        /****************************************************
         * VtxBuffer
         * 
         * struttura interna accessibile tramite i metodi di GPU utilizzando uno GPUStgBufferHandle
         */
        struct StagingBuffer
        {
        public:
                            StagingBuffer()                        { reset(); }

            void            reset ()
                            {
                                vkHandle = VK_NULL_HANDLE;
                                vkMemHandle = VK_NULL_HANDLE;
                                allocatedSizeInByte = 0;

                                mapped_pt = NULL;
                                mapped_size = 0;
                                mapped_offset = 0;
                            }


        public:
            VkBuffer        vkHandle;
            VkDeviceMemory  vkMemHandle;
            u32             allocatedSizeInByte;
            
            void            *mapped_pt;
            u32             mapped_size;
            u32             mapped_offset;
        };

    } //namespace gpu
} //namespace gos


#endif //_gosGPUResStagingBuffer_h_