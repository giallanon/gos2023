#include "gosGPU.h"
#include "vulkan/gosGPUVulkan.h"
#include "../gos/gos.h"

using namespace gos;



//*************************************************************************
GPU::ImmediateTransferCmd::ImmediateTransferCmd()
{
    vkDevice = NULL; 
    vkCmdBuffer = VK_NULL_HANDLE;
}

//*************************************************************************
void GPU::ImmediateTransferCmd::setup (sVkDevice *vkDeviceIN, gos::eGPUQueueType queueTypeIN)
{
    vkDevice = vkDeviceIN;
    queueType = queueTypeIN;
}

//*************************************************************************
void GPU::ImmediateTransferCmd::unsetup()
{
    if (VK_NULL_HANDLE != vkCmdBuffer)
    {
        vulkanDeleteCommandBuffer (*vkDevice, queueType, vkCmdBuffer);
        vkCmdBuffer = VK_NULL_HANDLE;
    }    

    vkDevice = NULL;
}

//*************************************************************************
void GPU::ImmediateTransferCmd::begin()
{
    //perparo un command buffer per il trasferimento dei dati
    if (VK_NULL_HANDLE == vkCmdBuffer)
    {
        if (!vulkanCreateCommandBuffer (*vkDevice, queueType, &vkCmdBuffer))
        {
            gos::logger::err ("GPU::ImmediateTransferCmd::begin() => createCommandBuffer failed\n");
            return;
        }
    }

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer (vkCmdBuffer, &beginInfo);      
}

//*************************************************************************
void GPU::ImmediateTransferCmd::copyBuffer (const VkBuffer srcBuffer, const VkBuffer dstBuffer, u32 offsetSRC, u32 offsetDST, u32 howManyByteToCopy)
{
    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = offsetSRC;
    copyRegion.dstOffset = offsetDST;
    copyRegion.size = howManyByteToCopy;
    vkCmdCopyBuffer(vkCmdBuffer, srcBuffer, dstBuffer, 1, &copyRegion);       
}

//*************************************************************************
void GPU::ImmediateTransferCmd::transitionImageLayout (VkImage image, u8 numMipMap, VkImageLayout oldLayout, VkImageLayout newLayout)
{
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = numMipMap;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;


    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;
    if (VK_IMAGE_LAYOUT_UNDEFINED == oldLayout && VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL == newLayout)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } 
    else if (VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL == oldLayout && VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL == newLayout)
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_NONE;//VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT; //VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else 
    {
        gos::logger::err ("gpu::ImmediateTransferCmd::transitionImageLayout() => unsupported layout transition!");
        return;
    }

    vkCmdPipelineBarrier(
        vkCmdBuffer,
        sourceStage, 
        destinationStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier);    
}

//*************************************************************************
void GPU::ImmediateTransferCmd::end()
{
    vkEndCommandBuffer(vkCmdBuffer);

    //submit
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &vkCmdBuffer;

    VkQueue vkQ = vkDevice->getQueueInfo(queueType)->vkQueueHandle;
    vkQueueSubmit (vkQ, 1, &submitInfo, VK_NULL_HANDLE);
    
    //attendo
    vkQueueWaitIdle (vkQ);
}