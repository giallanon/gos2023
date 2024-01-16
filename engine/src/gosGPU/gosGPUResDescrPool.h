#ifndef _gosGPUResDescrPool_h_
#define _gosGPUResDescrPool_h_
#include "gosGPUEnumAndDefine.h"


namespace gos
{
    namespace gpu
    {
        /****************************************************
         * DescrPool
         * 
         * struttura interna accessibile tramite i metodi di GPU utilizzando uno GPUDescrPoolHandle
         */
        struct DescrPool
        {
        public:
                            DescrPool()                        { reset(); }

            void            reset ()
                            {
                                vkHandle = VK_NULL_HANDLE;
                                flags = 0;
                            }


        public:
            VkDescriptorPool                vkHandle;
            VkDescriptorPoolCreateFlags     flags;
        };

    } //namespace gpu
} //namespace gos


#endif //_gosGPUResDescrPool_h_