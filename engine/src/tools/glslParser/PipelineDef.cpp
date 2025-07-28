#include "PipelineParser.h"

using namespace gos;


//***********************************
void PipelineDef::setDefault()
{
    memset (this, 0, sizeof(PipelineDef));

    outputRT_fmt = eImageFormat::_SAME_AS_CURRENT_SWAPCHAIN;
    outputRT_finalLayout = eImageLayout::color_attachment_optimal;
    outputRT_loadOp = eAttachmentLoadOp::clear;
    outputRT_storeOp = eAttachmentStoreOp::store;
    outputRT_clearCol_ARGB = 0xFF000000;
    
    outputDepth_fmt = eImageFormat::_SAME_AS_CURRENT_SWAPCHAIN;
    outputDepth_finalLayout = eDepthStencilLayout::depth_attachment_optimal;
    outputDepth_loadOp = eAttachmentLoadOp::clear;
    outputDepth_storeOp = eAttachmentStoreOp::store;
    outputDepth_zClearValue = 1;
    outputDepth_stencilClearValue = 0;

    zbuffer_enabled = true;
    zbuffer_write = true;
    zbuffer_cmpFn = eZFunc::LESS;

    stencil_enabled = false;
    stencil_cmpFn = eStencilFunc::NEVER;

    cullMode = eCullMode::CCW;
    drawPrimitive = eDrawPrimitive::trisList;
}