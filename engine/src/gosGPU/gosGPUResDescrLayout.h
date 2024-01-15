#ifndef _gosGPUResDescLayout_h_
#define _gosGPUResDescLayout_h_
#include "gosGPUEnumAndDefine.h"


namespace gos
{
    namespace gpu
    {
        /****************************************************
         * DescrLayout
         * 
         * struttura interna accessibile tramite i metodi di GPU utilizzando uno GPUDescrLayoutrHandle
         */
        struct DescrLayout
        {
        public:
                            DescrLayout()                        { reset(); }

            void            reset ()
                            {
                                vkHandle = VK_NULL_HANDLE;
                            }


        public:
            VkDescriptorSetLayout        vkHandle;
        };

    } //namespace gpu
} //namespace gos


#endif //_gosGPUResDescLayout_h_