#include "gosGPU.h"
#include "vulkan/gosGPUVulkan.h"
#include "../gos/gos.h"

using namespace gos;



//*************************************************************************
GPU::ImmediateTransferCmd::ImmediateTransferCmd()
{
    gpu = NULL; 
    handle_cmdBuffer.setInvalid();
}

//*************************************************************************
void GPU::ImmediateTransferCmd::setup (GPU *gpuIN, gos::eGPUQueueFamily queueTypeIN)
{
    gpu = gpuIN;
    queueType = queueTypeIN;
}

//*************************************************************************
void GPU::ImmediateTransferCmd::unsetup()
{
    if (handle_cmdBuffer.isValid())
    {
        gpu->deleteResource (handle_cmdBuffer);
        handle_cmdBuffer.setInvalid();
    }    
    gpu = NULL;
}

//*************************************************************************
VkCommandBuffer GPU::ImmediateTransferCmd::getVulkanCmdBufferHandle() const
{
    const gpu::CommandBuffer *cmdBuffer = gpu->getInfo(handle_cmdBuffer);
    return cmdBuffer->vkHandle;
}

//*************************************************************************
gpu::CmdBufferWriter2* GPU::ImmediateTransferCmd::begin()
{
    //perparo un command buffer per il trasferimento dei dati
    if (handle_cmdBuffer.isInvalid())
    {
        if (!gpu->cmdBuffer_create (queueType, &handle_cmdBuffer))
        {
            gos::logger::err ("GPU::ImmediateTransferCmd::begin() => commandBuffer_create failed\n");
            return NULL;
        }
    }

    cw.begin (gpu, handle_cmdBuffer);
    return &cw;
}

//*************************************************************************
void GPU::ImmediateTransferCmd::end()
{
    cw.end();

    const gpu::CommandBuffer *cmdBuffer = gpu->getInfo(handle_cmdBuffer);

    //submit
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmdBuffer->vkHandle;

    gpu->queue_submit (queueType, 1, &submitInfo, VK_NULL_HANDLE);
    
    //attendo
    gpu->queue_waitIdle (queueType);
}