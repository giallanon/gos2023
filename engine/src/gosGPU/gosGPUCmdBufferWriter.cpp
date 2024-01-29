#include "gosGPUCmdBufferWriter.h"
#include "gosGPU.h"


using namespace gos;

typedef gpu::CmdBufferWriter    GPUCMDWR;   //di comodo


//***********************************************
gpu::CmdBufferWriter::CmdBufferWriter()
{
    flag = u32MAX;
    vkCommandBuffer = VK_NULL_HANDLE;
}

//***********************************************
GPUCMDWR& gpu::CmdBufferWriter::begin (GPU *gpuIN, const GPUCmdBufferHandle handle)
{
    assert (NULL == vkCommandBuffer);
    gpu = gpuIN;
    flag = 0;
    vkPipelineHandle = VK_NULL_HANDLE;
    vkPipelineLayoutHandle = VK_NULL_HANDLE;
    depthClearColor = 1.0f;
    stencilClearColor = 0;


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
    if (gpu->toVulkan (pipelineHandle, &vkPipelineHandle, &vkPipelineLayoutHandle))
    {
        vkCmdBindPipeline (vkCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkPipelineHandle);
        gos::utils::bitSET (&flag, FLAG__PIPELINE_IS_BOUND);
    }
    else
    {
        gos::logger::err ("gpu::CmdBufferWriter::renderPass_begin() => invalid pipelineHandle\n");
        priv_setError();
    }

    return *this;
}

//***********************************************
GPUCMDWR& gpu::CmdBufferWriter::bindDescriptorSet (const GPUDescrSetInstancerHandle handle)
{
    while (1)
    {
        if (anyError())
            break;

        if (!gos::utils::isBitSET (&flag, FLAG__PIPELINE_IS_BOUND))
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

        vkCmdBindDescriptorSets (vkCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkPipelineLayoutHandle, 0, 1, &vkDescrSetHandle, 0, nullptr);
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

        if (gos::utils::isBitSET(&flag, FLAG__RENDER_PASS_BEGIN))
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

        if (gos::utils::isBitSET(&flag, FLAG__RENDER_PASS_BEGIN))
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

        if (gos::utils::isBitSET(&flag, FLAG__RENDER_PASS_BEGIN))
        {
            gos::logger::err ("gpu::CmdBufferWriter::renderPass_begin() => a render pass is already in progress\n");
            priv_setError();
            break;
        }
        gos::utils::bitSET (&flag, FLAG__RENDER_PASS_BEGIN);

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
            clearColorList[renderLayout->indexOfDepthStencilBuffer].depthStencil = {depthClearColor, stencilClearColor};

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

        if (!gos::utils::isBitSET(&flag, FLAG__RENDER_PASS_BEGIN))
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
GPUCMDWR& gpu::CmdBufferWriter::renderPass_end()
{
    while (1)
    {
        if (anyError())
            break;

        if (!gos::utils::isBitSET(&flag, FLAG__RENDER_PASS_BEGIN))
        {
            gos::logger::err ("gpu::CmdBufferWriter::renderPass_end() => you need to call renderPass_begin() first\n");
            priv_setError();
            break;
        }
        gos::utils::bitCLEAR (&flag, FLAG__RENDER_PASS_BEGIN);

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

        if (gos::utils::isBitSET(&flag, FLAG__RENDER_PASS_BEGIN))
        {
            gos::logger::err ("gpu::CmdBufferWriter::end() => a render pass in still in progress, call renderPass_end()\n");
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

    return !anyError();
}