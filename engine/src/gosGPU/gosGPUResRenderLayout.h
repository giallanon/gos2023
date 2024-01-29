#ifndef _gosGPUResRenderLayout_h_
#define _gosGPUResRenderLayout_h_
#include "gosGPUEnumAndDefine.h"


namespace gos
{
    namespace gpu
    {
        /****************************************************
         * RenderLayout
         * 
         * struttura interna accessibile tramite i metodi di GPU utilizzando uno GPURenderLayoutHandle
         */
        struct RenderLayout
        {
        public:
                            RenderLayout()                        { reset(); }

            void            reset ()
                            {
                                vkRenderPassHandle = VK_NULL_HANDLE;
                                numAttachment = 0;
                                numColorBuffer = 0;
                                indexOfDepthStencilBuffer = 0xFF;
                                flagNeedClear = 0;
                            }

            bool            doesNeedClear (u8 attachmentIndex) const        { return (flagNeedClear & (0x00000001<<attachmentIndex)) != 0; }
            bool            hasDepthStencil() const                         { return (indexOfDepthStencilBuffer != 0xFF); }

        public:
            VkRenderPass    vkRenderPassHandle;
            u8              numAttachment;
            u8              numColorBuffer;
            u8              indexOfDepthStencilBuffer;
            u32             flagNeedClear;              //ogni bit indica se il relativo attachment vuole un clear color
        };

    } //namespace gpu
} //namespace gos


#endif //_gosGPUResRenderLayout_h_