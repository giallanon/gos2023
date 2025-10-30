#include "gosGPUVulkanEnumAndDefine.h"

using namespace gos;

//*******************************************
VulkanQFamily::VulkanQFamily()
{
    vkDev = VK_NULL_HANDLE;
	vkQueueHandle = VK_NULL_HANDLE;
    familyIndex = 0xff;
    familyType = eGPUQueueFamily::unknown;
    
    for (u32 i = 0; i < NUM_MAX_THREAD; i++)
    {
        poolList[i].vkPoolHandle = VK_NULL_HANDLE;
        poolList[i].threadID = u32MAX;
    }
}

//*******************************************
void VulkanQFamily::setup (VkDevice vkDevIN, eGPUQueueFamily familyTypeIN, u32 familyIndexIN)
{
    assert (VK_NULL_HANDLE==vkDev && eGPUQueueFamily::unknown == familyType);
    vkDev = vkDevIN;
    familyType = familyTypeIN;
    familyIndex = familyIndexIN;

    vkGetDeviceQueue (vkDev, familyIndex, 0, &vkQueueHandle);
}

//*******************************************
void VulkanQFamily::unsetup()
{
    for (u32 i = 0; i < NUM_MAX_THREAD; i++)
    {
        poolList[i].threadID = u32MAX;
        if (VK_NULL_HANDLE != poolList[i].vkPoolHandle)
        {
            vkDestroyCommandPool (vkDev, poolList[i].vkPoolHandle, NULL);
            poolList[i].vkPoolHandle = VK_NULL_HANDLE;
        }
    }

	vkQueueHandle = VK_NULL_HANDLE;
    vkDev = NULL;
}

//*******************************************
VkCommandPool VulkanQFamily::getOrCreateCommandPool (u32 threadID)
{
    for (u32 i = 0; i <NUM_MAX_THREAD; i++)
    {
        if (poolList[i].threadID == threadID)
            return poolList[i].vkPoolHandle;
                
        if (u32MAX == poolList[i].threadID)
        {
            VkCommandPoolCreateInfo poolInfo{};
            poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            poolInfo.queueFamilyIndex = familyIndex;
                
            const VkResult result = vkCreateCommandPool (vkDev, &poolInfo, nullptr, &poolList[i].vkPoolHandle);
            if (VK_SUCCESS == result)
            {
                poolList[i].threadID = threadID;
                return poolList[i].vkPoolHandle;
            }

            DBGBREAK;
            return VK_NULL_HANDLE;
        }
    }
    DBGBREAK;
    return VK_NULL_HANDLE;
}

//*******************************************
void VulkanQFamily::deleteCommandBuffer (VkCommandPool vkPool, VkCommandBuffer vkHandle)
{
    for (u32 i = 0; i < NUM_MAX_THREAD; i++)
    {
        if (poolList[i].vkPoolHandle == vkPool)
        {
            VkCommandBuffer vkCmdBufferList[] = { vkHandle };
            vkFreeCommandBuffers (vkDev, poolList[i].vkPoolHandle, 1, vkCmdBufferList);

            const u32 lastIndex = NUM_MAX_THREAD - 1;
            const u32 nToCopy = lastIndex - i;
            if (nToCopy)
                memcpy (&poolList[i], &poolList[i + 1], sizeof(sPool) * nToCopy);

            poolList[lastIndex].vkPoolHandle = VK_NULL_HANDLE;
            poolList[lastIndex].threadID = u32MAX;
            return;
        }
    }
}

//**********************************************************
void VulkanQFamily::waitIdle ()
{
    vkQueueWaitIdle (vkQueueHandle);
}

//**********************************************************
VkResult VulkanQFamily::submit(u32 submitCount, const VkSubmitInfo *submitInfo, VkFence fence)
{
    return vkQueueSubmit (vkQueueHandle, submitCount, submitInfo, fence);
}


