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
                                vkPool = VK_NULL_HANDLE;
                                whichQ = eGPUQueueFamily::unknown;
                            }


        public:
            VkCommandBuffer vkHandle;
            VkCommandPool   vkPool;
            eGPUQueueFamily   whichQ;
        };

    } //namespace gpu
} //namespace gos


#endif //_gosGPUResCommandBuffer_h_