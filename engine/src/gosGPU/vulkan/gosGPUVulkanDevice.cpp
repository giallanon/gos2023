#include "gosGPUVulkanEnumAndDefine.h"
#include "gosGPUVukanHelpers.h"

using namespace gos;

//*******************************************
void VulkanDevice::priv_reset()
{
    vkDev = VK_NULL_HANDLE;
    phyDevInfo.reset(); 
    swapChainInfo.reset();
    numQFamily = 0;
    memory_maxAllocated = memory_curAllocated = 0;
    memset (map_qfamily_to_q, 0xFF, sizeof(map_qfamily_to_q));
}

//*******************************************
void VulkanDevice::unsetup()
{
    if (VK_NULL_HANDLE == vkDev)
        return;

    for (u8 i=0; i<numQFamily; i++)
        qfamilyList[i].unsetup();

    swapChainInfo.destroy(vkDev);

    vkDestroyDevice(vkDev, nullptr);
    priv_reset();
}

//*******************************************
void VulkanDevice::priv_addNativeQFamily (eGPUQueueFamily familyType, u32 familyIndex)
{
    assert (0xff == priv_from_family_to_index(familyType));

    for (u8 i = 0; i < numQFamily; i++)
    {
        if (qfamilyList[i].getFamilyIndex() == familyIndex)
        {
            map_qfamily_to_q[static_cast<u8>(familyType)] = i;
            return;
        }
    }

    qfamilyList[numQFamily].setup (vkDev, familyType, familyIndex);
    map_qfamily_to_q[static_cast<u8>(familyType)] = numQFamily;
    numQFamily++;
}

//*******************************************
bool VulkanDevice::setup (const sPhyDeviceInfo &phyInfo, const gos::StringList &requiredExtensionList, eVulkanVersion vulkanVersion)
{
    assert (VK_NULL_HANDLE == vkDev);
    this->phyDevInfo = phyInfo;

    bool ret = true;
    gos::Allocator *allocator = gos::getScrapAllocator();

    gos::logger::log ("VulkanDevice::setup()\n");
    gos::logger::incIndent();
    gos::logger::log ("creating with phyDev at index:%d\n   gfxQ familyIndex:%d, count=%d\n   computeQ familyIndex:%d, count=%d\n   transferQ familyIndex:%d, count=%d\n", 
                        phyDevInfo.devIndex,
                        phyDevInfo.queue_gfx.familyIndex, phyDevInfo.queue_gfx.count,
                        phyDevInfo.queue_compute.familyIndex, phyDevInfo.queue_compute.count,
                        phyDevInfo.queue_transfer.familyIndex, phyDevInfo.queue_transfer.count);

    //quali e quante queue mi servono?
    float queuePriority = 1.0f;
    VkDeviceQueueCreateInfo queueCreateInfo[16];
    u8 numOfQueue = 0;
    {
        memset (queueCreateInfo, 0, sizeof(queueCreateInfo));
        queueCreateInfo[numOfQueue].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo[numOfQueue].queueFamilyIndex = phyDevInfo.queue_gfx.familyIndex;
        queueCreateInfo[numOfQueue].queueCount = 1; //phyDevInfo.queue_gfx.count;     TODO indagare se avere + di 1 Q ha senso
        queueCreateInfo[numOfQueue].pQueuePriorities = &queuePriority;
        numOfQueue++;

        if (phyDevInfo.queue_compute.familyIndex != phyDevInfo.queue_gfx.familyIndex)
        {
            queueCreateInfo[numOfQueue].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo[numOfQueue].queueFamilyIndex = phyDevInfo.queue_compute.familyIndex;
            queueCreateInfo[numOfQueue].queueCount = 1;
            queueCreateInfo[numOfQueue].pQueuePriorities = &queuePriority;
            numOfQueue++;
        }

        if (phyDevInfo.queue_transfer.familyIndex != phyDevInfo.queue_gfx.familyIndex &&
            phyDevInfo.queue_transfer.familyIndex != phyDevInfo.queue_compute.familyIndex)
        {
            queueCreateInfo[numOfQueue].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo[numOfQueue].queueFamilyIndex = phyDevInfo.queue_transfer.familyIndex;
            queueCreateInfo[numOfQueue].queueCount = 1;
            queueCreateInfo[numOfQueue].pQueuePriorities = &queuePriority;
            numOfQueue++;
        }
    }

    //Creo l'elenco delle features che voglio attivare
    VkPhyDeviceFeatures devFeatures;
    devFeatures.checkPhysicalDeviceFeatures (phyDevInfo.vkDev, vulkanVersion);



    //creo il device
    const char *foundExtensions[128];
    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = numOfQueue;
    createInfo.pQueueCreateInfos = queueCreateInfo;
    createInfo.enabledExtensionCount = 0;
    createInfo.ppEnabledExtensionNames = foundExtensions;
    createInfo.pNext = &devFeatures.features;


    //aggiungo le etensioni richieste
    VkPhyDeviceExtensionList extList;
    extList.build (allocator, phyDevInfo.vkDev);
    {
        u32 iter;
        const char *identifier;
        requiredExtensionList.toStart(&iter);
        while (NULL != (identifier = requiredExtensionList.next(&iter)))
        {
            const u32 index = extList.find(identifier);
            if (u32MAX == index)
            {
                gos::logger::err ("extension %s not available!\n", identifier);
                ret = false;
            }
            else
            {
                foundExtensions[createInfo.enabledExtensionCount++] = extList(index)->extensionName;
                gos::logger::log ("using extension %s\n", identifier);
            }
        }
    }


    if (ret)
    {

        VkResult result = vkCreateDevice (phyDevInfo.vkDev, &createInfo, nullptr, &this->vkDev);
        if (VK_SUCCESS != result) 
        {
            gos::logger::err ("vkCreateDevice() returned %s\n", string_VkResult(result));
            ret = false;
        }
        else
        {
            //addo le q family
            priv_addNativeQFamily (eGPUQueueFamily::gfx, phyDevInfo.queue_gfx.familyIndex);
            priv_addNativeQFamily (eGPUQueueFamily::compute, phyDevInfo.queue_compute.familyIndex);
            priv_addNativeQFamily (eGPUQueueFamily::transfer, phyDevInfo.queue_transfer.familyIndex);
        }
    }
    
    if (ret)
        gos::logger::log (eTextColor::green, "OK\n");
        
    gos::logger::decIndent();
    return ret;
}

//*********************************************
bool VulkanDevice::getMemoryType (uint32_t typeBits, VkMemoryPropertyFlags properties, u32 *out_index)
{
    assert (NULL != out_index);
    for (u32 i = 0; i < phyDevInfo.vkMemoryProperties.memoryTypeCount; i++)
    {
        if ((typeBits & 1) == 1) //questo vuol dire che la risorsa che voglio allocare puo' essere allocata nel "memory type i-esimo"
        {
            //posto che il memory-type i-esimo sia un memory type valido per questa risorsa, allora voglio che abbia anche
            //tutti le "properties" che ho richiesto
            if ((phyDevInfo.vkMemoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
            {
                *out_index = i;
                return true;
            }
        }
        typeBits >>= 1;
    }

    return false;
}

//*******************************************
bool VulkanDevice::allocMemory (const VkMemoryAllocateInfo *pAllocateInfo, const VkAllocationCallbacks *pAllocator, VkDeviceMemory *pMemory)
{
    const VkResult result = vkAllocateMemory (vkDev, pAllocateInfo, pAllocator, pMemory);
    if (VK_SUCCESS == result)
    {
        memory_curAllocated += pAllocateInfo->allocationSize;
        if (memory_curAllocated > memory_maxAllocated)
            memory_maxAllocated = memory_curAllocated;

#ifdef _DEBUG
        char debug_m1[32];
        char debug_m2[32];
        gos::string::format::memoryToKB_MB_GB(pAllocateInfo->allocationSize, debug_m1, sizeof(debug_m1));
        gos::string::format::memoryToKB_MB_GB(memory_curAllocated, debug_m2, sizeof(debug_m2));
        gos::logger::log (eTextColor::cyan, "VulkanDevice::allocMemory(%s), cur allocated:%s\n", debug_m1, debug_m2);
#endif
        return true;
    }

    gos::logger::err ("VulkanDevice::allocMemory() => %s\n", string_VkResult(result));
    return false;
}

//*********************************************
void VulkanDevice::freeMemory (VkDeviceMemory memory, const VkAllocationCallbacks *pAllocator, u64 memSize)
{
    if (memory_curAllocated >= memSize)
        memory_curAllocated -= memSize;
    else
        memory_curAllocated = 0;
    vkFreeMemory(vkDev, memory, pAllocator);

#ifdef _DEBUG
        char debug_m1[32];
        char debug_m2[32];
        gos::string::format::memoryToKB_MB_GB(memSize, debug_m1, sizeof(debug_m1));
        gos::string::format::memoryToKB_MB_GB(memory_curAllocated, debug_m2, sizeof(debug_m2));
        gos::logger::log (eTextColor::cyan, "VulkanDevice::freeMemory(%s), cur allocated:%s\n", debug_m1, debug_m2);
#endif
}