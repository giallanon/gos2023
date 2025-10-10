#include "gosGPUPipe2_cmdBufferWriter.h"
#include "../gosGPU.h"


using namespace gos;
using namespace gos::gpu;
using namespace gos::gpu::pipe2;



//***********************************************
CmdBufferWriter2::CmdBufferWriter2()
{
    flag.setAll();
    vkCommandBuffer = VK_NULL_HANDLE;
}

//***********************************************
CmdBufferWriter2& CmdBufferWriter2::begin (GPU *gpuIN, const GPUCmdBufferHandle handle)
{
    assert (NULL == vkCommandBuffer);
    gpu = gpuIN;
    flag.zero();


    if (!gpu->toVulkan (handle, &vkCommandBuffer))
    {
        gos::logger::err ("gpu::pipe2::CmdBufferWriter2::begin => invalid cmdBufferHandle\n");
        priv_setError();
    }    


    VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0; // Optional
        beginInfo.pInheritanceInfo = nullptr; // Optional

    VkResult result = vkBeginCommandBuffer (vkCommandBuffer, &beginInfo);
    if (VK_SUCCESS != result)
    {
        gos::logger::err ("gpu::pipe2::CmdBufferWriter2::begin() => vkBeginCommandBuffer() => %s\n", string_VkResult(result));
        priv_setError();
    }


    return *this;
}

//***********************************************
CmdBufferWriter2& CmdBufferWriter2::setViewport (const GPUViewportHandle handle)
{
    if (!anyError())
    {
        const gos::gpu::Viewport *viewport = gpu->getInfo(handle);

        VkViewport vkViewport {0.0f, 0.0f, (viewport->getW_f32()), viewport->getH_f32(), 0.0f, 1.0f };
        vkCmdSetViewport(vkCommandBuffer, 0, 1, &vkViewport);

        VkRect2D scissor { 0, 0, viewport->getW(), viewport->getH() };
        vkCmdSetScissor (vkCommandBuffer, 0, 1, &scissor);
    }

    return *this;
}

//***********************************************
bool CmdBufferWriter2::end()
{
    while (1)
    {
        if (anyError())
            break;

        const VkResult result = vkEndCommandBuffer (vkCommandBuffer);
        if (VK_SUCCESS != result)
        {
            gos::logger::err ("gpu::pipe2::CmdBufferWriter2::end() => vkEndCommandBuffer() => %s\n", string_VkResult(result));
            priv_setError();
        }    
        break;
    }

    vkCommandBuffer = NULL;
    return !anyError();
}

//***********************************************
CmdBufferWriter2& CmdBufferWriter2::imageTransition (const GPURenderTargetHandle &rtHandle, const eImageLayout currentLayout, const eImageLayout newLayout)
{
    const gpu::RenderTarget *rt_info = gpu->getInfo (rtHandle);
    assert (NULL != rt_info);

    return imageTransition (rt_info->image, currentLayout, newLayout);
}

//***********************************************
CmdBufferWriter2& CmdBufferWriter2::imageTransition (const GPUZBufferHandle &zbHandle, const eImageLayout currentLayout, const eImageLayout newLayout)
{
    const gpu::DepthStencil *zBuffer_info = gpu->getInfo (zbHandle);
    assert (NULL != zBuffer_info);
    return imageTransition (zBuffer_info->image, currentLayout, newLayout);
}

//***********************************************
CmdBufferWriter2& CmdBufferWriter2::imageTransition (const VkImage &image, const eImageLayout currentLayoutIN, const eImageLayout newLayoutIN)
{
    const VkImageLayout currentLayout = gpu::toVulkan(currentLayoutIN);
    const VkImageLayout newLayout = gpu::toVulkan(newLayoutIN);

    while (1)
    {
        if (anyError())
            break;

        VkImageMemoryBarrier2 imageBarrier {};
        imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        imageBarrier.pNext = nullptr;

        imageBarrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
        imageBarrier.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
        imageBarrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
        imageBarrier.dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;

        imageBarrier.image = image;
        imageBarrier.oldLayout = currentLayout;
        imageBarrier.newLayout = newLayout;

        const VkImageAspectFlags ASPECT_MASK = (newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
        imageBarrier.subresourceRange = {};
        imageBarrier.subresourceRange.aspectMask = ASPECT_MASK;
        imageBarrier.subresourceRange.baseMipLevel = 0;
        imageBarrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
        imageBarrier.subresourceRange.baseArrayLayer = 0;
        imageBarrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;


        VkDependencyInfo depInfo {};
        depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        depInfo.pNext = nullptr;
        depInfo.imageMemoryBarrierCount = 1;
        depInfo.pImageMemoryBarriers = &imageBarrier;

        vkCmdPipelineBarrier2(vkCommandBuffer, &depInfo);
        break;
    }
    return *this;
}

//***********************************************
CmdBufferWriter2& CmdBufferWriter2::copyImageToImage (const VkImage &source, const VkImage &destination, const VkExtent2D &srcSize, const VkExtent2D &dstSize)
{
    while (1)
    {
        if (anyError())
            break;

        VkImageBlit2 blitRegion{ };
        blitRegion.sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2;
        blitRegion.pNext = nullptr;
        blitRegion.srcOffsets[1].x = srcSize.width;
        blitRegion.srcOffsets[1].y = srcSize.height;
        blitRegion.srcOffsets[1].z = 1;

        blitRegion.dstOffsets[1].x = dstSize.width;
        blitRegion.dstOffsets[1].y = dstSize.height;
        blitRegion.dstOffsets[1].z = 1;

        blitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blitRegion.srcSubresource.baseArrayLayer = 0;
        blitRegion.srcSubresource.layerCount = 1;
        blitRegion.srcSubresource.mipLevel = 0;

        blitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blitRegion.dstSubresource.baseArrayLayer = 0;
        blitRegion.dstSubresource.layerCount = 1;
        blitRegion.dstSubresource.mipLevel = 0;

        VkBlitImageInfo2 blitInfo{};
        blitInfo.sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2;
        blitInfo.pNext = nullptr;
  
        blitInfo.dstImage = destination;
        blitInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        blitInfo.srcImage = source;
        blitInfo.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        blitInfo.filter = VK_FILTER_LINEAR;
        blitInfo.regionCount = 1;
        blitInfo.pRegions = &blitRegion;

        vkCmdBlitImage2 (vkCommandBuffer, &blitInfo);
        break;
    }
    return *this;
}

//***********************************************
CmdBufferWriter2& CmdBufferWriter2::copyBuffer (const VkBuffer srcBuffer, const VkBuffer dstBuffer, u32 offsetSRC, u32 offsetDST, u32 howManyByteToCopy)
{
    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = offsetSRC;
    copyRegion.dstOffset = offsetDST;
    copyRegion.size = howManyByteToCopy;
    vkCmdCopyBuffer (vkCommandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
    return *this;
}

//***********************************************
CmdBufferWriter2& CmdBufferWriter2::copyBuffer (GPUStgBufferHandle srcStageBufferHandle, GPUVtxBufferHandle dstVtxBufferHandle, u32 offsetSRC, u32 offsetDST, u32 howManyByteToCopy)
{
    const gpu::Buffer *src = gpu->getInfo (srcStageBufferHandle);
    const gpu::Buffer *dst = gpu->getInfo (dstVtxBufferHandle);
    return copyBuffer (src->vkHandle, dst->vkHandle, offsetSRC, offsetDST, howManyByteToCopy);
}

//***********************************************
CmdBufferWriter2& CmdBufferWriter2::copyBuffer (GPUStgBufferHandle srcStageBufferHandle, GPUIdxBufferHandle dstIdxBufferHandle, u32 offsetSRC, u32 offsetDST, u32 howManyByteToCopy)
{
    const gpu::Buffer *src = gpu->getInfo (srcStageBufferHandle);
    const gpu::Buffer *dst = gpu->getInfo (dstIdxBufferHandle);
    return copyBuffer (src->vkHandle, dst->vkHandle, offsetSRC, offsetDST, howManyByteToCopy);
}

//***********************************************
CmdBufferWriter2& CmdBufferWriter2::copyImageToImage (const GPURenderTargetHandle &rtHandle, const VkImage &destination, const VkExtent2D &srcSize, const VkExtent2D &dstSize)
{
    const gpu::RenderTarget *rt_info = gpu->getInfo (rtHandle);
    assert (NULL != rt_info);

    return copyImageToImage (rt_info->image, destination, srcSize, dstSize);
}

//***********************************************
CmdBufferWriter2& CmdBufferWriter2::copyImageToImage (const GPURenderTargetHandle &rtSRC, const GPURenderTargetHandle &rtDST, const VkExtent2D &srcSize, const VkExtent2D &dstSize)
{
    const gpu::RenderTarget *rtSRC_info = gpu->getInfo (rtSRC);
    assert (NULL != rtSRC_info);

    const gpu::RenderTarget *rtDST_info = gpu->getInfo (rtDST);
    assert (NULL != rtDST_info);

    return copyImageToImage (rtSRC_info->image, rtDST_info->image, srcSize, dstSize);
}

//***********************************************
CmdBufferWriter2::BeginRend& CmdBufferWriter2::beginRender()
{
    beginRend.priv_setup (gpu, this, vkCommandBuffer);
    return beginRend;
}

