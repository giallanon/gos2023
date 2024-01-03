#ifndef _RenderPass_h_
#define _RenderPass_h_
#include "gosGPU.h"


class RenderPass
{
public:
                    RenderPass();
                    ~RenderPass();

    RenderPass&     requireBegin();
    RenderPass&     requireColorBuffer (VkFormat imageFormat, bool bClear, const gos::ColorHDR &clearColor = gos::ColorHDR(1.0f, 1, 1, 1) );
    //RenderPass&     requireDepthStencil (...); 
    RenderPass&     requireEnd();

    void            bindColorBuffer (u8 index, vkImageView &imageView);
    void            bindColorBufferToCurrentSwapChainImage (u8 index);
    //void            bindDepthBuffer (vkImageView &imageView);



private:

}; //class RenderPass

#endif //_RenderPass_h_
