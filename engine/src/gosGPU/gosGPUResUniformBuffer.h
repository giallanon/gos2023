#ifndef _gosGPUResUniformBuffer_h_
#define _gosGPUResUniformBuffer_h_
#include "gosGPUEnumAndDefine.h"


namespace gos
{
    namespace gpu
    {
        /****************************************************
         * UniformBuffer
         * 
         * struttura interna accessibile tramite i metodi di GPU utilizzando uno GPUUniformBufferHandle
         */
        struct UniformBuffer
        {
        public:
                            UniformBuffer()                        { reset(); }

            void            reset ()
                            {
                                vkHandle = VK_NULL_HANDLE;
                                vkMemHandle = VK_NULL_HANDLE;
                                mapped_host_pt = NULL;
                                mapped_offset = 0;
                                bufferSize = 0;
                            }


        public:
            VkBuffer        vkHandle;
            VkDeviceMemory  vkMemHandle;

            u32             bufferSize;
            void            *mapped_host_pt;   //se HOST_VISIBLE | HOST_COHERENT, questo pt punta alla zona di memoria in CPU. Viene gestito in automatico da map()/unmap()
            u32             mapped_offset;       //il pt [mapped_host_pt] mappa a partire da [mapped_offset]

        };

    } //namespace gpu
} //namespace gos


#endif //_gosGPUResUniformBuffer_h_