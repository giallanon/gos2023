#ifndef _gosGPUResFrameBuffer_h_
#define _gosGPUResFrameBuffer_h_
#include "gosGPUEnumAndDefine.h"
#include "../gos/dataTypes/gosPosDim2D.h"

namespace gos
{
    namespace gpu
    {
        /****************************************************
         * FrameBuffer
         * 
         * struttura interna accessibile tramite i metodi di GPU utilizzando uno GPUDFrameBufferHandle
         */
        struct FrameBuffer
        {
        public:
                            FrameBuffer()                        { reset(); }

            void            reset ()
                            {
                                width = 0;
                                height = 0;
                                resolvedW = resolvedH = 0;
                                boundToSwapChain = false;
                                boundToMainRT = false;
                                memset (vkFrameBufferList, 0, sizeof(vkFrameBufferList));
                                renderLayoutHandle.setInvalid();
                                depthStencilHandle.setInvalid();
                                numRenderTaget = 0;
                            }

            void            resolve (i16 w, i16 h)                     
                            {
                                resolvedW = width.resolve (0, w);
                                resolvedH = height.resolve (0, h);
                            }                              

        public:
            bool    boundToSwapChain;   //se si, va ricreato quando la swapchain viene ricreata
            bool    boundToMainRT;      //se si, contiene N VkFramebuffer, uno per ogni image della swapchain

            //info per Vulkan
            u32                     resolvedW;
            u32                     resolvedH;
            VkFramebuffer           vkFrameBufferList[SWAPCHAIN_NUM_MAX_IMAGES];    //ne basterebbe 1 solo in teoria, ma se ci bindiamo
                                                                                    //al mainRT ne devo usare 1 per ogni swapchain imahe
            //info di creazione
            gos::Dim2D              width;
            gos::Dim2D              height;
            GPURenderLayoutHandle   renderLayoutHandle;
            GPUDepthStencilHandle   depthStencilHandle;
            u32                     numRenderTaget;
            GPURenderTargetHandle   renderTargetHandleList[GOSGPU__NUM_MAX_RENDER_TARGET];    //elenco dei RT da usare
        };

    } //namespace gpu
} //namespace gos



#endif //_gosGPUResFrameBuffer_h_