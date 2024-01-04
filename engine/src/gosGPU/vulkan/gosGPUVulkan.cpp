#include "gosGPUVulkan.h"
#include "gosGPUVukanHelpers.h"
#include "../../../gos/gos.h"

using namespace gos;

//*********************************************
bool gos::vulkanCreateInstance (VkInstance *out, const gos::StringList &requiredValidationLayerList, const gos::StringList &requiredExtensionList)
{
    gos::logger::log ("vulkanCreateInstance()\n");
    gos::logger::incIndent();

    bool ret = true;
    gos::Allocator *allocator = gos::getScrapAllocator();
    UTF8Char virgola(',');
    gos::string::utf8::StringListParser parser;

    //app info, informazioni generali da passare a vulkan
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = gos::getAppName();
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "GOSEngine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;


    VkInstanceCreateInfo createInfo{};
    const char *foundValidationLayers[128];
    const char *foundExtensions[128];
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledLayerCount = 0;
    createInfo.ppEnabledLayerNames = foundValidationLayers;
    createInfo.enabledExtensionCount = 0;
    createInfo.ppEnabledExtensionNames = foundExtensions;


    //elenco dei validation layer disponibili per la instance
    VkInstanceValidationLayersList vkAvailValidationLayerList;
    vkAvailValidationLayerList.build(allocator);
    vkAvailValidationLayerList.printInfo();
    {
        //verifico che tutti i layer richiesti siano disponibili
        u32 iter;
        const char *identifier;
        requiredValidationLayerList.toStart(&iter);
        while (NULL != (identifier = requiredValidationLayerList.nextAsChar(&iter)))
        {
            const u32 index = vkAvailValidationLayerList.find(identifier);
            if (u32MAX == index)
            {
                gos::logger::err ("layer %s not available!\n", identifier);
                ret = false;
            }
            else
            {
                foundValidationLayers[createInfo.enabledLayerCount++] = vkAvailValidationLayerList(index)->layerName;
            }
        }
    }

    VkInstanceExtensionList vkAvailExtensionList;
    if (ret)
    {
        //elenco delle estensioni disponibili per la instance
        vkAvailExtensionList.build(allocator);
        vkAvailExtensionList.printInfo();

        //verifico che tutte le estensioni richieste siano disponibili
        u32 iter;
        const char *identifier;
        requiredExtensionList.toStart(&iter);
        while (NULL != (identifier = requiredExtensionList.nextAsChar(&iter)))
        {
            const u32 index = vkAvailExtensionList.find(identifier);
            if (u32MAX == index)
            {
                gos::logger::err ("extension %s not available!\n", identifier);
                ret = false;
            }
            else
            {
                foundExtensions[createInfo.enabledExtensionCount++] = vkAvailExtensionList(index)->extensionName;
            }
        }
    }

    if (ret)
    {
        gos::logger::log ("Creating vulkan instance with the following:\n");
        gos::logger::incIndent();
        gos::logger::log ("extensions (%d): ", createInfo.enabledExtensionCount);
        for (u32 i=0; i<createInfo.enabledExtensionCount; i++)
            gos::logger::log("%s ", createInfo.ppEnabledExtensionNames[i]);
        gos::logger::log("\n");

        gos::logger::log ("layers (%d): ", createInfo.enabledLayerCount);
        for (u32 i=0; i<createInfo.enabledLayerCount; i++)
            gos::logger::log("%s ", createInfo.ppEnabledLayerNames[i]);
        gos::logger::log("\n");

        VkResult result = vkCreateInstance(&createInfo, nullptr, out);
        if (VK_SUCCESS != result)
        {
            ret = false;
            gos::logger::log (eTextColor::red, "ERROR: vkCreateInstance => %s\n", string_VkResult(result));
        }
        else
            gos::logger::log (eTextColor::green,"OK\n");

        gos::logger::decIndent();
    }
    gos::logger::decIndent();
    return ret;
}


//*********************************************
bool gos::vulkanScanAndSelectAPhysicalDevices (const VkInstance &vkInstance, const VkSurfaceKHR &vkSurface, const gos::StringList &requiredExtensionList, sPhyDeviceInfo *out)
{
    gos::Allocator *allocator = gos::getScrapAllocator();
    gos::logger::log ("vulkanScanPhysicalDevices\n");
    gos::logger::incIndent();
    out->reset();

    //elenco dei device disponibili nel sistema
    u32 nDevices = 0;
    vkEnumeratePhysicalDevices(vkInstance, &nDevices, nullptr);
    if (0 == nDevices)
    {
        gos::logger::err ("no devices found!\n");
        gos::logger::decIndent();
        return false;
    }
    gos::logger::log ("found %d devices\n", nDevices);
    if (nDevices > 16)
        nDevices = 16;
    VkPhysicalDevice deviceList[16];
    vkEnumeratePhysicalDevices(vkInstance, &nDevices, deviceList);    

    //ne scelgo uno
    gos::logger::incIndent();
    for (u32 i=0; i<nDevices; i++)
    {
        //proprita' del device
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(deviceList[i], &deviceProperties);
        gos::logger::log ("dev name: %s\n", deviceProperties.deviceName);
        gos::logger::log ("dev index: %d\n", i);
        gos::logger::log ("dev type: %s\n", string_VkPhysicalDeviceType(deviceProperties.deviceType));

        //deve assolutamente essere una GPU dedicata, a meno che non sia la sola e unica GPU presente
        if (1 == nDevices)
        {
            if (deviceProperties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            {
                gos::logger::log (eTextColor::yellow, "WARN: this device is not VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU, but it's the only one available(%s)\n", string_VkPhysicalDeviceType(deviceProperties.deviceType));
            }
        }
        else
        {
            if (deviceProperties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            {
                gos::logger::log (eTextColor::red, "this device is not VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU\n");
                continue;
            }
        }

        //feature del device
        //VkPhysicalDeviceFeatures deviceFeatures;
        //vkGetPhysicalDeviceFeatures(deviceList[i], &deviceFeatures);
        

        //enumerazione delle estensioni supportate da questo device
        VkPhyDeviceExtensionList extList;
        extList.build (allocator, deviceList[i]);
        extList.printInfo();
        
        //verifichiamo che supporti tutte le extensioni richieste
        bool bIsGoodDevice = true;
        u32 iter;
        const char *identifier;
        requiredExtensionList.toStart(&iter);
        while (NULL != (identifier = requiredExtensionList.nextAsChar(&iter)))
        {
            const u32 index = extList.find (identifier);
            if (u32MAX == index)
            {
                gos::logger::log (eTextColor::yellow, "NOT GOOD: extension %s not available\n", identifier);
                bIsGoodDevice = false;
            }
            else
            {
                gos::logger::log (eTextColor::green, "extension %s is available\n", identifier);
            }
        }    

        //enumerazione delle queue di questo device
        u32 selectedGfxFamilyQIndex = u32MAX;
        u32 selectedComputeFamilyQIndex = u32MAX;
        gos::logger::log ("available family queues:\n");
        gos::logger::incIndent();
        {
            //vediamo quali queue sono supportate da questo device
            VkPhyDeviceQueueList list;
            list.build(allocator, deviceList[i]);
            for (u32 i2=0; i2<list.getCount(); i2++)
            {
                gos::logger::log ("queue family %d:\n", i2);
                gos::logger::incIndent();
                list.printQueueFamilyInfo(i2);

                //deve assolutamente supoprtare la fn di PRESENT
                VkBool32 bIsSupportedKHR = false;
                vkGetPhysicalDeviceSurfaceSupportKHR (deviceList[i], i2, vkSurface, &bIsSupportedKHR);
                if (!bIsSupportedKHR)
                    gos::logger::log (eTextColor::red, "does NOT support PRESENT to KHR surface\n");
                else
                {
                    if (u32MAX == selectedGfxFamilyQIndex)
                    if (list.support_VK_QUEUE_GRAPHICS_BIT(i2) && list.support_VK_QVK_QUEUE_TRANSFER_BIT(i2))
                        selectedGfxFamilyQIndex = i2;

                    if (list.support_VK_QUEUE_COMPUTE_BIT(i2))
                    {
                        if (u32MAX == selectedComputeFamilyQIndex)
                            selectedComputeFamilyQIndex = i2;
                        else
                        {
                            //preferisco una Q che supporti COMPUTE ma non supporti GFX, nella speranza di avere
                            //una Q di compute pura, preferibilmente diversa da quella gfx
                            if (!list.support_VK_QUEUE_GRAPHICS_BIT(i2))
                                selectedComputeFamilyQIndex = i2;
                        }
                    }
                }
                gos::logger::decIndent();
            }
        }
        gos::logger::decIndent();


        if (bIsGoodDevice && u32MAX != selectedGfxFamilyQIndex && u32MAX != selectedComputeFamilyQIndex)
        {
            out->vkDev = deviceList[i];
            out->devIndex = i;
            out->gfxQIndex = selectedGfxFamilyQIndex;
            out->computeQIndex = selectedComputeFamilyQIndex;
        }
    }
    gos::logger::decIndent();

    gos::logger::decIndent();
    if (!out->isValid())
        return false;

    //recupero alcune props del device
    vkGetPhysicalDeviceMemoryProperties (out->vkDev, &out->vkMemoryProperties);
    return true;
}

/*********************************************
 * Dato il [vkPhyDevice] e una lista di estensioni richieste [requiredExtensionList], crea il device logico
 * create le queue e filla out_vulkan con queste informazioni
 */
bool gos::vulkanCreateDevice (sPhyDeviceInfo &vkPhyDevInfo, const gos::StringList &requiredExtensionList, sVkDevice *out_vulkan)
{
    assert (NULL != out_vulkan);

    gos::Allocator *allocator = gos::getScrapAllocator();

    bool ret = true;
    gos::logger::log ("vulkanCreateDevice()\n");
    gos::logger::incIndent();
    gos::logger::log ("creating with phyDev at index:%d, gfxQ at index %d, computeQ at index:%d\n", vkPhyDevInfo.devIndex, vkPhyDevInfo.gfxQIndex, vkPhyDevInfo.computeQIndex);

    //quali e quante queue mi servono?
    float queuePriority = 1.0f;
    VkDeviceQueueCreateInfo queueCreateInfo[2];
    u8 numOfQueue = 0;
    memset (queueCreateInfo, 0, sizeof(queueCreateInfo));
    queueCreateInfo[numOfQueue].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo[numOfQueue].queueFamilyIndex = vkPhyDevInfo.gfxQIndex;
    queueCreateInfo[numOfQueue].queueCount = 1;
    queueCreateInfo[numOfQueue].pQueuePriorities = &queuePriority;
    numOfQueue++;

    if (vkPhyDevInfo.gfxQIndex != vkPhyDevInfo.computeQIndex)
    {
        queueCreateInfo[numOfQueue].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo[numOfQueue].queueFamilyIndex = vkPhyDevInfo.computeQIndex;
        queueCreateInfo[numOfQueue].queueCount = 1;
        queueCreateInfo[numOfQueue].pQueuePriorities = &queuePriority;
        numOfQueue++;
    }
    
    //quali feature voglio usare?
    VkPhysicalDeviceFeatures deviceFeatures{};

    //creo il device
    const char *foundExtensions[128];
    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = queueCreateInfo;
    createInfo.queueCreateInfoCount = numOfQueue;
    createInfo.pEnabledFeatures = &deviceFeatures;        
    createInfo.enabledExtensionCount = 0;
    createInfo.ppEnabledExtensionNames = foundExtensions;


    //aggiungo le etensioni richieste
    VkPhyDeviceExtensionList extList;
    extList.build (allocator, vkPhyDevInfo.vkDev);
    {
        u32 iter;
        const char *identifier;
        requiredExtensionList.toStart(&iter);
        while (NULL != (identifier = requiredExtensionList.nextAsChar(&iter)))
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
        VkResult result = vkCreateDevice(vkPhyDevInfo.vkDev, &createInfo, nullptr, &out_vulkan->dev);
        if (VK_SUCCESS != result) 
        {
            gos::logger::err ("vkCreateDevice() returned %s\n", string_VkResult(result));
            ret = false;
        }
        else
        {
            gos::logger::log (eTextColor::green, "OK\n");

            out_vulkan->phyDevInfo = vkPhyDevInfo;
            if (vkPhyDevInfo.gfxQIndex == vkPhyDevInfo.computeQIndex)
                vkPhyDevInfo.computeQIndex = vkPhyDevInfo.gfxQIndex;

            //recupero l'handle della gfxQ
            vkGetDeviceQueue(out_vulkan->dev, vkPhyDevInfo.gfxQIndex, 0, &out_vulkan->gfxQ);
            vkGetDeviceQueue(out_vulkan->dev, vkPhyDevInfo.computeQIndex, 0, &out_vulkan->computeQ);
        }
    }
    
    //creo un command pool
    if (ret)
    {
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = out_vulkan->phyDevInfo.gfxQIndex;
        
        const VkResult result = vkCreateCommandPool(out_vulkan->dev, &poolInfo, nullptr, &out_vulkan->commandPool);
        if (VK_SUCCESS != result)
        {
            ret = false;
            gos::logger::err ("vkCreateCommandPool() error: %s\n", string_VkResult(result)); 
        }
    }

    gos::logger::decIndent();
    return ret;
}

//*********************************************
bool gos::vulkanCreateSwapChain (sVkDevice &vulkan, const VkSurfaceKHR &vkSurface, sSwapChainInfo *out)
{
    gos::logger::log("vulkanCreateSwapChain\n");
    gos::logger::incIndent();

    gos::Allocator *allocator = gos::getScrapAllocator();

    VkSurfaceCapabilitiesKHR vkSurfCapabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vulkan.phyDevInfo.vkDev, vkSurface, &vkSurfCapabilities);
    gos::logger::log ("surf capab\n");
    gos::logger::incIndent();
    gos::logger::log ("min/max image count:%d;%d\n", vkSurfCapabilities.minImageCount, vkSurfCapabilities.maxImageCount);
    gos::logger::log ("curremt width/height: %d;%d\n", vkSurfCapabilities.currentExtent.width, vkSurfCapabilities.currentExtent.height);
    gos::logger::decIndent();

    VPhyDevicekSurfaceFormatKHRList listOfSurfaceFormat;
    listOfSurfaceFormat.build (allocator, vulkan.phyDevInfo.vkDev, vkSurface);
    listOfSurfaceFormat.printInfo();

    VPhyDevicekSurfacePresentModesKHRList listOfPresentMode;
    listOfPresentMode.build (allocator, vulkan.phyDevInfo.vkDev, vkSurface);
    listOfPresentMode.printInfo();


    //voglio creare una swap chain che abbia:
    //  VK_FORMAT_B8G8R8A8_SRGB
    //  VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
    //  VK_PRESENT_MODE_MAILBOX_KHR oppure VK_PRESENT_MODE_FIFO_RELAXED_KHR oppure VK_PRESENT_MODE_FIFO_KHR (in ordine di priorità)
    //  image count almeno di 2, preferibilmente 3
    out->imageFormat = VK_FORMAT_B8G8R8A8_SRGB;
    out->colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    out->imageExtent = vkSurfCapabilities.currentExtent;
    out->imageCount = 3;
    if (out->imageCount > vkSurfCapabilities.maxImageCount)
        out->imageCount = vkSurfCapabilities.maxImageCount;
    if (out->imageCount < vkSurfCapabilities.minImageCount)
        out->imageCount = vkSurfCapabilities.minImageCount;
    
    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
    if (listOfPresentMode.exists(VK_PRESENT_MODE_MAILBOX_KHR))
        presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
    else if (listOfPresentMode.exists(VK_PRESENT_MODE_FIFO_RELAXED_KHR))
        presentMode = VK_PRESENT_MODE_FIFO_RELAXED_KHR;

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = vkSurface;
    createInfo.minImageCount = out->imageCount;
    createInfo.imageFormat = out->imageFormat;
    createInfo.imageColorSpace = out->colorSpace;
    createInfo.imageExtent = out->imageExtent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.preTransform = vkSurfCapabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    gos::logger::log ("Attemp to create a swapchain with the following:\n");
    gos::logger::incIndent();
        gos::logger::log ("minImageCount = %d\n", createInfo.minImageCount);
        gos::logger::log ("imageFormat = %s\n", string_VkFormat(createInfo.imageFormat));
        gos::logger::log ("imageColorSpace = %s\n", string_VkColorSpaceKHR(createInfo.imageColorSpace));
        gos::logger::log ("imageExtent = %d;%d\n", createInfo.imageExtent.width, createInfo.imageExtent.height);
        gos::logger::log ("imageUsage = %s\n", string_VkImageUsageFlags(createInfo.imageUsage).c_str());
        gos::logger::log ("imageSharingMode = %s\n", string_VkSharingMode(createInfo.imageSharingMode));
        gos::logger::log ("presentMode = %s\n", string_VkPresentModeKHR(createInfo.presentMode));
    gos::logger::decIndent();

    VkResult result = vkCreateSwapchainKHR (vulkan.dev, &createInfo, nullptr, &out->vkSwapChain);
    if (VK_SUCCESS != result)
    {
        out->reset();
        gos::logger::err ("%s\n", string_VkResult(result));
    }
    else
    {
        //recupero gli handle delle image. Ho chiesto di crearne almeno [createInfo.minImageCount] ma il driver potrebbe averne create di +
        vkGetSwapchainImagesKHR (vulkan.dev, out->vkSwapChain, &out->imageCount, NULL);
        if (out->imageCount > SWAPCHAIN_NUM_MAX_IMAGES)
        {
            gos::logger::err ("driver created a swapchain width %d images. GOS support up to %d images\n", out->imageCount, SWAPCHAIN_NUM_MAX_IMAGES);
            out->imageCount = SWAPCHAIN_NUM_MAX_IMAGES;
        }

        vkGetSwapchainImagesKHR (vulkan.dev, out->vkSwapChain, &out->imageCount, out->vkImageList);
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

            result = vkCreateImageView (vulkan.dev, &imgViewCreateInfo, nullptr, &out->vkImageListView[i]);
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
bool gos::vulkanFindBestDepthFormat (const sPhyDeviceInfo &vkPhyDevInfo, VkFormat *out_depthFormat)
{
    // adattato da https://github.com/SaschaWillems/Vulkan/blob/master/base/VulkanTools.cpp
    // Since all depth formats may be optional, we need to find a suitable depth format to use
    // Start with the highest precision packed format
    const VkFormat formatList[] = {
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D24_UNORM_S8_UINT,
        VK_FORMAT_D16_UNORM_S8_UINT,
        VK_FORMAT_D16_UNORM
    };

    constexpr u32 N = sizeof(formatList) / sizeof(VkFormat);
    for (u32 i=0; i<N; i++)
    {
        VkFormatProperties formatProps;
        vkGetPhysicalDeviceFormatProperties (vkPhyDevInfo.vkDev, formatList[i], &formatProps);
        if (formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
        {
            *out_depthFormat = formatList[i];
            return true;
        }
    }

    return false;
}

//*********************************************
bool gos::vulkanFindBestDepthStencilFormat (const sPhyDeviceInfo &vkPhyDevInfo, VkFormat* out_depthStencilFormat)
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
        vkGetPhysicalDeviceFormatProperties (vkPhyDevInfo.vkDev, formatList[i], &formatProps);
        if (formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
        {
            *out_depthStencilFormat = formatList[i];
            return true;
        }
    }

    return false;
}

//*********************************************
bool gos::vulkanGetMemoryType (const sPhyDeviceInfo &vkPhyDevInfo, uint32_t typeBits, VkMemoryPropertyFlags properties, u32 *out_index)
{
    assert (NULL != out_index);
    for (u32 i = 0; i < vkPhyDevInfo.vkMemoryProperties.memoryTypeCount; i++)
    {
        if ((typeBits & 1) == 1)
        {
            if ((vkPhyDevInfo.vkMemoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
            {
                *out_index = i;
                return true;
            }
        }
        typeBits >>= 1;
    }

    return false;
}