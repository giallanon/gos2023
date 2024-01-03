#ifndef _gosDepthStencil_h_
#define _gosDepthStencil_h_
#include "gosGPUEnumAndDefine.h"
#include "../gos/dataTypes/gosPosDim2D.h"

namespace gos
{
    namespace gpu
    {
        /****************************************************
         * DepthStencil
         * 
         * struttura interna accessibile tramite i metodi di GPU utilizzando uno GPUDepthStencilHandle
         * La dimensione di un DepthStencil puo' essere legata alla dimensione della viewport
         */
        struct DepthStencil
        {
        public:
                            DepthStencil()                        { reset(); }

            void            reset ()
                            {
                                resolvedW = resolvedH = 0;
                                bHasStencil = false;
                                depthFormat = VK_FORMAT_UNDEFINED;
                                image = VK_NULL_HANDLE;
                                mem = VK_NULL_HANDLE;
                                view = VK_NULL_HANDLE;
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

            bool            bHasStencil;
            VkFormat        depthFormat;
            VkImage         image;
            VkDeviceMemory  mem;
            VkImageView     view;
        };

    } //namespace gpu
} //namespace gos

#endif //_gosDepthStencil_h_
