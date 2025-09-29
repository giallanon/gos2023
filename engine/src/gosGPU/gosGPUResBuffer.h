#ifndef _gosGPUResBuffer_h_
#define _gosGPUResBuffer_h_
#include "gosGPUEnumAndDefine.h"


namespace gos
{
    namespace gpu
    {
        /****************************************************
         * Buffer
         * 
         * struttura interna accessibile tramite i metodi di GPU utilizzando un
         *   GPUUniformBufferHandle
         *   GPUStorageBufferHandle
         *   GPUVtxBufferHandle
         *   GPUIdxBufferHandle
         *   GPUStgBufferHandle
         */
        struct Buffer
        {
        public:
                            Buffer()                        { reset(); }

            void            reset ()
                            {
                                vkHandle = VK_NULL_HANDLE;
                                _vkMemHandle = VK_NULL_HANDLE;
                                mapped_host_pt = NULL;
                                mapped_offset = 0;
                                mapped_size = 0;
                                bufferSize = 0;
                                mode = eMemAccessMode::invalid;
                                memoryAllocated = 0;
                            }


        public:
            VkBuffer        vkHandle;
            VkDeviceMemory  _vkMemHandle;
            eMemAccessMode   mode;
            u64             memoryAllocated;

            u32             bufferSize;
            u32             mapped_offset;
            u32             mapped_size;
            u8              *mapped_host_pt;    //se HOST_VISIBLE | HOST_COHERENT, questo pt punta alla zona di memoria in CPU. Viene gestito in automatico da map()/unmap()

        };

    } //namespace gpu
} //namespace gos


#endif //_gosGPUResBuffer_h_