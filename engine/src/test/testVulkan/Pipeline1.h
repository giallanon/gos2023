#ifndef _Pipeline1_h_
#define _Pipeline1_h_
#include "gosGPU.h"


class Pipeline1
{
public:
                    Pipeline1()                   { priv_reset(); }

    bool            create (gos::GPU *gpu, const GPUVtxDeclHandle vtxDeclHandle, eDrawPrimitive drawPrimitive);

    void            destroy (gos::GPU *gpu);
    bool            recreateFrameBuffers (gos::GPU *gpu);

    VkRenderPass    getRenderPassHandle() const                 { return renderPassHandle; }
    VkFramebuffer   getFrameBufferHandle(u8 imageIndex)         { return frameBufferHandleList[imageIndex]; }

private:
    void            priv_reset();
    bool            priv_createRenderPass (gos::GPU *gpu);
    void            priv_destroyFrameBuffers (gos::GPU *gpu);

public:
    VkPipeline          pipeHandle;
    VkPipelineLayout    _layoutHandle;
    VkRenderPass        renderPassHandle;
    VkFramebuffer       frameBufferHandleList[SWAPCHAIN_NUM_MAX_IMAGES];

};

#endif //_Pipeline1_h_