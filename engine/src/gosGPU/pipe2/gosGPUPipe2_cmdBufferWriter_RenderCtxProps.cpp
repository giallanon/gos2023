#include "gosGPUPipe2_cmdBufferWriter.h"
#include "../gosGPU.h"

using namespace gos;
using namespace gos::gpu;


//*****************************************
void RenderCtx::Props::reset ()
{
    numColorAttachments = 0;
    haveZB = 0;
    renderAreaW = renderAreaH = 0;
}

//*****************************************
void RenderCtx::Props::withRenderArea (u32 w, u32 h)
{
    renderAreaW = w;
    renderAreaH = h;
}

//*****************************************
void RenderCtx::Props::withRenderArea (GPU *gpu, const GPURenderTargetHandle &rtHandle)
{
    const gpu::RenderTarget *rt_info = gpu->get_info (rtHandle);
    assert (NULL != rt_info);
    withRenderArea (rt_info->resolvedW, rt_info->resolvedH);
}

//*****************************************
void RenderCtx::Props::withRT (const VkImageView &sourceIMG, eAttachmentLoadOp loadOp, eAttachmentStoreOp storeOp, const gos::ColorHDR &color)
{
    assert (numColorAttachments < GOSGPU__NUM_MAX_ATTACHMENT);
    VkRenderingAttachmentInfo *p = &colorAttachList[numColorAttachments++];

    memset (p, 0, sizeof(VkRenderingAttachmentInfo));
    p->sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    p->imageView = sourceIMG;
    p->imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
    p->loadOp = gpu::toVulkan(loadOp);
    p->storeOp = gpu::toVulkan(storeOp);
    p->clearValue.color.float32[0] = color.col.r;
    p->clearValue.color.float32[1] = color.col.g;
    p->clearValue.color.float32[2] = color.col.b;
    p->clearValue.color.float32[3] = color.col.a;
}

//*****************************************
void RenderCtx::Props::withRT (GPU *gpu, const GPURenderTargetHandle &rtHandle, eAttachmentLoadOp loadOp, eAttachmentStoreOp storeOp, const gos::ColorHDR &color)
{
    const gpu::RenderTarget *rt_info = gpu->get_info (rtHandle);
    assert (NULL != rt_info);
    withRT (rt_info->view, loadOp, storeOp, color);
}

//*****************************************
void RenderCtx::Props::withZB (GPU *gpu, const GPUZBufferHandle &zbHandle, eAttachmentLoadOp loadOp, eAttachmentStoreOp storeOp, f32 clearValue_depth, u32 clearValue_stencil)
{
    assert (0 == haveZB);

    haveZB = 1;
    const gpu::DepthStencil *zBuffer_info = gpu->get_info (zbHandle);
    assert (NULL != zBuffer_info);

    memset (&zBufferAttach, 0, sizeof(zBufferAttach));
    zBufferAttach.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    zBufferAttach.imageView = zBuffer_info->view;
    zBufferAttach.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL_KHR;
    zBufferAttach.loadOp = gpu::toVulkan(loadOp);
    zBufferAttach.storeOp = gpu::toVulkan(storeOp);
    zBufferAttach.clearValue.depthStencil.depth = clearValue_depth;
    zBufferAttach.clearValue.depthStencil.stencil = clearValue_stencil;
}



