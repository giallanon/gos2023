#ifndef _gosGPUResDescrSetLayout_h_
#define _gosGPUResDescrSetLayout_h_
#include "gosGPUEnumAndDefine.h"


namespace gos
{
    namespace gpu
    {
        /****************************************************
         * DescrSetLayout
         * 
         * struttura interna accessibile tramite i metodi di GPU utilizzando uno GPUDescrSetLayoutrHandle
         */
        struct DescrSetLayout
        {
        public:
                            DescrSetLayout()                        { reset(); }

            void            reset ()
                            {
                                vkHandle = VK_NULL_HANDLE;
                            }


        public:
            VkDescriptorSetLayout        vkHandle;
        };

    } //namespace gpu
} //namespace gos


#endif //_gosGPUResDescrSetLayout_h_