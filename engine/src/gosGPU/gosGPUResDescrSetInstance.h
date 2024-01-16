#ifndef _gosGPUResDescrSetInstance_h_
#define _gosGPUResDescrSetInstance_h_
#include "gosGPUEnumAndDefine.h"


namespace gos
{
    namespace gpu
    {
        /****************************************************
         * DescrSetInstance
         * 
         * struttura interna accessibile tramite i metodi di GPU utilizzando uno GPUDescrSetInstancerHandle
         * E' di fatto una zona di memoria allocata da un DescrPool e descritta da un DescrSetLayout
         */
        struct DescrSetInstance
        {
        public:
                            DescrSetInstance()                        { reset(); }

            void            reset ()
                            {
                                vkHandle = VK_NULL_HANDLE;
                                vkPoolHandle = VK_NULL_HANDLE;
                                bCanBeFreed = false;
                            }


        public:
            VkDescriptorSet     vkHandle;
            VkDescriptorPool    vkPoolHandle;
            bool                bCanBeFreed;    //solo se creati da Pool con VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT allora posso farne il free
        };

    } //namespace gpu
} //namespace gos


#endif //_gosGPUResDescrSetInstance_h_