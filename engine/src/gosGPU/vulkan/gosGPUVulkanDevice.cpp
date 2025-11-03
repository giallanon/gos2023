#include "gosGPUVulkanDevice.h"
#include "gosGPUVukanHelpers.h"
#include "../gosGPUUtils.h"

using namespace gos;

//*******************************************
void VulkanDevice::priv_reset()
{
    vkDev = VK_NULL_HANDLE;
    phyDevInfo.reset(); 
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

    vkDestroyDevice(vkDev, nullptr);
    priv_reset();
}

//*******************************************
void VulkanDevice::priv_addNativeQFamily (eGPUQueueFamily familyType, u32 familyIndex)
{
    assert (0xff == priv_from_family_to_index(familyType));

    for (u8 i = 0; i < numQFamily; i++)
    {
        if (qfamilyList[i].getNativeFamilyIndex() == familyIndex)
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
bool VulkanDevice::findBestDepthOnlyFormat (VkFormat *out_depthFormat) const
{
    // adattato da https://github.com/SaschaWillems/Vulkan/blob/master/base/VulkanTools.cpp
    // Since all depth formats may be optional, we need to find a suitable depth format to use
    // Start with the highest precision packed format
    const VkFormat formatList[] = {
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D16_UNORM
    };

    constexpr u32 N = sizeof(formatList) / sizeof(VkFormat);
    for (u32 i=0; i<N; i++)
    {
        VkFormatProperties formatProps;
        vkGetPhysicalDeviceFormatProperties (phyDevInfo.vkDev, formatList[i], &formatProps);
        if (formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
        {
            *out_depthFormat = formatList[i];
            return true;
        }
    }

    return false;
}

//*********************************************
bool VulkanDevice::findBestDepthStencilFormat (VkFormat* out_depthStencilFormat) const
{
    // adattato da https://github.com/SaschaWillems/Vulkan/blob/master/base/VulkanTools.cpp
    const VkFormat formatList[] = {
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT,
        VK_FORMAT_D16_UNORM_S8_UINT,
    };

    constexpr u32 N = sizeof(formatList) / sizeof(VkFormat);
    for (u32 i=0; i<N; i++)
    {
        VkFormatProperties formatProps;
        vkGetPhysicalDeviceFormatProperties (phyDevInfo.vkDev, formatList[i], &formatProps);
        if (formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
        {
            *out_depthStencilFormat = formatList[i];
            return true;
        }
    }

    return false;
}

//**********************************************************
bool VulkanDevice::semaphore_create  (VkSemaphore *out)
{
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;    

    const VkResult result = vkCreateSemaphore (vkDev, &semaphoreInfo, nullptr, out);
    if (VK_SUCCESS == result)
        return true;
    gos::logger::err ("vulkanCreateSemaphore() => %s\n", string_VkResult(result));
    return false;
}

//************************************
void VulkanDevice::semaphore_destroy  (VkSemaphore &in)
{
    if (VK_NULL_HANDLE != vkDev && VK_NULL_HANDLE != in)
    {
        vkDestroySemaphore (vkDev, in, nullptr);
        in = VK_NULL_HANDLE;
    }
}

//**********************************************************
bool VulkanDevice::fence_create  (bool bStartAsSignaled, VkFence *out)
{
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    if (bStartAsSignaled)
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    const VkResult result = vkCreateFence (vkDev, &fenceInfo, nullptr, out);
    if (VK_SUCCESS == result)
        return true;
    gos::logger::err ("vkCreateFence() => %s\n", string_VkResult(result));
    return false;
}

//************************************
void VulkanDevice::fence_destroy  (VkFence &in)
{
    if (VK_NULL_HANDLE != vkDev && VK_NULL_HANDLE != in)
    {
        vkDestroyFence (vkDev, in, nullptr);
        in = VK_NULL_HANDLE;
    }
}

//************************************
bool VulkanDevice::fence_wait (const VkFence fenceHandle, u64 timeout_ns)
{
    const VkResult result = vkWaitForFences (vkDev, 1, &fenceHandle, VK_TRUE, timeout_ns);
    if (VK_SUCCESS == result)
        return true;
    return false;
}

//************************************
bool VulkanDevice::fence_waitMany (const VkFence *fenceHandleList, bool bWaitForAll, u32 fenceCount, u64 timeout_ns)
{
    VkBool32 vkb = VK_FALSE;
    if (bWaitForAll)
        vkb = VK_TRUE;
    const VkResult result = vkWaitForFences (vkDev, fenceCount, fenceHandleList, vkb, timeout_ns);
    if (VK_SUCCESS == result)
        return true;
    return false;
}

//************************************
bool VulkanDevice::fence_isSignaled  (const VkFence fenceHandle)
{
    const VkResult result = vkGetFenceStatus (vkDev, fenceHandle);
    if (VK_SUCCESS == result)
        return true;
    return false;
}

//************************************
void VulkanDevice::fence_reset (const VkFence fenceHandle)
{
    vkResetFences (vkDev, 1, &fenceHandle);
}

//************************************
void VulkanDevice::fence_resetMany (const VkFence *fenceHandleList, u32 fenceCount)
{
    vkResetFences (vkDev, fenceCount, fenceHandleList);
}

//************************************
bool VulkanDevice::isImage2DFmtSupported (eImageFormat fmtIN, eImageTiling tilingIN) const
{
    VkImageFormatProperties2 out;
    VkPhysicalDeviceImageFormatInfo2 fmt;

    memset (&out, 0, sizeof(out));
    out.sType = VK_STRUCTURE_TYPE_IMAGE_FORMAT_PROPERTIES_2;

    memset (&fmt, 0, sizeof(fmt));
    fmt.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_FORMAT_INFO_2;
    fmt.format = gpu::toVulkan(fmtIN);
    fmt.type = VK_IMAGE_TYPE_2D;
    
    switch (tilingIN)
    {
    default:
        DBGBREAK;
        return false;
    case eImageTiling::optimal: fmt.tiling = VK_IMAGE_TILING_OPTIMAL; break;
    case eImageTiling::linear: fmt.tiling = VK_IMAGE_TILING_LINEAR; break;
    }
    
    fmt.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    VkResult result = vkGetPhysicalDeviceImageFormatProperties2 (phyDevInfo.vkDev, &fmt, &out);
    if (VK_SUCCESS == result)
        return true;
        
    gos::logger::log ("WARN: isImage2DFmtSupported(%s,%s) => not supported\n", gos::utils::enumToString(fmtIN), gos::utils::enumToString(tilingIN));
    return false;
}

//*********************************************
bool VulkanDevice::swapchain_create (const VkSurfaceKHR &vkSurfaceKHR, bool bVSync, sSwapChainInfo *out)
{
    assert (VK_NULL_HANDLE != vkSurfaceKHR);

    out->reset();

    gos::logger::log("VulkanDevice::createSwapChain()\n");
    gos::logger::incIndent();

    gos::Allocator *allocator = gos::getScrapAllocator();

    VkSurfaceCapabilitiesKHR vkSurfCapabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(phyDevInfo.vkDev, vkSurfaceKHR, &vkSurfCapabilities);
    gos::logger::log ("surf capab\n");
    gos::logger::incIndent();
    gos::logger::log ("min/max image count:%d;%d\n", vkSurfCapabilities.minImageCount, vkSurfCapabilities.maxImageCount);
    gos::logger::log ("current width/height: %d;%d\n", vkSurfCapabilities.currentExtent.width, vkSurfCapabilities.currentExtent.height);
    gos::logger::decIndent();

    VPhyDevicekSurfaceFormatKHRList listOfSurfaceFormat;
    listOfSurfaceFormat.build (allocator, phyDevInfo.vkDev, vkSurfaceKHR);
    listOfSurfaceFormat.printInfo();

    VPhyDevicekSurfacePresentModesKHRList listOfPresentMode;
    listOfPresentMode.build (allocator, phyDevInfo.vkDev, vkSurfaceKHR);
    listOfPresentMode.printInfo();


    //voglio creare una swap chain che abbia:
    //  VK_FORMAT_B8G8R8A8_SRGB-
    //  VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
    //  VK_PRESENT_MODE_MAILBOX_KHR oppure VK_PRESENT_MODE_FIFO_RELAXED_KHR oppure VK_PRESENT_MODE_FIFO_KHR (in ordine di priorità)
    //  image count almeno di 2, preferibilmente 3
    //out->imageFormat = VK_FORMAT_B8G8R8A8_SRGB;
    const VkFormat desiredFormatList[] = { VK_FORMAT_R8G8B8A8_SRGB, VK_FORMAT_B8G8R8A8_SRGB };
    for (u32 i=0; i< sizeof(desiredFormatList) / sizeof(VkFormat); i++)
    {
        if (listOfSurfaceFormat.isSupportedFormat(desiredFormatList[i]))
        {
            out->imageFormat = desiredFormatList[i];
            break;
        }
    }
    out->colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    out->imageExtent = vkSurfCapabilities.currentExtent;
    out->imageCount = 3;
    if (out->imageCount > vkSurfCapabilities.maxImageCount)
        out->imageCount = vkSurfCapabilities.maxImageCount;
    if (out->imageCount < vkSurfCapabilities.minImageCount)
        out->imageCount = vkSurfCapabilities.minImageCount;
    
    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;    //vsync, sempre disponibile
    if (!bVSync)
    {
        if (listOfPresentMode.exists(VK_PRESENT_MODE_MAILBOX_KHR))
            presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
        else if (listOfPresentMode.exists(VK_PRESENT_MODE_FIFO_RELAXED_KHR))
            presentMode = VK_PRESENT_MODE_FIFO_RELAXED_KHR;
        else if (listOfPresentMode.exists(VK_PRESENT_MODE_IMMEDIATE_KHR))
            presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
    }
    
    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = vkSurfaceKHR;
    createInfo.minImageCount = out->imageCount;
    createInfo.imageFormat = out->imageFormat;
    createInfo.imageColorSpace = out->colorSpace;
    createInfo.imageExtent = out->imageExtent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.preTransform = vkSurfCapabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    gos::logger::log ("Attempt to create a swapchain with the following:\n");
    gos::logger::incIndent();
        gos::logger::log ("minImageCount = %d\n", createInfo.minImageCount);
        gos::logger::log ("imageFormat = %s\n", string_VkFormat(createInfo.imageFormat));
        gos::logger::log ("imageColorSpace = %s\n", string_VkColorSpaceKHR(createInfo.imageColorSpace));
        gos::logger::log ("imageExtent = %d;%d\n", createInfo.imageExtent.width, createInfo.imageExtent.height);
        gos::logger::log ("imageUsage = %s\n", string_VkImageUsageFlags(createInfo.imageUsage).c_str());
        gos::logger::log ("imageSharingMode = %s\n", string_VkSharingMode(createInfo.imageSharingMode));
        gos::logger::log ("presentMode = %s\n", string_VkPresentModeKHR(createInfo.presentMode));
    gos::logger::decIndent();

    VkResult result = vkCreateSwapchainKHR (vkDev, &createInfo, nullptr, &out->vkSwapChain);
    if (VK_SUCCESS != result)
    {
        out->reset();
        gos::logger::err ("%s\n", string_VkResult(result));
    }
    else
    {
        //recupero gli handle delle image. Ho chiesto di crearne almeno [createInfo.minImageCount] ma il driver potrebbe averne create di +
        vkGetSwapchainImagesKHR (vkDev, out->vkSwapChain, &out->imageCount, NULL);
        if (out->imageCount > SWAPCHAIN_NUM_MAX_IMAGES)
        {
            gos::logger::err ("driver created a swapchain width %d images. GOS support up to %d images\n", out->imageCount, SWAPCHAIN_NUM_MAX_IMAGES);
            out->imageCount = SWAPCHAIN_NUM_MAX_IMAGES;
        }

        vkGetSwapchainImagesKHR (vkDev, out->vkSwapChain, &out->imageCount, out->vkImageList);
        //creo le image view

        for (u8 i=0; i<out->imageCount; i++)
        {
            VkImageViewCreateInfo imgViewCreateInfo{};
            imgViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            imgViewCreateInfo.image = out->vkImageList[i];
            imgViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            imgViewCreateInfo.format = out->imageFormat;
            imgViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            imgViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            imgViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            imgViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            imgViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imgViewCreateInfo.subresourceRange.baseMipLevel = 0;
            imgViewCreateInfo.subresourceRange.levelCount = 1;
            imgViewCreateInfo.subresourceRange.baseArrayLayer = 0;
            imgViewCreateInfo.subresourceRange.layerCount = 1;  

            result = vkCreateImageView (vkDev, &imgViewCreateInfo, nullptr, &out->vkImageListView[i]);
            if (VK_SUCCESS != result)
            {
                gos::logger::err ("error creating image view for image num %d: %s\n", i, string_VkResult(result));
                break;
            }
        }

        if (VK_SUCCESS == result)
            gos::logger::log (eTextColor::green, "OK (image count=%d)\n", out->imageCount);
    }


    gos::logger::decIndent();
    return (VK_SUCCESS == result);
}

//*********************************************
void VulkanDevice::swapchain_delete (sSwapChainInfo &s)
{
    for (u8 i=0;i<s.imageCount;i++)
    {
        if (VK_NULL_HANDLE != s.vkImageListView[i])
            vkDestroyImageView(vkDev, s.vkImageListView[i], nullptr);
    }
    if (VK_NULL_HANDLE != s.vkSwapChain)
        vkDestroySwapchainKHR(vkDev, s.vkSwapChain, nullptr);
    s.reset();
}

//************************************
VkResult VulkanDevice::swapChain_acquireImage (sSwapChainInfo &swapchain, u64 timeout_ns, VkSemaphore semaphore, VkFence fence, u32 *out_imageIndex)
{
    assert (NULL != out_imageIndex);
    return vkAcquireNextImageKHR (vkDev, swapchain.vkSwapChain, timeout_ns, semaphore, fence, out_imageIndex);
}

//************************************
VkResult VulkanDevice::swapChain_present (sSwapChainInfo &swapchain, const VkSemaphore *semaphoreHandleList, u32 semaphoreCount, u32 imageIndex)
{
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = semaphoreCount;
    presentInfo.pWaitSemaphores = semaphoreHandleList; //prima di presentare, aspetta che GPU segnali tutti i semafori di [semaphoreHandleList]
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain.vkSwapChain;
    presentInfo.pImageIndices = &imageIndex;
    
    return vkQueuePresentKHR (getQFamily(eGPUQueueFamily::gfx)->getHandle(), &presentInfo);
}

//*********************************************
bool VulkanDevice::commandBuffer_create (eGPUQueueFamily whichQ, u32 threadID, VkCommandPool *out_pool, VkCommandBuffer *out_handle)
{
    VulkanQFamily *q = getQFamily(whichQ);
    assert (NULL != q);
   
    return q->commandBuffer_create (threadID, out_pool, out_handle);
}

//*********************************************
void VulkanDevice::commandBuffer_delete (eGPUQueueFamily whichQ, VkCommandPool vkPool, VkCommandBuffer vkHandle)
{
    VulkanQFamily *q = getQFamily(whichQ);
    assert (NULL != q);
   
    q->commandBuffer_delete (vkPool, vkHandle);
}

//************************************
VkResult VulkanDevice::shader_create (const void *bufferIN, u32 bufferSize, VkShaderModule *out)
{
    assert (NULL != out);
    
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = bufferSize;
    createInfo.pCode = reinterpret_cast<const uint32_t*>(bufferIN);
    
    return vkCreateShaderModule(vkDev, &createInfo, nullptr, out);
}

//************************************
void  VulkanDevice::shader_delete (VkShaderModule vkHandle)
{
    if (VK_NULL_HANDLE != vkHandle)
        vkDestroyShaderModule (vkDev, vkHandle, nullptr);
}



//*********************************************
bool VulkanDevice::priv_getMemoryType (uint32_t typeBits, VkMemoryPropertyFlags properties, u32 *out_index)
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
bool VulkanDevice::priv_allocMemory (const VkMemoryAllocateInfo *pAllocateInfo, VkDeviceMemory *pMemory)
{
    const VkResult result = vkAllocateMemory (vkDev, pAllocateInfo, nullptr, pMemory);
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
void VulkanDevice::priv_freeMemory (VkDeviceMemory memory, u64 memSize)
{
    if (memory_curAllocated >= memSize)
        memory_curAllocated -= memSize;
    else
        memory_curAllocated = 0;
    vkFreeMemory(vkDev, memory, nullptr);

#ifdef _DEBUG
        char debug_m1[32];
        char debug_m2[32];
        gos::string::format::memoryToKB_MB_GB(memSize, debug_m1, sizeof(debug_m1));
        gos::string::format::memoryToKB_MB_GB(memory_curAllocated, debug_m2, sizeof(debug_m2));
        gos::logger::log (eTextColor::cyan, "VulkanDevice::freeMemory(%s), cur allocated:%s\n", debug_m1, debug_m2);
#endif
}

//*********************************************
VkResult VulkanDevice::memory_map(VkDeviceMemory vkMemHandle, u32 offset, u32 sizeInByte, VkMemoryMapFlags flags, void** out_p) const
{
    if (u32MAX == sizeInByte)
    {
        assert (offset == 0);
        return vkMapMemory (vkDev, vkMemHandle, 0, VK_WHOLE_SIZE, 0, out_p);    
    }

    //size deve essere un multipo di out->deviceProperties.limits.nonCoherentAtomSize
    const u32 minSize = static_cast<u32>(phyDevInfo.deviceProperties.limits.nonCoherentAtomSize);
    const u32 aa = sizeInByte % minSize;
    sizeInByte += minSize - aa;                                

    return vkMapMemory (vkDev, vkMemHandle, offset, sizeInByte, 0, out_p);
}

//*********************************************
void VulkanDevice::memory_unmap (VkDeviceMemory vkMemHandle)
{
    vkUnmapMemory(vkDev, vkMemHandle);
}

//*********************************************
void VulkanDevice::memory_invalidateRanges (u32 numRanges, const VkMappedMemoryRange *rangeList)
{
    vkInvalidateMappedMemoryRanges (vkDev, numRanges, rangeList);
}

//*********************************************
void VulkanDevice::memory_flushRanges (u32 numRanges, const VkMappedMemoryRange *rangeList)
{
    vkFlushMappedMemoryRanges (vkDev, numRanges, rangeList);
}

//*********************************************
bool VulkanDevice::image_create2D (u32 dimx, u32 dimy, u8 nMipMap, VkFormat fmt, eMemAccessMode memAccessMode, VkImageUsageFlags usage, VkImage *out_imagehandle, VkDeviceMemory *out_vkMemHandle, u32 *out_sizeInByte)
{
    assert (NULL != out_imagehandle);
    assert (NULL != out_vkMemHandle);
    assert (NULL != out_sizeInByte);
    assert (nMipMap >= 1);
    *out_imagehandle = VK_NULL_HANDLE;
    *out_vkMemHandle = VK_NULL_HANDLE;
    *out_sizeInByte = 0;

    VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;

    VkMemoryPropertyFlags vkMemProperties;
    switch (memAccessMode)
    {
    default:
        gos::logger::err ("VulkanDevice::image_create2D() => invalid mode %d => '%s' \n", memAccessMode, gpu::enumToString(memAccessMode));
        return false;
        break;

    case eMemAccessMode::onGPU:
        vkMemProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        break;

    case eMemAccessMode::shared_cpuW_autoSync:
        vkMemProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        //prob qui ci vuole tiling = VK_IMAGE_TILING_LINEAR;
        break;

    case eMemAccessMode::shared_cpuW_manualSync:
        vkMemProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        //prob qui ci vuole tiling = VK_IMAGE_TILING_LINEAR;
        break;

    case eMemAccessMode::readback:
        vkMemProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
        tiling = VK_IMAGE_TILING_LINEAR;
        break;
    }         
    
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = fmt;
    imageInfo.extent = { static_cast<uint32_t>(dimx), static_cast<uint32_t>(dimy), 1 };
    imageInfo.mipLevels = nMipMap;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;    
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;    
    imageInfo.flags = 0; // Optional

    //creo immagine
    VkResult result = vkCreateImage(vkDev, &imageInfo, nullptr, out_imagehandle);
    if (VK_SUCCESS != result)
    {
        gos::logger::err ("VulkanDevice::image_create2D() => vkCreateImage failed => %s\n", string_VkResult(result));
        return false;
    }

    //alloco memoria
    VkMemoryRequirements memReqs;
    vkGetImageMemoryRequirements (vkDev, *out_imagehandle, &memReqs);       
    *out_sizeInByte= static_cast<u32>(memReqs.size);

    VkMemoryAllocateInfo memAllloc{};
	memAllloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memAllloc.allocationSize = memReqs.size;
    priv_getMemoryType (memReqs.memoryTypeBits, vkMemProperties, &memAllloc.memoryTypeIndex);

    if (!priv_allocMemory (&memAllloc, out_vkMemHandle))
    {
        gos::logger::err ("VulkanDevice::image_create2D() => error allocating memory\n");
        return false;
    }

    //bindo il buffer alla memoria allocata
    result = vkBindImageMemory (vkDev, *out_imagehandle, *out_vkMemHandle, 0);
    if (VK_SUCCESS != result)
    {
        gos::logger::err ("VulkanDevice::image_create2D() => vkBindBufferMemory() => %s\n", string_VkResult(result));
        return false;
    }

    return true;    
}

//*********************************************
void VulkanDevice::image_delete (VkImage vkHandle, VkDeviceMemory vkMemHandle, u32 memoryAllocated)
{
    if (VK_NULL_HANDLE != vkHandle)
    {
	    vkDestroyImage(vkDev, vkHandle, nullptr);
    }

    if (VK_NULL_HANDLE != vkMemHandle)
    {
	    priv_freeMemory (vkMemHandle, memoryAllocated);
    }    
}

//*********************************************
void VulkanDevice::image_getSubresourceLayout (VkImage image, const VkImageSubresource *subResource, VkSubresourceLayout *subResourceLayout) const
{
    vkGetImageSubresourceLayout(vkDev, image, subResource, subResourceLayout);
}

//*********************************************
VkResult VulkanDevice::imageView_create (const VkImageViewCreateInfo &createInfo, VkImageView *out_view)
{
    return vkCreateImageView(vkDev, &createInfo, nullptr, out_view);
}

//*********************************************
void VulkanDevice::imageView_delete (VkImageView vkHandle)
{
    if (VK_NULL_HANDLE != vkHandle)
    {
	    vkDestroyImageView(vkDev, vkHandle, nullptr);
    }
}

//*********************************************
bool VulkanDevice::buffer_create (u32 sizeInByte, VkBufferUsageFlags usage, VkMemoryPropertyFlags memProperties,
                                bool bCanBeUsedBy_gfxQ, bool bCanBeUsedBy_computeQ, bool bCanBeUsedBy_transferQ,
                                VkBuffer *out_vkBufferHandle, VkDeviceMemory *out_vkMemHandle, u32*out_realMemAllocated)
{
    assert (NULL != out_vkBufferHandle);
    assert (NULL != out_vkMemHandle);
    assert (NULL != out_realMemAllocated);
    *out_vkBufferHandle = VK_NULL_HANDLE;
    *out_vkMemHandle = VK_NULL_HANDLE;

    //Se la risorsa e' usata da una sola queueFamiliIndex, allora e' EXCLUSIVE, altrimenti e' CONCURRENT
    VkSharingMode sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    u32 queueIndexList[3];
    u32 queueCount = 0;
    if (bCanBeUsedBy_gfxQ || bCanBeUsedBy_computeQ || bCanBeUsedBy_transferQ)
    {
        const u32 familyIndex[] = {
            getQFamily(eGPUQueueFamily::gfx)->getNativeFamilyIndex(),
            getQFamily(eGPUQueueFamily::compute)->getNativeFamilyIndex(),
            getQFamily(eGPUQueueFamily::transfer)->getNativeFamilyIndex()
        };

        const u32 MASK[] = {
            ((u32)0x00000001 << familyIndex[0]),
            ((u32)0x00000001 << familyIndex[1]),
            ((u32)0x00000001 << familyIndex[2])
        };

        u32 mask = 0;
        if (bCanBeUsedBy_gfxQ)          mask |= MASK[0];
        if (bCanBeUsedBy_computeQ)      mask |= MASK[1];
        if (bCanBeUsedBy_transferQ)     mask |= MASK[2];

        for (u8 i=0; i<3; i++)
        {
            if ((mask & MASK[i]) != 0)
            {
                queueIndexList[queueCount++] = familyIndex[i]; 
                mask &= (~MASK[i]); 
            }
        }

        if (queueCount==1)
            sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        else
            sharingMode = VK_SHARING_MODE_CONCURRENT;
    }
    

    VkBufferCreateInfo createInfo {};
    createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    createInfo.sharingMode = sharingMode;
    createInfo.size = sizeInByte;
    createInfo.usage = usage;
    createInfo.queueFamilyIndexCount = queueCount;
    createInfo.pQueueFamilyIndices = queueIndexList;

    //creo il buffer
    VkResult result = vkCreateBuffer (vkDev, &createInfo, nullptr, out_vkBufferHandle);
    if (VK_SUCCESS != result)
    {
        gos::logger::err (" VulkanDevice::buffer_create(size=%d) => vkCreateBuffer() => %s\n", sizeInByte, string_VkResult(result));
        return false;
    }

    //alloco memoria
    VkMemoryRequirements memReqs;
    vkGetBufferMemoryRequirements (vkDev, *out_vkBufferHandle, &memReqs);
    *out_realMemAllocated = static_cast<u32>(memReqs.size);

    VkMemoryAllocateInfo memAllloc{};
	memAllloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memAllloc.allocationSize = memReqs.size;
    priv_getMemoryType (memReqs.memoryTypeBits, memProperties, &memAllloc.memoryTypeIndex);

    if (!priv_allocMemory (&memAllloc, out_vkMemHandle))
    {
        gos::logger::err ("VulkanDevice::buffer_create() => error allocating memory\n");
        return false;
    }

    //bindo il buffer alla memoria allocata
    result = vkBindBufferMemory (vkDev, *out_vkBufferHandle, *out_vkMemHandle, 0);
    if (VK_SUCCESS != result)
    {
        gos::logger::err ("VulkanDevice::buffer_create() => vkBindBufferMemory() => %s\n", string_VkResult(result));
        return false;
    }

    return true;    
}

//*********************************************
void VulkanDevice::buffer_delete (VkBuffer vkBufferHandle, VkDeviceMemory vkMemHandle, u32 realMemAllocated)
{
    vkDestroyBuffer (vkDev, vkBufferHandle, nullptr);
    priv_freeMemory (vkMemHandle, realMemAllocated);
}

//*********************************************
VkResult VulkanDevice::sampler_create (const VkSamplerCreateInfo &creat, VkSampler *out)
{
    assert (NULL != out);
    return vkCreateSampler(vkDev, &creat, nullptr, out);
}

//*********************************************
void VulkanDevice::sampler_delete (VkSampler vkHandle)
{
    if (VK_NULL_HANDLE != vkHandle)
        vkDestroySampler (vkDev, vkHandle, nullptr);
}

//*********************************************
VkResult VulkanDevice::descPool_create (const VkDescriptorPoolCreateInfo &creatInfo, VkDescriptorPool *out)
{
    return vkCreateDescriptorPool (vkDev, &creatInfo, nullptr, out);
}
//*********************************************
void VulkanDevice::descPool_delete (VkDescriptorPool vkHandle)
{
    vkDestroyDescriptorPool (vkDev, vkHandle, nullptr);
}

//*********************************************
VkResult VulkanDevice::descSetLayout_create (const VkDescriptorSetLayoutCreateInfo &creatInfo, VkDescriptorSetLayout *out)
{
    return vkCreateDescriptorSetLayout (vkDev, &creatInfo, nullptr, out);
}

//*********************************************
void VulkanDevice::descSetLayout_delete (VkDescriptorSetLayout vkHandle)
{
    vkDestroyDescriptorSetLayout (vkDev, vkHandle, nullptr);
}

//*********************************************
VkResult VulkanDevice::descriptorSet_create (VkDescriptorPool vkPoolHandle, VkDescriptorSetLayout vkDescSetLayoutHandle, VkDescriptorSet *out)
{
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = vkPoolHandle;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &vkDescSetLayoutHandle;

    return vkAllocateDescriptorSets (vkDev, &allocInfo, out);
}

//*********************************************
void VulkanDevice::descriptorSet_delete (VkDescriptorPool vkPoolHandle, VkDescriptorSet vkHandle)
{
    vkFreeDescriptorSets (vkDev, vkPoolHandle, 1, &vkHandle);
}

//*********************************************
void VulkanDevice::descriptorSet_update (u32 descriptorWriteCount, const VkWriteDescriptorSet *pDescriptorWrites, u32 descriptorCopyCount, const VkCopyDescriptorSet *pDescriptorCopies) const
{
    vkUpdateDescriptorSets (vkDev, descriptorWriteCount, pDescriptorWrites, descriptorCopyCount, pDescriptorCopies);
}

//*********************************************
VkResult VulkanDevice::pipelineLayout_create (const VkPipelineLayoutCreateInfo &creat, VkPipelineLayout *out)
{
    return vkCreatePipelineLayout (vkDev, &creat, nullptr, out);
}

//*********************************************
void VulkanDevice::pipelineLayout_delete (VkPipelineLayout vkPipelineLayoutHandle)
{
    if (VK_NULL_HANDLE != vkPipelineLayoutHandle)
        vkDestroyPipelineLayout (vkDev, vkPipelineLayoutHandle, nullptr);
}

//*********************************************
VkResult VulkanDevice::pipeline_create (const VkGraphicsPipelineCreateInfo &creat, VkPipeline *out)
{
    return vkCreateGraphicsPipelines (vkDev, VK_NULL_HANDLE, 1, &creat, nullptr, out);
}

//*********************************************
void VulkanDevice::pipeline_delete (VkPipeline vkPipelineHandle)
{
    if (VK_NULL_HANDLE != vkPipelineHandle)
        vkDestroyPipeline (vkDev, vkPipelineHandle, nullptr);
}