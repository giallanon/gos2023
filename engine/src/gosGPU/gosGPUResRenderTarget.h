#ifndef _gosGPUResRenderTarget_h_
#define _gosGPUResRenderTarget_h_
#include "gosGPUEnumAndDefine.h"
#include "../gos/dataTypes/gosPosDim2D.h"

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
                                vkMemHandle = VK_NULL_HANDLE;
                                viewAsRT = VK_NULL_HANDLE;
                                viewAsTexture = VK_NULL_HANDLE;
                            }

            void            resolve (i16 w, i16 h)                     
                            {
                                resolvedW = width.resolve (0, w);
                                resolvedH = height.resolve (0, h);
                            }                            

        public:
            gos::Dim2D      width;
            gos::Dim2D      height;
            u32             resolvedW;
            u32             resolvedH;

            VkFormat        format;
            VkImage         image;
            VkDeviceMemory  vkMemHandle;
            VkImageView     viewAsRT;
            VkImageView     viewAsTexture;
        };

    } //namespace gpu
} //namespace gos


#endif //_gosGPUResRenderTarget_h_