#include "gosGPUVulkanQFamily.h"

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

    thread::mutexCreate (&mutex);
}

//*******************************************
void VulkanQFamily::unsetup()
{
    if (VK_NULL_HANDLE == vkDev)
        return;

    thread::mutexDestroy (mutex);
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
void VulkanQFamily::deleteCommandPool (VkCommandPool vkPool)
{
    for (u32 i = 0; i < NUM_MAX_THREAD; i++)
    {
        if (poolList[i].vkPoolHandle == vkPool)
        {
            vkDestroyCommandPool (vkDev, vkPool, NULL);

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


//*******************************************
bool VulkanQFamily::commandBuffer_create (u32 threadID, VkCommandPool *out_pool, VkCommandBuffer *out_handle)
{
    *out_pool = getOrCreateCommandPool(threadID);
    if (NULL == *out_pool)
    {
        gos::logger::log ("VulkanQFamily::commandBuffer_create() => can't create a command pool\n");
        return false;
    }

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = *out_pool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    const VkResult result = vkAllocateCommandBuffers (vkDev, &allocInfo, out_handle);
    if (result == VK_SUCCESS)
        return true;

    gos::logger::log ("VulkanQFamily::commandBuffer_create() => vkAllocateCommandBuffers() => %s\n", string_VkResult(result));
    return false;
}

//*******************************************
void VulkanQFamily::commandBuffer_delete (VkCommandPool vkPool, VkCommandBuffer vkHandle)
{
    vkFreeCommandBuffers (vkDev, vkPool, 1, &vkHandle);
}

//**********************************************************
void VulkanQFamily::waitIdle ()
{
    vkQueueWaitIdle (vkQueueHandle);
}

//**********************************************************
VkResult VulkanQFamily::submit(u32 submitCount, const VkSubmitInfo *submitInfo, VkFence fence)
{
    thread::mutexLock (mutex);
	const VkResult ret = vkQueueSubmit (vkQueueHandle, submitCount, submitInfo, fence);
    thread::mutexUnlock (mutex);
    return ret;
}


