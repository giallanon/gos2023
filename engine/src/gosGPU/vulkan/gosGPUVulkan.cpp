#include "gosGPUVulkan.h"
#include "gosGPUVukanHelpers.h"
#include "../../../gos/gos.h"
#include "../gosGPUUtils.h"

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
            gos::logger::log("[%s]  ", createInfo.ppEnabledExtensionNames[i]);
        gos::logger::log("\n");

        gos::logger::log ("layers (%d): ", createInfo.enabledLayerCount);
        for (u32 i=0; i<createInfo.enabledLayerCount; i++)
            gos::logger::log("[%s]  ", createInfo.ppEnabledLayerNames[i]);
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
    vkEnumeratePhysicalDevices (vkInstance, &nDevices, nullptr);
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
        sPhyDeviceInfo::sQueueInfo selectedQueue_gfx;
        sPhyDeviceInfo::sQueueInfo selectedQueue_compute;
        sPhyDeviceInfo::sQueueInfo selectedQueue_transfer;
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
                    if (u32MAX == selectedQueue_gfx.familyIndex)
                    {
                        if (list.support_VK_QUEUE_GRAPHICS_BIT(i2))
                        {
                            selectedQueue_gfx.familyIndex = i2;
                            selectedQueue_gfx.count = list.get(i2)->queueCount;
                        }
                    }

                    if (list.support_VK_QUEUE_COMPUTE_BIT(i2))
                    {
                        if (u32MAX == selectedQueue_compute.familyIndex)
                        {
                            selectedQueue_compute.familyIndex = i2;
                            selectedQueue_compute.count = list.get(i2)->queueCount;
                        }
                        else
                        {
                            //preferisco una Q che supporti COMPUTE ma non supporti GFX, nella speranza di avere
                            //una Q di compute pura, preferibilmente diversa da quella gfx
                            if (!list.support_VK_QUEUE_GRAPHICS_BIT(i2))
                            {
                                selectedQueue_compute.familyIndex = i2;
                                selectedQueue_compute.count = list.get(i2)->queueCount;
                            }
                        }
                    }

                    
                    //cerco di trovare una Q dedicata al transfer che supporti espressamente solo quello
                    if (list.support_VK_QVK_QUEUE_TRANSFER_BIT(i2) && !list.support_VK_QUEUE_GRAPHICS_BIT(i2) && !list.support_VK_QUEUE_COMPUTE_BIT(i2))
                    {
                        if (u32MAX == selectedQueue_transfer.familyIndex)                        
                        {
                            selectedQueue_transfer.familyIndex = i2;
                            selectedQueue_transfer.count = list.get(i2)->queueCount;
                        }
                        else
                        {
                            if (list.get(i2)->queueCount > selectedQueue_transfer.count)
                            {
                                selectedQueue_transfer.familyIndex = i2;
                                selectedQueue_transfer.count = list.get(i2)->queueCount;
                            }
                        }
                    }
                    
                }
                gos::logger::decIndent();
            }

            //Per la transferQ... se non ne ho trovata una dedicata allora uso la GFX o la COMPUTER che
            //sono garantite supportare la fn di transfer anche se non espressamente indicato
            if (u32MAX == selectedQueue_transfer.familyIndex)
            {
                //tra le 2, scelgo quella con il maggior numero di code
                if (selectedQueue_gfx.count > selectedQueue_compute.count)
                    selectedQueue_transfer = selectedQueue_gfx;
                else
                    selectedQueue_transfer = selectedQueue_compute;
            }
        }
        gos::logger::decIndent();


        if (bIsGoodDevice && u32MAX != selectedQueue_gfx.familyIndex && u32MAX != selectedQueue_compute.familyIndex && u32MAX != selectedQueue_transfer.familyIndex)
        {
            out->vkDev = deviceList[i];
            out->devIndex = i;
            
            out->queue_gfx = selectedQueue_gfx;
            out->queue_compute = selectedQueue_compute;
            out->queue_transfer = selectedQueue_transfer;
        }
    }
    gos::logger::decIndent();


    //recupero alcune props del device
    if (out->isValid())
    {
        gos::logger::log ("GetPhysicalDeviceMemoryProperties\n");
        gos::logger::incIndent();
        vkGetPhysicalDeviceMemoryProperties (out->vkDev, &out->vkMemoryProperties);
    
        const VkPhysicalDeviceMemoryProperties *info = &out->vkMemoryProperties;
        gos::logger::log ("memory heap count:%d\n", info->memoryHeapCount);
        gos::logger::incIndent();
        for (u32 i = 0; i < info->memoryHeapCount; i++)
        {
            gos::logger::log ("index:%d\tsize= %" PRIu64 "B ", i, info->memoryHeaps[i].size);
            if (((info->memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) != 0))
                gos::logger::log (", HEAP_DEVICE_LOCAL");
            gos::logger::log ("\n");
        }
        gos::logger::decIndent();

        
        gos::logger::log ("memory type count: %d\n", info->memoryTypeCount);
        gos::logger::incIndent();
        for (u32 i = 0; i < info->memoryTypeCount; i++)
        {
            gos::logger::log ("heap-index:%d", info->memoryTypes[i].heapIndex);

            if ((info->memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) != 0)
                gos::logger::log (", DEVICE_LOCAL");
            if ((info->memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT ) != 0)
                gos::logger::log (", HOST_VISIBLE");
            if ((info->memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT ) != 0)
                gos::logger::log (", HOST_COHERENT");
            if ((info->memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT ) != 0)
                gos::logger::log (", HOST_CACHED");
            if ((info->memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT ) != 0)
                gos::logger::log (", LAZILY_ALLOCATED");


            if ((info->memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_PROTECTED_BIT  ) != 0)
                gos::logger::log (", PROTECTED");
            if ((info->memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD  ) != 0)
                gos::logger::log (", DEVICE_COHERENT_BIT_AMD");
            if ((info->memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_UNCACHED_BIT_AMD  ) != 0)
                gos::logger::log (", DEVICE_UNCACHED_BIT_AMD");
            if ((info->memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_RDMA_CAPABLE_BIT_NV  ) != 0)
                gos::logger::log (", RDMA_CAPABLE");

            gos::logger::log ("\n");
        }
        gos::logger::decIndent();

            
        gos::logger::decIndent();
    }



    gos::logger::decIndent();
    if (!out->isValid())
        return false;
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
    gos::logger::log ("creating with phyDev at index:%d\n   gfxQ familyIndex:%d, count=%d\n   computeQ familyIndex:%d, count=%d\n   transferQ familyIndex:%d, count=%d\n", 
                        vkPhyDevInfo.devIndex,
                        vkPhyDevInfo.queue_gfx.familyIndex, vkPhyDevInfo.queue_gfx.count,
                        vkPhyDevInfo.queue_compute.familyIndex, vkPhyDevInfo.queue_compute.count,
                        vkPhyDevInfo.queue_transfer.familyIndex, vkPhyDevInfo.queue_transfer.count);

    //quali e quante queue mi servono?
    float queuePriority = 1.0f;
    VkDeviceQueueCreateInfo queueCreateInfo[16];
    u8 numOfQueue = 0;
    memset (queueCreateInfo, 0, sizeof(queueCreateInfo));
    queueCreateInfo[numOfQueue].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo[numOfQueue].queueFamilyIndex = vkPhyDevInfo.queue_gfx.familyIndex;
    queueCreateInfo[numOfQueue].queueCount = 1; //vkPhyDevInfo.queue_gfx.count;     TODO indagare se avere + di 1 Q ha senso
    queueCreateInfo[numOfQueue].pQueuePriorities = &queuePriority;
    numOfQueue++;

    if (vkPhyDevInfo.queue_compute.familyIndex != vkPhyDevInfo.queue_gfx.familyIndex)
    {
        queueCreateInfo[numOfQueue].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo[numOfQueue].queueFamilyIndex = vkPhyDevInfo.queue_compute.familyIndex;
        queueCreateInfo[numOfQueue].queueCount = 1;
        queueCreateInfo[numOfQueue].pQueuePriorities = &queuePriority;
        numOfQueue++;
    }

    if (vkPhyDevInfo.queue_transfer.familyIndex != vkPhyDevInfo.queue_gfx.familyIndex && 
        vkPhyDevInfo.queue_transfer.familyIndex != vkPhyDevInfo.queue_compute.familyIndex)
    {
        queueCreateInfo[numOfQueue].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo[numOfQueue].queueFamilyIndex = vkPhyDevInfo.queue_transfer.familyIndex;
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

        VkResult result = vkCreateDevice (vkPhyDevInfo.vkDev, &createInfo, nullptr, &out_vulkan->dev);
        if (VK_SUCCESS != result) 
        {
            gos::logger::err ("vkCreateDevice() returned %s\n", string_VkResult(result));
            ret = false;
        }
        else
        {
            out_vulkan->phyDevInfo = vkPhyDevInfo;

            //recupero l'handle della gfxQ
            sVkDevice::sQueueInfo *queueInfo = out_vulkan->getQueueInfo (eGPUQueueType::gfx);
            queueInfo->familyIndex = vkPhyDevInfo.queue_gfx.familyIndex;
            vkGetDeviceQueue (out_vulkan->dev, queueInfo->familyIndex, 0, &queueInfo->vkQueueHandle);
            

            queueInfo = out_vulkan->getQueueInfo (eGPUQueueType::compute);
            queueInfo->familyIndex = vkPhyDevInfo.queue_compute.familyIndex;
            vkGetDeviceQueue (out_vulkan->dev, queueInfo->familyIndex, 0, &queueInfo->vkQueueHandle);
            if (queueInfo->familyIndex == vkPhyDevInfo.queue_gfx.familyIndex)
                queueInfo->isAnAliasFor = eGPUQueueType::gfx;

            queueInfo = out_vulkan->getQueueInfo (eGPUQueueType::transfer);
            queueInfo->familyIndex = vkPhyDevInfo.queue_transfer.familyIndex;
            vkGetDeviceQueue (out_vulkan->dev, queueInfo->familyIndex, 0, &queueInfo->vkQueueHandle);
            if (queueInfo->familyIndex == vkPhyDevInfo.queue_gfx.familyIndex)
                queueInfo->isAnAliasFor = eGPUQueueType::gfx;
            else if (queueInfo->familyIndex == vkPhyDevInfo.queue_compute.familyIndex)
                queueInfo->isAnAliasFor = eGPUQueueType::compute;
        }
    }
    
    //creo un command pool per ogni Q
    if (ret)
    {
        const eGPUQueueType queueType[] = { eGPUQueueType::gfx, eGPUQueueType::compute, eGPUQueueType::transfer };
        const u32 NUM_Q =  sizeof(queueType) / sizeof(eGPUQueueType);
        for (u32 i=0; i<NUM_Q; i++)
        {
            sVkDevice::sQueueInfo *queueInfo = out_vulkan->getQueueInfo (queueType[i]);
            if (eGPUQueueType::unknown == queueInfo->isAnAliasFor)
            {
                VkCommandPoolCreateInfo poolInfo{};
                poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
                poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
                poolInfo.queueFamilyIndex = queueInfo->familyIndex;
                
                gos::logger::log ("creating CommandPool for queue %d [%s]\n", i, gpu::enumToString(queueType[i]));
                const VkResult result = vkCreateCommandPool (out_vulkan->dev, &poolInfo, nullptr, &queueInfo->vkPoolHandle);
                if (VK_SUCCESS != result)
                {
                    ret = false;
                    gos::logger::err ("vkCreateCommandPool() error: %s\n", string_VkResult(result)); 
                }
            }
        }

        for (u32 i=0; i<NUM_Q; i++)
        {
            sVkDevice::sQueueInfo *queueInfo = out_vulkan->getQueueInfo (queueType[i]);
            if (eGPUQueueType::unknown != queueInfo->isAnAliasFor)
            {
                //questa Q e' un alias per un'altra Q, quindi condivide lo stesso command pool      
                gos::logger::log ("CommandPool for queue %d [%s] is an alias of [%s]\n", i, gpu::enumToString(queueType[i]), gpu::enumToString(queueInfo->isAnAliasFor));          
                queueInfo->vkPoolHandle = out_vulkan->getQueueInfo(queueInfo->isAnAliasFor)->vkPoolHandle;
            }

            
        }
    }

    if (ret)
        gos::logger::log (eTextColor::green, "OK\n");
        
    gos::logger::decIndent();
    return ret;
}

//*********************************************
bool gos::vulkanCreateSwapChain (sVkDevice &vulkan, const VkSurfaceKHR &vkSurface, bool bVSync, sSwapChainInfo *out)
{
    gos::logger::log("vulkanCreateSwapChain\n");
    gos::logger::incIndent();

    gos::Allocator *allocator = gos::getScrapAllocator();

    VkSurfaceCapabilitiesKHR vkSurfCapabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vulkan.phyDevInfo.vkDev, vkSurface, &vkSurfCapabilities);
    gos::logger::log ("surf capab\n");
    gos::logger::incIndent();
    gos::logger::log ("min/max image count:%d;%d\n", vkSurfCapabilities.minImageCount, vkSurfCapabilities.maxImageCount);
    gos::logger::log ("current width/height: %d;%d\n", vkSurfCapabilities.currentExtent.width, vkSurfCapabilities.currentExtent.height);
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
    
    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;    //vsync, sempre disponibile
    if (!bVSync)
    {
        if (listOfPresentMode.exists(VK_PRESENT_MODE_MAILBOX_KHR))
            presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
        else if (listOfPresentMode.exists(VK_PRESENT_MODE_FIFO_RELAXED_KHR))
            presentMode = VK_PRESENT_MODE_FIFO_RELAXED_KHR;
    }
    
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

//*********************************************
bool gos::vulkanCreateBuffer (const sVkDevice &vulkan, u32 sizeInByte, VkBufferUsageFlags usage, VkMemoryPropertyFlags memProperties,
                                bool bCanBeUsedBy_gfxQ, bool bCanBeUsedBy_computeQ, bool bCanBeUsedBy_transferQ,
                                VkBuffer *out_vkBufferHandle, VkDeviceMemory *out_vkMemHandle)
{
    assert (NULL != out_vkBufferHandle);
    assert (NULL != out_vkMemHandle);
    *out_vkBufferHandle = VK_NULL_HANDLE;
    *out_vkMemHandle = VK_NULL_HANDLE;

    //Se la risorsa e' usata da una sola queueFamiliIndex, allora e' EXCLUSIVE, altrimenti e' CONCURRENT
    VkSharingMode sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    u32 queueIndexList[3];
    u32 queueCount = 0;
    if (bCanBeUsedBy_gfxQ || bCanBeUsedBy_computeQ || bCanBeUsedBy_transferQ)
    {
        const u32 familyIndex[] = {
            vulkan.getQueueInfo(eGPUQueueType::gfx)->familyIndex,
            vulkan.getQueueInfo(eGPUQueueType::compute)->familyIndex,
            vulkan.getQueueInfo(eGPUQueueType::transfer)->familyIndex
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
    VkResult result = vkCreateBuffer (vulkan.dev, &createInfo, nullptr, out_vkBufferHandle);
    if (VK_SUCCESS != result)
    {
        gos::logger::err (" gos::vulkanCreateBuffer(size=%d) => vkCreateBuffer() => %s\n", sizeInByte, string_VkResult(result));
        return false;
    }

    //alloco memoria
    VkMemoryRequirements memReqs;
    vkGetBufferMemoryRequirements (vulkan.dev, *out_vkBufferHandle, &memReqs);       

    VkMemoryAllocateInfo memAllloc{};
	memAllloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memAllloc.allocationSize = memReqs.size;
    vulkanGetMemoryType (vulkan.phyDevInfo, memReqs.memoryTypeBits, memProperties, &memAllloc.memoryTypeIndex);

	result = vkAllocateMemory (vulkan.dev, &memAllloc, nullptr, out_vkMemHandle);
    if (VK_SUCCESS != result)
    {
        gos::logger::err ("gos::vulkanCreateBuffer() => vkAllocateMemory() => %s\n", string_VkResult(result));
        return false;
    }

    //bindo il buffer alla memoria allocata
    result = vkBindBufferMemory (vulkan.dev, *out_vkBufferHandle, *out_vkMemHandle, 0);
    if (VK_SUCCESS != result)
    {
        gos::logger::err ("gos::vulkanCreateBuffer() => vkBindBufferMemory() => %s\n", string_VkResult(result));
        return false;
    }

    return true;    
}



//*********************************************
bool gos::vulkanCreateCommandBuffer (const sVkDevice &vulkan, eGPUQueueType whichQ, VkCommandBuffer *out_handle)
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = vulkan.getQueueInfo(whichQ)->vkPoolHandle;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    const VkResult result = vkAllocateCommandBuffers (vulkan.dev, &allocInfo, out_handle);
    if (result == VK_SUCCESS)
        return true;
    
    gos::logger::log ("vulkanCreateCommandBuffer() => vkAllocateCommandBuffers() => %s\n", string_VkResult(result));
    return false;
}

//*********************************************
bool gos::vulkanDeleteCommandBuffer (const sVkDevice &vulkan, eGPUQueueType whichQ, VkCommandBuffer &vkHandle)
{
    VkCommandBuffer vkCmdBufferList[] = { vkHandle };

    vkFreeCommandBuffers (vulkan.dev, vulkan.getQueueInfo(whichQ)->vkPoolHandle, 1, vkCmdBufferList);
    return true;
}