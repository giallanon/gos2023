#ifndef _gosGPUResRenderTarget_h_
#define _gosGPUResRenderTarget_h_
#include "gosGPUEnumAndDefine.h"

namespace gos
{
    namespace gpu
    {
        /****************************************************
         * RenderTarget
         * 
         * struttura interna accessibile tramite i metodi di GPU utilizzando uno GPURenderTargetHandle
         */
        struct RenderTarget
        {
        public:
                            RenderTarget()                        { reset(); }

            void            reset ()
                            {
                                width = height = 0;
                                format = VK_FORMAT_UNDEFINED;
                                image = VK_NULL_HANDLE;
                                mem = VK_NULL_HANDLE;
                                viewAsRT = VK_NULL_HANDLE;
                                viewAsTexture = VK_NULL_HANDLE;
                            }

        public:
            u32             width;
            u32             height;
            VkFormat        format;
            VkImage         image;
            VkDeviceMemory  mem;
            VkImageView     viewAsRT;
            VkImageView     viewAsTexture;
        };

    } //namespace gpu
} //namespace gos


#endif //_gosGPUResRenderTarget_h_