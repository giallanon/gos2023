#include "gosGPUVulkan.h"
#include "gosGPUVukanHelpers.h"
#include "../../../gos/gos.h"
#include "../gosGPUUtils.h"

using namespace gos;


//*********************************************
static u32 toVulkanVersion (eVulkanVersion v)
{
    switch (v)
    {
    default:
        DBGBREAK;
        return VK_API_VERSION_1_0;
    case eVulkanVersion::v1_0:  return VK_API_VERSION_1_0;
    case eVulkanVersion::v1_1:  return VK_API_VERSION_1_1;
    case eVulkanVersion::v1_2:  return VK_API_VERSION_1_2;
    case eVulkanVersion::v1_3:  return VK_API_VERSION_1_3;
    }
}

//*********************************************
bool gos::vulkanCreateInstance (VkInstance *out, const gos::StringList &requiredValidationLayerList, const gos::StringList &requiredExtensionList, eVulkanVersion vulkanVersion)
{
    gos::logger::log ("vulkanCreateInstance()\n");
    gos::logger::incIndent();


    //recupero la versione di vulkan installata
    auto FN_vkEnumerateInstanceVersion = PFN_vkEnumerateInstanceVersion(vkGetInstanceProcAddr(nullptr, "vkEnumerateInstanceVersion"));
    if(FN_vkEnumerateInstanceVersion == nullptr)
        gos::logger::log ("Installed Vulkan version is 1.0\n");
    else
    {
        uint32_t instanceVersion;
        FN_vkEnumerateInstanceVersion (&instanceVersion);
        gos::logger::log ("Installed Vulkan version is %d.%d.%d\n", VK_API_VERSION_MAJOR(instanceVersion), VK_API_VERSION_MINOR(instanceVersion), VK_API_VERSION_PATCH(instanceVersion));
    }
    gos::logger::log ("Target Vulkan version is %d.%d.%d\n", VK_API_VERSION_MAJOR(toVulkanVersion(vulkanVersion)), VK_API_VERSION_MINOR(toVulkanVersion(vulkanVersion)), VK_API_VERSION_PATCH(toVulkanVersion(vulkanVersion)));


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
    appInfo.apiVersion = toVulkanVersion (vulkanVersion);


    VkInstanceCreateInfo createInfo{};
    const char *foundValidationLayers[128];
    const char *foundExtensions[128];
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledLayerCount = 0;
    createInfo.ppEnabledLayerNames = foundValidationLayers;
    createInfo.enabledExtensionCount = 0;
    createInfo.ppEnabledExtensionNames = foundExtensions;

#ifdef DEBUG_VULKAN_SYNC
	VkValidationFeatureEnableEXT enabledValidationFeatures[] =
	{
		VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT,
        VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT,
        VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT
	};

	VkValidationFeaturesEXT validationFeatures = { VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT };
	validationFeatures.enabledValidationFeatureCount = sizeof(enabledValidationFeatures) / sizeof(enabledValidationFeatures[0]);
	validationFeatures.pEnabledValidationFeatures = enabledValidationFeatures;

	createInfo.pNext = &validationFeatures;
#endif

    //elenco dei validation layer disponibili per la instance
    VkInstanceValidationLayersList vkAvailValidationLayerList;
    vkAvailValidationLayerList.build(allocator);
    vkAvailValidationLayerList.printInfo();
    {
        //verifico che tutti i layer richiesti siano disponibili
        u32 iter;
        const char *identifier;
        requiredValidationLayerList.toStart(&iter);
        while (NULL != (identifier = requiredValidationLayerList.next(&iter)))
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
        while (NULL != (identifier = requiredExtensionList.next(&iter)))
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
bool gos::vulkanScanAndSelectAPhysicalDevices (const VkInstance &vkInstance, const VkSurfaceKHR &vkSurfaceKHR, const gos::StringList &requiredExtensionList, eVulkanVersion vulkanVersion, sPhyDeviceInfo *out)
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
        gos::logger::log ("dev api version is %d.%d.%d\n", VK_API_VERSION_MAJOR(deviceProperties.apiVersion), VK_API_VERSION_MINOR(deviceProperties.apiVersion), VK_API_VERSION_PATCH(deviceProperties.apiVersion));

        if (deviceProperties.apiVersion < toVulkanVersion(vulkanVersion))
        {
            gos::logger::log (eTextColor::red, "DISCARDED! does not mach minimun API version required\n");
            continue;
        }

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

        //enumerazione delle estensioni supportate da questo device
        VkPhyDeviceExtensionList extList;
        extList.build (allocator, deviceList[i]);
        extList.printInfo();
        
        //verifichiamo che supporti tutte le extensioni richieste
        bool bIsGoodDevice = true;
        u32 iter;
        const char *identifier;
        requiredExtensionList.toStart(&iter);
        while (NULL != (identifier = requiredExtensionList.next(&iter)))
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

        //verifica del supporto a tutte le feature richieste
        VkPhyDeviceFeatures devFeatures;
        if (!devFeatures.checkPhysicalDeviceFeatures(deviceList[i], vulkanVersion))
        {
            bIsGoodDevice = false;
        }


        //enumerazione delle queue di questo device
        sPhyDeviceInfo::sQueueInfo selectedQueue_gfx;
        sPhyDeviceInfo::sQueueInfo selectedQueue_compute;
        sPhyDeviceInfo::sQueueInfo selectedQueue_transfer;
        if (bIsGoodDevice)
        {
            gos::logger::log ("available family queues:\n");
            gos::logger::incIndent();
            {
                VkPhyDeviceQueueList list;
                list.build(allocator, deviceList[i]);
                for (u32 i2 = 0; i2 < list.getCount(); i2++)
                {
                    gos::logger::log ("queue family %d:\n", i2);
                    gos::logger::incIndent();
                    list.printQueueFamilyInfo(i2);

                    //deve assolutamente supoprtare la fn di PRESENT
                    if (VK_NULL_HANDLE != vkSurfaceKHR)
                    {
                        VkBool32 bIsSupportedKHR = false;
                        vkGetPhysicalDeviceSurfaceSupportKHR (deviceList[i], i2, vkSurfaceKHR, &bIsSupportedKHR);
                        if (!bIsSupportedKHR)
                        {
                            gos::logger::log (eTextColor::red, "does NOT support PRESENT to KHR surface\n");
                            gos::logger::decIndent();
                            continue;
                        }
                    }

                    //determino il tipo di Q supportate
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
        }


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
        gos::logger::log ("vkGetPhysicalDeviceProperties\n");
        gos::logger::incIndent();
        vkGetPhysicalDeviceProperties (out->vkDev, &out->deviceProperties);
        gos::logger::log ("maxMemoryAllocationCount: %d\n", out->deviceProperties.limits.maxMemoryAllocationCount);
        gos::logger::log ("maxPushConstantsSize: %d\n", out->deviceProperties.limits.maxPushConstantsSize);
        gos::logger::log ("maxSamplerAllocationCount: %d\n", out->deviceProperties.limits.maxSamplerAllocationCount);
        gos::logger::log ("maxImageArrayLayers: %d\n", out->deviceProperties.limits.maxImageArrayLayers);
        gos::logger::log ("maxTexelBufferElements: %d\n", out->deviceProperties.limits.maxTexelBufferElements);
        gos::logger::log ("maxDescriptorSetSampledImages: %d\n", out->deviceProperties.limits.maxDescriptorSetSampledImages);
        gos::logger::log ("minUniformBufferOffsetAlignment: %d\n", out->deviceProperties.limits.minUniformBufferOffsetAlignment);
        gos::logger::log ("maxUniformBufferRange: %d\n", out->deviceProperties.limits.maxUniformBufferRange);
        gos::logger::log ("minStorageBufferOffsetAlignment: %d\n", out->deviceProperties.limits.minStorageBufferOffsetAlignment);
        gos::logger::log ("maxStorageBufferRange: %d\n", out->deviceProperties.limits.maxStorageBufferRange);
        gos::logger::log ("nonCoherentAtomSize: %d\n", out->deviceProperties.limits.nonCoherentAtomSize);
        gos::logger::decIndent();

        gos::logger::log ("GetPhysicalDeviceMemoryProperties\n");
        gos::logger::incIndent();
        vkGetPhysicalDeviceMemoryProperties (out->vkDev, &out->vkMemoryProperties);
    
        const VkPhysicalDeviceMemoryProperties *info = &out->vkMemoryProperties;
        gos::logger::log ("memory heap count:%d\n", info->memoryHeapCount);
        gos::logger::incIndent();
        for (u32 i = 0; i < info->memoryHeapCount; i++)
        {
            char temp[64];
            gos::string::format::memoryToKB_MB_GB (info->memoryHeaps[i].size, temp, sizeof(temp));
            //gos::string::format::U64(info->memoryHeaps[i].size, '.', temp, sizeof(temp));
            gos::logger::log ("index:%d\n  size= %s, flags=0x%08X", i, temp, info->memoryHeaps[i].flags);

            if (((info->memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) != 0))
                gos::logger::log (", HEAP_DEVICE_LOCAL");
            gos::logger::log ("\n");

            gos::logger::log ("  memory type:\n");
            VkMemoryPropertyFlags prevPropFlag = VK_MEMORY_PROPERTY_FLAG_BITS_MAX_ENUM;
            for (u32 i2 = 0; i2 < info->memoryTypeCount; i2++)
            {
                if (info->memoryTypes[i2].heapIndex == i)
                {
                    if (prevPropFlag != info->memoryTypes[i2].propertyFlags)
                    {
                        prevPropFlag = info->memoryTypes[i2].propertyFlags;
                        gos::logger::log ("    0x%08X", info->memoryTypes[i2].propertyFlags);
            
                        if ((info->memoryTypes[i2].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) != 0)
                            gos::logger::log (", DEVICE_LOCAL");
                        if ((info->memoryTypes[i2].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT ) != 0)
                            gos::logger::log (", HOST_VISIBLE");
                        if ((info->memoryTypes[i2].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT ) != 0)
                            gos::logger::log (", HOST_COHERENT");
                        if ((info->memoryTypes[i2].propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT ) != 0)
                            gos::logger::log (", HOST_CACHED");
                        if ((info->memoryTypes[i2].propertyFlags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT ) != 0)
                            gos::logger::log (", LAZILY_ALLOCATED");
                        if ((info->memoryTypes[i2].propertyFlags & VK_MEMORY_PROPERTY_PROTECTED_BIT  ) != 0)
                            gos::logger::log (", PROTECTED");
                        if ((info->memoryTypes[i2].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD  ) != 0)
                            gos::logger::log (", DEVICE_COHERENT_BIT_AMD");
                        if ((info->memoryTypes[i2].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_UNCACHED_BIT_AMD  ) != 0)
                            gos::logger::log (", DEVICE_UNCACHED_BIT_AMD");
                        if ((info->memoryTypes[i2].propertyFlags & VK_MEMORY_PROPERTY_RDMA_CAPABLE_BIT_NV  ) != 0)
                            gos::logger::log (", RDMA_CAPABLE");
            
                        gos::logger::log ("\n");
                    }
                }
            }            
        }
        gos::logger::decIndent();

        
        

            
        gos::logger::decIndent();
    }



    gos::logger::decIndent();
    if (!out->isValid())
        return false;
    return true;
}



