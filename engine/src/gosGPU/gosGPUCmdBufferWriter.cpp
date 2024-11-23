#include "gosGPUCmdBufferWriter.h"
#include "gosGPU.h"


using namespace gos;

typedef gpu::CmdBufferWriter    GPUCMDWR;   //di comodo


//***********************************************
gpu::CmdBufferWriter::CmdBufferWriter()
{
    flag.setAll();
    vkCommandBuffer = VK_NULL_HANDLE;
}

//***********************************************
GPUCMDWR& gpu::CmdBufferWriter::begin (GPU *gpuIN, const GPUCmdBufferHandle handle)
{
    assert (NULL == vkCommandBuffer);
    gpu = gpuIN;
    flag.zero();
    depthClearColor = 1.0f;
    stencilClearColor = 0;
    curPipeline = NULL;


    if (!gpu->toVulkan (handle, &vkCommandBuffer))
    {
        gos::logger::err ("gpu::CmdBufferWriter::begin => invalid cmdBufferHandle\n");
        priv_setError();
    }    


    VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0; // Optional
        beginInfo.pInheritanceInfo = nullptr; // Optional

    VkResult result = vkBeginCommandBuffer (vkCommandBuffer, &beginInfo);
    if (VK_SUCCESS != result)
    {
        gos::logger::err ("gpu::CmdBufferWriter::begin() => vkBeginCommandBuffer() => %s\n", string_VkResult(result));
        priv_setError();
    }


    return *this;
}

//***********************************************
GPUCMDWR& gpu::CmdBufferWriter::setViewport (const GPUViewportHandle handle)
{
    if (!anyError())
    {
        const gos::gpu::Viewport *viewport = gpu->viewport_get(handle);

        VkViewport vkViewport {0.0f, 0.0f, (viewport->getW_f32()), viewport->getH_f32(), 0.0f, 1.0f };
        vkCmdSetViewport(vkCommandBuffer, 0, 1, &vkViewport);

        VkRect2D scissor { 0, 0, viewport->getW(), viewport->getH() };
        vkCmdSetScissor (vkCommandBuffer, 0, 1, &scissor);
    }

    return *this;
}

//***********************************************
GPUCMDWR& gpu::CmdBufferWriter::bindPipeline (const GPUPipelineHandle pipelineHandle)
{
    if (anyError())
        return *this;

    //recupero vulkan pipeline
    if (gpu->toVulkan (pipelineHandle, &curPipeline))
    {
        vkCmdBindPipeline (vkCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, curPipeline->vkPipelineHandle);
        flag.set (FLAG__PIPELINE_IS_BOUND);
    }
    else
    {
        gos::logger::err ("gpu::CmdBufferWriter::bindPipeline() => invalid pipelineHandle\n");
        priv_setError();
    }

    return *this;
}

//***********************************************
GPUCMDWR& gpu::CmdBufferWriter::pushConstant (u8 whichOne, const void *data, u32 sizeof_data)
{
    while (1)
    {
        if (NULL == curPipeline)
        {
            gos::logger::err ("gpu::CmdBufferWriter::pushConstant(%d) => no current pipeline set\n", whichOne);
            priv_setError();
            break;
        }

        if (whichOne >= GOSGPU__NUM_MAX_PUSH_CONSTANT_PER_PIPELINE)
        {
            gos::logger::err ("gpu::CmdBufferWriter::pushConstant(%d) => invalid index\n", whichOne);
            priv_setError();
            break;
        }

        if (curPipeline->pushContantList[whichOne].size != sizeof_data)
        {
            gos::logger::err ("gpu::CmdBufferWriter::pushConstant(%d) => size does not match\n", whichOne);
            priv_setError();
            break;
        }

        vkCmdPushConstants (vkCommandBuffer, curPipeline->vkPipelineLayoutHandle, 
                            curPipeline->pushContantList[whichOne].stageFlags,
                            curPipeline->pushContantList[whichOne].offset,
                            curPipeline->pushContantList[whichOne].size,
                            data);
        break;
    }

    return *this;
}

//***********************************************
GPUCMDWR& gpu::CmdBufferWriter::pushDescriptor_begin (u8 set)
{
    while (1)
    {
        if (anyError())
            break;

        if (flag.isBitSet (FLAG__PUSH_DESCRIPTOR_BEGIN))
        {
            gos::logger::err ("gpu::CmdBufferWriter::pushDescriptor_begin() => a 'pushDescriptor_begin' is already in progress\n");
            priv_setError();
            break;
        }

        flag.set (FLAG__PUSH_DESCRIPTOR_BEGIN);
        writeDescr.num = 0;
        writeDescr.set = set;

        break;
    }

    return *this;
}

//***********************************************
GPUCMDWR& gpu::CmdBufferWriter::pushDescriptor_UBO (const GPUUniformBufferHandle &handle, u8 binding)
{
    while (1)
    {
        if (anyError())
            break;

        if (!flag.isBitSet (FLAG__PUSH_DESCRIPTOR_BEGIN))
        {
            gos::logger::err ("gpu::CmdBufferWriter::pushDescriptor_UBO() => you need to call 'pushDescriptor_begin'\n");
            priv_setError();
            break;
        }

        if (writeDescr.num >= GOSGPU__NUM_MAX_WRITE_DESCRIPTORS_PER_CMDBUFFER)
        {
            gos::logger::err ("gpu::CmdBufferWriter::pushDescriptor_UBO() => too many write descriptors'\n");
            priv_setError();
            break;
        }

        VkBuffer vkBufferHandle;
        u32 bufferSize;
        if (!gpu->toVulkan (handle, &vkBufferHandle, &bufferSize))
        {
            gos::logger::err ("CmdBufferWriter::pushDescriptor_UBO() => invalid uniform buffer handle\n");
            return *this;
        }
        

        const u8 n = writeDescr.num;

        memset (&writeDescr.bufferInfo[n], 0, sizeof(VkDescriptorBufferInfo));
        writeDescr.bufferInfo[n].buffer = vkBufferHandle;
        writeDescr.bufferInfo[n].offset = 0;
        writeDescr.bufferInfo[n].range = bufferSize;


        memset (&writeDescr.descr[n], 0, sizeof(VkWriteDescriptorSet));
        writeDescr.descr[n].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        //writeDescr.descr[n].dstSet = 0; //ignorato dall'estensione "vkCmdPushDescriptorSetKHR
        writeDescr.descr[n].dstBinding = binding;
        writeDescr.descr[n].descriptorCount = 1;
        writeDescr.descr[n].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writeDescr.descr[n].pBufferInfo = &writeDescr.bufferInfo[n];

        writeDescr.num++;
        break;
    }


    return *this;
}

//***********************************************
GPUCMDWR& gpu::CmdBufferWriter::pushDescriptor_end ()
{
    while (1)
    {
        if (anyError())
            break;

        if (!flag.isBitSet (FLAG__PUSH_DESCRIPTOR_BEGIN))
        {
            gos::logger::err ("gpu::CmdBufferWriter::pushDescriptor_end() => you need to call 'pushDescriptor_begin'\n");
            priv_setError();
            break;
        }

        if (0 != writeDescr.num)
        {
            GPU::vkCmdPushDescriptorSetKHR (vkCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, curPipeline->vkPipelineLayoutHandle, 
                writeDescr.set, 
                writeDescr.num,
                writeDescr.descr);
        }

        flag.clear (FLAG__PUSH_DESCRIPTOR_BEGIN);
        break;
    }


    return *this;
}

//***********************************************
GPUCMDWR& gpu::CmdBufferWriter::bindDescriptorSet (const GPUDescrSetInstanceHandle handle, u8 set, u32 dynamicOffset)
{
    while (1)
    {
        if (anyError())
            break;

        if (!flag.isBitSet (FLAG__PIPELINE_IS_BOUND))
        {
            gos::logger::err ("gpu::CmdBufferWriter::bindDescriptorSet() => you need to have a pipeline bound\n");
            priv_setError();
            break;
        }

        //recupero il descrSetInstance
        VkDescriptorSet vkDescrSetHandle;
        if (!gpu->toVulkan (handle, &vkDescrSetHandle))
        {
            gos::logger::err ("gpu::CmdBufferWriter::bindDescriptorSet() => invalid descrSetInstace handle\n");
            priv_setError();
            break;
        }           

        if (u32MAX == dynamicOffset)
            vkCmdBindDescriptorSets (vkCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, curPipeline->vkPipelineLayoutHandle, set, 1, &vkDescrSetHandle, 0, nullptr);
        else
            vkCmdBindDescriptorSets (vkCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, curPipeline->vkPipelineLayoutHandle, set, 1, &vkDescrSetHandle, 1, &dynamicOffset);
            
        break;
    }

    return *this;
}

//***********************************************
GPUCMDWR& gpu::CmdBufferWriter::bindVtxBuffer (const GPUVtxBufferHandle handle)
{
    while (1)
    {
        if (anyError())
            break;

        VkBuffer vkVtxBuffer;
        if (!gpu->toVulkan (handle, &vkVtxBuffer))
        {
            gos::logger::err ("gpu::CmdBufferWriter::bindVtxBuffer() => invalid vtxBufferHandle\n");
            priv_setError();
            break;
        }            

        //bindo il vtx buffer a partire dal layout=0
        static const u8 VTXBUFFER__FIRST_VTX_STREAM_INDEX = 0;
        static const u8 VTXBUFFER__NUM_STREAM = 1;
        VkBuffer        vtxBufferList[VTXBUFFER__NUM_STREAM] = { vkVtxBuffer };
        VkDeviceSize    vtxBufferOffsetsList[VTXBUFFER__NUM_STREAM] = {0};    
        vkCmdBindVertexBuffers (vkCommandBuffer, VTXBUFFER__FIRST_VTX_STREAM_INDEX, VTXBUFFER__NUM_STREAM, vtxBufferList, vtxBufferOffsetsList);
        break;
    }

    return *this;
}

//***********************************************
GPUCMDWR& gpu::CmdBufferWriter::bindVtxBuffers (const GPUVtxBufferHandle handleStream0, const GPUVtxBufferHandle handleStream1)
{
    while (1)
    {
        if (anyError())
            break;

        VkBuffer vkVtxBuffer0;
        if (!gpu->toVulkan (handleStream0, &vkVtxBuffer0))
        {
            gos::logger::err ("gpu::CmdBufferWriter::bindVtxBuffer() => invalid vtxBufferHandle\n");
            priv_setError();
            break;
        }            

        VkBuffer vkVtxBuffer1;
        if (!gpu->toVulkan (handleStream1, &vkVtxBuffer1))
        {
            gos::logger::err ("gpu::CmdBufferWriter::bindVtxBuffer() => invalid vtxBufferHandle\n");
            priv_setError();
            break;
        }            

        //bindo il vtx buffer a partire dal layout=0
        static const u8 VTXBUFFER__FIRST_VTX_STREAM_INDEX = 0;
        static const u8 VTXBUFFER__NUM_STREAM = 2;
        VkBuffer        vtxBufferList[VTXBUFFER__NUM_STREAM] = { vkVtxBuffer0, vkVtxBuffer1 };
        VkDeviceSize    vtxBufferOffsetsList[VTXBUFFER__NUM_STREAM] = {0};    
        vkCmdBindVertexBuffers (vkCommandBuffer, VTXBUFFER__FIRST_VTX_STREAM_INDEX, VTXBUFFER__NUM_STREAM, vtxBufferList, vtxBufferOffsetsList);
        break;
    }

    return *this;
}

//***********************************************
GPUCMDWR& gpu::CmdBufferWriter::bindIdxBufferU16 (const GPUIdxBufferHandle handle)
{
    while (1)
    {
        if (anyError())
            break;

        VkBuffer vkIdxBuffer;
        if (!gpu->toVulkan (handle, &vkIdxBuffer))
        {
            gos::logger::err ("gpu::CmdBufferWriter::bindIdxBufferU16() => invalid idxBufferHandle\n");
            priv_setError();
            break;
        }            

        //bindo il vtx buffer a partire dal layout=0
        vkCmdBindIndexBuffer (vkCommandBuffer, vkIdxBuffer, 0, VK_INDEX_TYPE_UINT16);
        break;
    }

    return *this;
}

//***********************************************
GPUCMDWR& gpu::CmdBufferWriter::setClearColor (u8 colorAttachmentIndex, const gos::ColorHDR &color)
{
    while (1)
    {
        if (anyError())
            break;

        if (flag.isBitSet (FLAG__RENDER_PASS_BEGIN))
        {
            gos::logger::err ("gpu::CmdBufferWriter::setClearColor() => a render pass is already in progress. You must set clear color before renderPass_begin()\n");
            priv_setError();
            break;
        }

        if (colorAttachmentIndex >= GOSGPU__NUM_MAX_ATTACHMENT)
        {
            gos::logger::err ("gpu::CmdBufferWriter::setClearColor() => invalid colorAttachmentIndex (too big): %d\n", colorAttachmentIndex);
            priv_setError();
            break;
        }

        clearColorList[colorAttachmentIndex].color.float32[0] = color.col.r;
        clearColorList[colorAttachmentIndex].color.float32[1] = color.col.g;
        clearColorList[colorAttachmentIndex].color.float32[2] = color.col.b;
        clearColorList[colorAttachmentIndex].color.float32[3] = color.col.a;
        break;
    }

    return *this;
}

//***********************************************
GPUCMDWR& gpu::CmdBufferWriter::setDepthBufferColor (f32 depth, u32 stencil)
{
    while (1)
    {
        if (anyError())
            break;

        if (flag.isBitSet (FLAG__RENDER_PASS_BEGIN))
        {
            gos::logger::err ("gpu::CmdBufferWriter::setDepthBufferColor() => a render pass is already in progress. You must set clear color before renderPass_begin()\n");
            priv_setError();
            break;
        }

        depthClearColor = depth;
        stencilClearColor = stencil;
        break;
    }

    return *this;
}


//***********************************************
GPUCMDWR& gpu::CmdBufferWriter::renderPass_begin (const GPURenderLayoutHandle renderLayoutHandle, const GPUFrameBufferHandle frameBufferHandle)
{
    while (1)
    {
        if (anyError())
            break;

        if (flag.isBitSet (FLAG__RENDER_PASS_BEGIN))
        {
            gos::logger::err ("gpu::CmdBufferWriter::renderPass_begin() => a render pass is already in progress\n");
            priv_setError();
            break;
        }
        flag.set (FLAG__RENDER_PASS_BEGIN);

        //recupero il vulkan render pass
        const gpu::RenderLayout *renderLayout = gpu->getInfo (renderLayoutHandle);
        if (NULL == renderLayout)
        {
            gos::logger::err ("gpu::CmdBufferWriter::renderPass_begin() => invalid renderLayoutHandle\n");
            priv_setError();
            break;
        }

        //recupero il frame buffer
        VkFramebuffer vkFrameBufferHandle;
        u32 renderAreaW;
        u32 renderAreaH;
        if (!gpu->toVulkan (frameBufferHandle, &vkFrameBufferHandle, &renderAreaW, &renderAreaH))
        {
            gos::logger::err ("gpu::CmdBufferWriter::renderPass_begin() => invalid frameBufferHandle\n");
            priv_setError();
            break;
        }


        //vulkan begin render pass
        if (0xFF != renderLayout->indexOfDepthStencilBuffer)
        {
            assert (renderLayout->indexOfDepthStencilBuffer < GOSGPU__NUM_MAX_ATTACHMENT);
            clearColorList[renderLayout->indexOfDepthStencilBuffer].depthStencil = {depthClearColor, stencilClearColor};
        }

        VkRenderPassBeginInfo renderPassInfo{};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass = renderLayout->vkRenderPassHandle;
            renderPassInfo.framebuffer = vkFrameBufferHandle;
            renderPassInfo.renderArea.offset = {0, 0};
            renderPassInfo.renderArea.extent = { renderAreaW, renderAreaH };
            renderPassInfo.clearValueCount = renderLayout->numAttachment;
            renderPassInfo.pClearValues = clearColorList;

        vkCmdBeginRenderPass (vkCommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        break;
    }

    return *this;
}

//***********************************************
GPUCMDWR& gpu::CmdBufferWriter::drawIndexed (u32 indexCount, u32 instanceCount, u32 firstIndex, u32 vertexOffset, u32 firstInstance)
{
    while (1)
    {
        if (anyError())
            break;

        if (!flag.isBitSet (FLAG__RENDER_PASS_BEGIN))
        {
            gos::logger::err ("gpu::CmdBufferWriter::drawIndexd() => you need to call renderPass_begin() first\n");
            priv_setError();
            break;
        }
        
        vkCmdDrawIndexed(vkCommandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
        break;
    }
    return *this;
}

//***********************************************
GPUCMDWR& gpu::CmdBufferWriter::draw (u32 vtxCount, u32 instanceCount, u32 firstVtx, u32 firstInstance)
{
    while (1)
    {
        if (anyError())
            break;

        if (!flag.isBitSet (FLAG__RENDER_PASS_BEGIN))
        {
            gos::logger::err ("gpu::CmdBufferWriter::draw() => you need to call renderPass_begin() first\n");
            priv_setError();
            break;
        }

        vkCmdDraw (vkCommandBuffer, vtxCount, instanceCount, firstVtx, firstInstance);        
        break;
    }
    return *this;
}

//***********************************************
GPUCMDWR& gpu::CmdBufferWriter::imageTransition (const VkImage &image, const eImageLayout currentLayoutIN, const eImageLayout newLayoutIN)
{
    const VkImageLayout currentLayout = gpu::toVulkan(currentLayoutIN);
    const VkImageLayout newLayout = gpu::toVulkan(newLayoutIN);

    while (1)
    {
        if (anyError())
            break;

        /*if (!flag.isBitSet (FLAG__RENDER_PASS_BEGIN))
        {
            gos::logger::err ("gpu::CmdBufferWriter::imageTransition() => you need to call renderPass_begin() first\n");
            priv_setError();
            break;
        }
        */

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
GPUCMDWR& gpu::CmdBufferWriter::copyImageToImage (const VkImage &source, const VkImage &destination, const VkExtent2D &srcSize, const VkExtent2D &dstSize)
{
    while (1)
    {
        if (anyError())
            break;

        /*if (!flag.isBitSet (FLAG__RENDER_PASS_BEGIN))
        {
            gos::logger::err ("gpu::CmdBufferWriter::imageTransition() => you need to call renderPass_begin() first\n");
            priv_setError();
            break;
        }
        */

        VkImageBlit2 blitRegion{ .sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2, .pNext = nullptr };
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

        VkBlitImageInfo2 blitInfo{ .sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2, .pNext = nullptr };
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
GPUCMDWR& gpu::CmdBufferWriter::renderPass_end()
{
    while (1)
    {
        if (anyError())
            break;

        if (!flag.isBitSet (FLAG__RENDER_PASS_BEGIN))
        {
            gos::logger::err ("gpu::CmdBufferWriter::renderPass_end() => you need to call renderPass_begin() first\n");
            priv_setError();
            break;
        }
        flag.clear (FLAG__RENDER_PASS_BEGIN);

        vkCmdEndRenderPass (vkCommandBuffer);
        break;
    }
    return *this;
}

//***********************************************
bool gpu::CmdBufferWriter::end()
{
    while (1)
    {
        if (anyError())
            break;

        if (flag.isBitSet (FLAG__RENDER_PASS_BEGIN))
        {
            gos::logger::err ("gpu::CmdBufferWriter::end() => a render pass in still in progress, call renderPass_end()\n");
            priv_setError();
            break;
        }

        if (flag.isBitSet (FLAG__PUSH_DESCRIPTOR_BEGIN))
        {
            gos::logger::err ("gpu::CmdBufferWriter::end() => a 'pushDescriptor_begin' in still in progress, call 'pushDescriptor_end'\n");
            priv_setError();
            break;
        }


        const VkResult result = vkEndCommandBuffer (vkCommandBuffer);
        if (VK_SUCCESS != result)
        {
            gos::logger::err ("gpu::CmdBufferWriter::end() => vkEndCommandBuffer() => %s\n", string_VkResult(result));
            priv_setError();
        }    
        break;
    }

    vkCommandBuffer = NULL;
    return !anyError();
}