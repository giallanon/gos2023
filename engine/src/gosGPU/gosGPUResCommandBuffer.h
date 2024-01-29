#ifndef _gosGPUResCommandBuffer_h_
#define _gosGPUResCommandBuffer_h_
#include "gosGPUEnumAndDefine.h"


namespace gos
{
    namespace gpu
    {
        /****************************************************
         * CommandBuffer
         * 
         * struttura interna accessibile tramite i metodi di GPU utilizzando uno GPUCommandBufferHandle
         */
        struct CommandBuffer
        {
        public:
                            CommandBuffer()                        { reset(); }

            void            reset ()
                            {
                                vkHandle = VK_NULL_HANDLE;
                                whichQ = eGPUQueueType::unknown;
                            }


        public:
            VkCommandBuffer vkHandle;
            eGPUQueueType   whichQ;
        };

    } //namespace gpu
} //namespace gos


#endif //_gosGPUResCommandBuffer_h_