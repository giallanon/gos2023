#include "gosGPUPipe2_cmdBufferWriter.h"
#include "../gosGPU.h"

using namespace gos;
using namespace gos::gpu;

typedef CmdBufferWriter2::SetupRenderCtx CMDBUFW_SETUPRNDCTX_CLASS;

//*****************************************
void CmdBufferWriter2::SetupRenderCtx::reset (GPU *gpuIN, CmdBufferWriter2 *cmdBufferWriterIN, VkCommandBuffer vkCommandBufferIN, RenderCtx *out_renderCtx)
{
    gpu = gpuIN;
    cmdBufferWriter = cmdBufferWriterIN;
    vkCommandBuffer = vkCommandBufferIN;
    this->out_renderCtx = out_renderCtx;

    props.reset();
}


//*****************************************
RenderCtx& CmdBufferWriter2::SetupRenderCtx::define_end()
{
    out_renderCtx->internal__reset (gpu, vkCommandBuffer, props);
    return *out_renderCtx;
}

//*****************************************
CMDBUFW_SETUPRNDCTX_CLASS& CmdBufferWriter2::SetupRenderCtx::withRenderArea (u32 w, u32 h)
{
    props.withRenderArea (w,h);
    return *this;
}

//*****************************************
CMDBUFW_SETUPRNDCTX_CLASS& CmdBufferWriter2::SetupRenderCtx::withRenderArea (const GPURenderTargetHandle &rtHandle)
{
    props.withRenderArea (gpu, rtHandle);
    return *this;
}

//*****************************************
CMDBUFW_SETUPRNDCTX_CLASS& CmdBufferWriter2::SetupRenderCtx::withRT (const VkImageView &source, eAttachmentLoadOp loadOp, eAttachmentStoreOp storeOp, const gos::ColorHDR &color)
{
    props.withRT (source, loadOp, storeOp, color);
    return *this;
}

//*****************************************
CMDBUFW_SETUPRNDCTX_CLASS& CmdBufferWriter2::SetupRenderCtx::withRT (const GPURenderTargetHandle &rtHandle, eAttachmentLoadOp loadOp, eAttachmentStoreOp storeOp, const gos::ColorHDR &color)
{
    props.withRT (gpu, rtHandle, loadOp, storeOp, color);
    return *this;
}

//*****************************************
CMDBUFW_SETUPRNDCTX_CLASS& CmdBufferWriter2::SetupRenderCtx::withZB (const GPUZBufferHandle &zbHandle, eAttachmentLoadOp loadOp, eAttachmentStoreOp storeOp, f32 clearValue_depth, u32 clearValue_stencil)
{
    props.withZB (gpu, zbHandle, loadOp, storeOp, clearValue_depth, clearValue_stencil);
    return *this;
}



