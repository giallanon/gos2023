#include "gosGPU.h"
#include "gosGPUUtils.h"
#include "../gos/string/gosStringList.h"
#include "../gos/gos.h"
#include "../gos/memory/gosAllocatorHeap.h"

using namespace gos;

PFN_vkCmdPushDescriptorSetKHR   GPU::vkCmdPushDescriptorSetKHR = VK_NULL_HANDLE;

typedef gos::AllocatorHeap<gos::AllocPolicy_Track_simple, gos::AllocPolicy_Thread_Safe>		GOSGPUMemAllocatorTS;

//************************************
static VKAPI_ATTR VkBool32 VKAPI_CALL GOS_vulkanDebugCallback (VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, UNUSED_PARAM(VkDebugUtilsMessageTypeFlagsEXT messageType), const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, UNUSED_PARAM(void* pUserData)) 
{
    char prefix[32];
    if ((messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT))
        sprintf_s (prefix, sizeof(prefix), "VULKAN [verbose]=> ");
    else if ((messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT))
        sprintf_s (prefix, sizeof(prefix), "VULKAN [info   ]=> ");
    else if ((messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT))
        sprintf_s (prefix, sizeof(prefix), "VULKAN [warn   ]=> ");
    else if ((messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT))
        sprintf_s (prefix, sizeof(prefix), "VULKAN [error  ]=> ");
    else
        sprintf_s (prefix, sizeof(prefix), "VULKAN [unknown]=> ");

    gos::logger::logWithPrefix (eTextColor::magenta, prefix, "%s\n\n", pCallbackData->pMessage);
    return VK_FALSE;
}


//********************************************************** 
GPU::GPU()
{
    this->allocator = NULL;
    vkInstance = VK_NULL_HANDLE;
    vkSurface = VK_NULL_HANDLE;
    vkDebugMessenger = VK_NULL_HANDLE;
    defaultViewportHandle.setInvalid();
    defaultRTHandle.setInvalid();
    defaultDepthStencil.handle.setInvalid();
    currentSwapChainImageIndex = 0;
    bRecreateSwapChainOnNextFrame = false;
    bSwapChainRecreatedDuringThisFrame = false;
    helperStagingBuffer.setInvalid();
}

//********************************************************** 
GPU::~GPU()
{ 
    deinit();
}

//************************************
void GPU::deinit()
{
    if (NULL == allocator)
        return;

    gos::logger::log ("GPU::deinit()\n");
    gos::logger::incIndent();

        deleteResource (helperStagingBuffer);
        helperImmediateTransferCmd.unsetup();

        //delete dei Sampler
        {
            auto list = samplerDescrHashMap._queryList();
            for (u32 i=0; i<list->getNElem(); i++)
            {
                GPUSamplerHandle h = list->queryElem(i).value;
                priv_samplerDelete (h);
            }
        }
        
        toBeDeletedBuilder.deleteAll();
        toBeDeletedBuilder.unsetup();

        frameBufferDependentOnSwapChainList.unsetup();

        //elimino l'handle del default RT
        renderTargetList.release (defaultRTHandle);

        deleteResource(defaultDepthStencil.handle);
        //depthStencilList.release(defaultDepthStencil.handle);

        //elimino la vport di default
        deleteResource (defaultViewportHandle);

        priv_deinitandleLists();
        priv_deinitVulkan();
        //priv_deinitWindowSystem();
    gos::logger::decIndent();

    GOSDELETE(gos::getSysHeapAllocator(), allocator);
    allocator = NULL;
}    

//********************************************************** 
bool GPU::init (GOSWinHandle mainWin, bool vSyncIN)
{
    vSync = vSyncIN;

    gos::logger::log ("GPU::init\n");
    gos::logger::incIndent();

    //Creo un allocatore dedicato per la GPU
    GOSGPUMemAllocatorTS *gpuAllocator = GOSNEW(gos::getSysHeapAllocator(), GOSGPUMemAllocatorTS)("GPU");
    gpuAllocator->setup (1024 * 1024 * 128); //128MB
    this->allocator = gpuAllocator;

    //liste varie
    toBeDeletedBuilder.setup();
    frameBufferDependentOnSwapChainList.setup (allocator, 128);


    //init vero e proprio
    bool bSuccess = false;
    while (1)
    {
        //if (!priv_initWindowSystem (width, height, appName))
        //    break;
        this->window.winH = mainWin;

        if (!priv_initHandleLists())
            break;
        if (!priv_initVulkan(eVulkanVersion::v1_3))
            break;
        bSuccess = true;
        break;
    }

    //default viewport
    viewport_create ("0", "0", "0-", "0-", &defaultViewportHandle);

    //default render target che e' in sostanza bindata alla viewport
    {
        gpu::RenderTarget *rt = renderTargetList.reserve (&defaultRTHandle);
        rt->reset();
        rt->format = vulkan.swapChainInfo.imageFormat;
        rt->image = VK_NULL_HANDLE;
        rt->vkMemHandle = VK_NULL_HANDLE;
        rt->view = NULL;
        rt->width = vulkan.swapChainInfo.imageExtent.width;
        rt->height = vulkan.swapChainInfo.imageExtent.height;
    }

    //default depth stencil
    {
        const bool bWithStencil = false;
        VkFormat depthStencilFormat = VK_FORMAT_UNDEFINED;
        if (bWithStencil)
            gos::vulkanFindBestDepthStencilFormat (vulkan.phyDevInfo, &depthStencilFormat);
        else
            gos::vulkanFindBestDepthOnlyFormat (vulkan.phyDevInfo, &depthStencilFormat);

        depthStencil_create (gpu::fromVulkan(depthStencilFormat), "0-", "0-", false, &defaultDepthStencil.handle);
        gos::gpu::DepthStencil *s;
        if (depthStencilList.fromHandleToPointer (defaultDepthStencil.handle, &s))
        {
            defaultDepthStencil.vkFormat = s->depthFormat;
            defaultDepthStencil.gosFormat = gpu::fromVulkan(s->depthFormat);
        }

        gos::logger::log ("default zbuffer created with format: %s\n", enumToString(defaultDepthStencil.gosFormat));
    }

    //fine
    if (bSuccess)
        gos::logger::log (eTextColor::green, "GPU::init => OK\n");
    else
        gos::logger::err ("GPU::init => failed\n");
    gos::logger::decIndent();
    
    return bSuccess;
}

//**********************************************************
bool GPU::priv_initVulkan (eVulkanVersion vulkanVersion)
{
    gos::logger::log ("GPU::priv_initVulkan()\n");
    gos::Allocator *scrapAllocator = gos::getScrapAllocator();

    //creazione di Vulkan instance
    {
        gos::StringList vkInstance_requiredExtensionList(scrapAllocator);
        gos::StringList vkInstance_requiredValidationLayerList(scrapAllocator);

        //GLFW ha bisogno di un po' di estensioni di vulkan, le recupero e le addo all'elenco delle estensioni necessarie
        {
            u32 glfwExtensionCount = 0;
            const char **glfwExtensions;
            glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
            for (u32 i = 0; i < glfwExtensionCount; i++)
                vkInstance_requiredExtensionList.add (glfwExtensions[i]);
        }
        vkInstance_requiredExtensionList.add (VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

#ifdef _DEBUG
        vkInstance_requiredValidationLayerList.add ("VK_LAYER_KHRONOS_validation");
        vkInstance_requiredExtensionList.add (VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

        //creazione dell'istanza di vulkan
        if (!vulkanCreateInstance (&vkInstance, vkInstance_requiredValidationLayerList, vkInstance_requiredExtensionList, vulkanVersion))
        {
            gos::logger::err ("problem creating vulkan instance\n");
            return false;
        }
    }
    
    //aggiungo una callback per il layer di debug, giusto per printare i msg di vulkan in un bel colore rosa
#ifdef _DEBUG    
    priv_vulkanAddDebugCallback();
#endif

    //creo una surface basata sulla [window]
    //GLFW fa tutto da solo, ma in linea di massima questa parte sarebbe dipendente da piattaforma
    GLFWwindow *glfWin = window.getGLF();
    VkResult result = glfwCreateWindowSurface(vkInstance, glfWin, nullptr, &vkSurface);
    if (VK_SUCCESS != result)
    {
        gos::logger::err ("glfwCreateWindowSurface() returned %s\n", string_VkResult(result));
        return false;
    }

    //cerco un physical device che sia appropriato
    {
        gos::StringList vkDevice_requiredExtensionList(scrapAllocator);
        vkDevice_requiredExtensionList.add (VK_KHR_SWAPCHAIN_EXTENSION_NAME);
        vkDevice_requiredExtensionList.add (VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME);
        //vkDevice_requiredExtensionList.add (VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME);
        //vkDevice_requiredExtensionList.add (VK_KHR_MAINTENANCE1_EXTENSION_NAME);
        //vkDevice_requiredExtensionList.add (VK_KHR_MAINTENANCE3_EXTENSION_NAME);

        sPhyDeviceInfo vkPhysicalDevInfo;
        if (!vulkanScanAndSelectAPhysicalDevices(vkInstance, vkSurface, vkDevice_requiredExtensionList, vulkanVersion, &vkPhysicalDevInfo))
        {
            gos::logger::err ("\ncan't find a good enough vulkan device\n");
            return false;
        }
        else
        {
            gos::logger::log (eTextColor::green, "\nselected device is at index %d\n   gfxQ familyIndex=%d, count=%d\n   computeQ familyIndex=%d, count=%d\n   transferQ familyIndex=%d, count=%d\n",
                vkPhysicalDevInfo.devIndex,
                vkPhysicalDevInfo.queue_gfx.familyIndex, vkPhysicalDevInfo.queue_gfx.count,
                vkPhysicalDevInfo.queue_compute.familyIndex, vkPhysicalDevInfo.queue_compute.count,
                vkPhysicalDevInfo.queue_transfer.familyIndex, vkPhysicalDevInfo.queue_compute.count);
        }
        gos::logger::log("\n");


        //creazione del device logico di vulkan
        if (!vulkanCreateDevice (vkPhysicalDevInfo, vkDevice_requiredExtensionList, vulkanVersion, &vulkan))
        {
            gos::logger::err ("can't create a logical device\n");
            return false;
        }
        gos::logger::log("\n");
    }

    vkCmdPushDescriptorSetKHR = (PFN_vkCmdPushDescriptorSetKHR)vkGetDeviceProcAddr(vulkan.dev, "vkCmdPushDescriptorSetKHR");
    if (!vkCmdPushDescriptorSetKHR) 
    {
        gos::logger::err ("Could not get a valid function pointer for vkCmdPushDescriptorSetKHR\n");
        return false;
    }




    //initVulkan:: creazione swap chain
    if (!vulkanCreateSwapChain (vulkan, vkSurface, vSync, &vulkan.swapChainInfo))
    {
        gos::logger::err ("can't create swap chain\n");
        return false;
    }    
    gos::logger::log("\n");

    //tutto ok
    gos::logger::log("\n");


    helperImmediateTransferCmd.setup (&vulkan, eGPUQueueType::transfer);
    return true;
}

//**********************************************************
void  GPU::priv_deinitVulkan()
{
    gos::logger::log ("GPU::priv_deinitVulkan()\n");
    if (VK_NULL_HANDLE != vkInstance)
    {
        if (VK_NULL_HANDLE != vulkan.dev)
            vulkan.destroy();

        if (VK_NULL_HANDLE != vkDebugMessenger)
        {
            auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(vkInstance, "vkDestroyDebugUtilsMessengerEXT");
            if (NULL != func)
                func(vkInstance, vkDebugMessenger, NULL);
        }

        if (VK_NULL_HANDLE != vkSurface)
            vkDestroySurfaceKHR(vkInstance, vkSurface, nullptr);

        vkDestroyInstance(vkInstance, NULL);
        vkInstance = VK_NULL_HANDLE;
    }
}    

//**********************************************************
bool GPU::priv_initHandleLists()
{
    gos::logger::log ("GPU::priv_initHandleLists()\n");
    shaderList.setup (allocator);
    vtxDeclList.setup (allocator);

    viewportlList.setup (allocator);
    viewportHandleList.setup (allocator, 32);   //questa mi serve per tenere traccia di tutti gli handle creati dato che durante il resize della window, devo andare ad aggiustare tutte le viewport

    depthStencilList.setup (allocator);
    depthStencilHandleList.setup (allocator, 32);   //questa mi serve per tenere traccia di tutti gli handle creati dato che durante il resize della window, devo andare ad aggiustare tutte i swpth buffer (nel caso che siano bindati alla dimensione della vport)

    renderTargetList.setup (allocator);
    renderTargetHandleList.setup (allocator, 64);   //questa mi serve per tenere traccia di tutti gli handle creati dato che durante il resize della window, devo andare ad aggiustare tutte i rt buffer (nel caso che siano bindati alla dimensione della vport)

    renderLayoutList.setup (allocator);
    pipelineList.setup (allocator);
    frameBufferList.setup (allocator);
    vtxBufferList.setup (allocator);
    staginBufferList.setup (allocator);
    idxBufferList.setup (allocator);
    descrSetLayoutList.setup (allocator);
    uniformBufferList.setup (allocator);
    storageBufferList.setup (allocator);
    descrPoolList.setup (allocator);
    descrSetInstanceList.setup (allocator);
    cmdBufferList.setup (allocator);
    textureList.setup (allocator);
    samplerList.setup (allocator);
    samplerDescrHashMap.setup (allocator, 128);
    return true;
}

//**********************************************************
void  GPU::priv_deinitandleLists()
{
    gos::logger::log ("GPU::priv_deinitandleLists()\n");
    
    shaderList.unsetup();
    vtxDeclList.unsetup();

    viewportlList.unsetup();
    viewportHandleList.unsetup();

    depthStencilList.unsetup();
    depthStencilHandleList.unsetup();

    renderTargetList.unsetup ();
    renderTargetHandleList.unsetup();

    renderLayoutList.unsetup ();
    pipelineList.unsetup();
    frameBufferList.unsetup();
    vtxBufferList.unsetup();
    staginBufferList.unsetup();
    idxBufferList.unsetup();
    descrSetLayoutList.unsetup();
    uniformBufferList.unsetup();
    storageBufferList.unsetup();
    descrPoolList.unsetup();
    descrSetInstanceList.unsetup();
    cmdBufferList.unsetup();
    textureList.unsetup();
    samplerList.unsetup();
    samplerDescrHashMap.unsetup();
}

//************************************
void GPU::priv_vulkanAddDebugCallback()
{
    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = GOS_vulkanDebugCallback;
    createInfo.pUserData = nullptr; // Optional

    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(vkInstance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr)
        func(vkInstance, &createInfo, NULL, &vkDebugMessenger);
}

//**********************************************************
bool GPU::semaphore_create  (VkSemaphore *out)
{
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;    

    const VkResult result = vkCreateSemaphore (vulkan.dev, &semaphoreInfo, nullptr, out);
    if (VK_SUCCESS == result)
        return true;
    gos::logger::err ("vulkanCreateSemaphore() => %s\n", string_VkResult(result));
    return false;
}

//************************************
void GPU::semaphore_destroy  (VkSemaphore &in)
{
    if (VK_NULL_HANDLE != vulkan.dev && VK_NULL_HANDLE != in)
    {
        vkDestroySemaphore (vulkan.dev, in, nullptr);
        in = VK_NULL_HANDLE;
    }
}

//**********************************************************
bool GPU::fence_create  (bool bStartAsSignaled, VkFence *out)
{
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    if (bStartAsSignaled)
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    const VkResult result = vkCreateFence (vulkan.dev, &fenceInfo, nullptr, out);
    if (VK_SUCCESS == result)
        return true;
    gos::logger::err ("vkCreateFence() => %s\n", string_VkResult(result));
    return false;
}

//************************************
void GPU::fence_destroy  (VkFence &in)
{
    if (VK_NULL_HANDLE != vulkan.dev && VK_NULL_HANDLE != in)
    {
        vkDestroyFence (vulkan.dev, in, nullptr);
        in = VK_NULL_HANDLE;
    }
}

//************************************
bool GPU::fence_wait (const VkFence &fenceHandle, u64 timeout_ns)
{
    const VkResult result = vkWaitForFences (vulkan.dev, 1, &fenceHandle, VK_TRUE, timeout_ns);
    if (VK_SUCCESS == result)
        return true;
    return false;
}

//************************************
bool GPU::fence_waitMany (const VkFence *fenceHandleList, bool bWaitForAll, u32 fenceCount, u64 timeout_ns)
{
    VkBool32 vkb = VK_FALSE;
    if (bWaitForAll)
        vkb = VK_TRUE;
    const VkResult result = vkWaitForFences (vulkan.dev, fenceCount, fenceHandleList, vkb, timeout_ns);
    if (VK_SUCCESS == result)
        return true;
    return false;
}

//************************************
bool GPU::fence_isSignaled  (const VkFence &fenceHandle)
{
    const VkResult result = vkGetFenceStatus (vulkan.dev, fenceHandle);
    if (VK_SUCCESS == result)
        return true;
    return false;
}

//************************************
void GPU::fence_reset (const VkFence &fenceHandle)
{
    vkResetFences (vulkan.dev, 1, &fenceHandle);
}

//************************************
void GPU::fence_resetMany (const VkFence *fenceHandleList, u32 fenceCount)
{
    vkResetFences (vulkan.dev, fenceCount, fenceHandleList);
}

//************************************
gos::eImageFormat GPU::swapChain_getImageFormat() const
{ 
    return gpu::fromVulkan(vulkan.swapChainInfo.imageFormat); 
}

//************************************
bool GPU::swapChain_acquireImage (u64 timeout_ns, VkSemaphore semaphore, VkFence fence)
{
    bSwapChainRecreatedDuringThisFrame = false;

    const u64 timeNow_msec = gos::getTimeSinceStart_msec();
    toBeDeletedBuilder.check (timeNow_msec);

    if (bRecreateSwapChainOnNextFrame)
    {
        bRecreateSwapChainOnNextFrame = false;
        priv_swapChain_recreate();
    }


    const VkResult result = vkAcquireNextImageKHR (vulkan.dev, vulkan.swapChainInfo.vkSwapChain, timeout_ns, semaphore, fence, &currentSwapChainImageIndex);

    switch (result)
    {
    default:
        gos::logger::err ("GPU::beginFrame() => vkAcquireNextImageKHR() => %s\n", string_VkResult(result));
        return false;

    case VK_SUCCESS:
        return true;

    case VK_SUBOPTIMAL_KHR:
        //posso ancora renderizzare, ma al prossimo newFrame la swapchain verra' ricreata
        bRecreateSwapChainOnNextFrame = true;
        return true;

    case VK_ERROR_OUT_OF_DATE_KHR:
        priv_swapChain_recreate();
        return false;

    case VK_TIMEOUT:
        return false;

    case VK_NOT_READY:
        return false;
    }
}

//************************************
VkResult GPU::swapChain_present (const VkSemaphore *semaphoreHandleList, u32 semaphoreCount)
{
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = semaphoreCount;
    presentInfo.pWaitSemaphores = semaphoreHandleList; //prima di presentare, aspetta che GPU segnali tutti i semafori di [semaphoreHandleList]
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &vulkan.swapChainInfo.vkSwapChain;
    presentInfo.pImageIndices = &currentSwapChainImageIndex;
    
    return vkQueuePresentKHR (vulkan.getQueueInfo(eGPUQueueType::gfx)->vkQueueHandle, &presentInfo);
}

//**********************************************************
bool GPU::priv_swapChain_recreate ()
{
    bSwapChainRecreatedDuringThisFrame = true;
    gos::logger::log (eTextColor::green, "GPU::swapChain_recreate()\n");
    gos::logger::incIndent();

    int width = 0;
    int height = 0;
    while (width == 0 || height == 0) 
    {
        gos::logger::log ("windows size is weird (w=%d, h=%d), waiting...\n", width, height);

        GLFWwindow *glfWin = window.getGLF();
        glfwGetFramebufferSize (glfWin, &width, &height);
        glfwWaitEvents();
    }

    //attendo che Vulkan sia in idle
    bool ret = true;
    vkDeviceWaitIdle (vulkan.dev);

    //distruggo la swapchain
    vulkan.swapChainInfo.destroy(vulkan.dev);

    //ricreazione swap chain
    if (!vulkanCreateSwapChain (vulkan, this->vkSurface, vSync, &vulkan.swapChainInfo))
    {
        gos::logger::err ("can't create swap chain\n");
        ret = false;
    }
    
    //attuale dimensione della vport
    const i16 vportW = (i16)vulkan.swapChainInfo.imageExtent.width;
    const i16 vportH = (i16)vulkan.swapChainInfo.imageExtent.height;
    //aggiorno le info del default RT
    {
        gpu::RenderTarget *rt;
        priv_fromHandleToPointer (renderTargetList, defaultRTHandle, &rt);
        rt->format = vulkan.swapChainInfo.imageFormat;
        rt->width = vulkan.swapChainInfo.imageExtent.width;
        rt->height = vulkan.swapChainInfo.imageExtent.height;
    }

    //aggiorno tutte le viewport che hanno una dimensione/posizione relativa
    u32 n = viewportHandleList.getNElem();
    for (u32 i=0; i<n; i++)
    {
        gos::gpu::Viewport *v;
        if (viewportlList.fromHandleToPointer (viewportHandleList(i), &v))
            v->resolve (vportW, vportH);
    }

    //aggiorno tutti i RT che hanno una dimensione/posizione relativa
    n = renderTargetHandleList.getNElem();
    for (u32 i=0; i<n; i++)
    {
        gos::gpu::RenderTarget *s;
        if (renderTargetList.fromHandleToPointer (renderTargetHandleList(i), &s))
        {
            if (s->width.isRelative() || s->height.isRelative())
            {
                priv_renderTarget_deleteFromStruct (*s);
                s->resolve (vportW, vportH);
                priv_renderTarget_createFromStruct (*s);
            }
        }
    }

    //aggiorno tutti i depth buffer che hanno una dimensione/posizione relativa
    n = depthStencilHandleList.getNElem();
    for (u32 i=0; i<n; i++)
    {
        gos::gpu::DepthStencil *s;
        if (depthStencilList.fromHandleToPointer (depthStencilHandleList(i), &s))
        {
            if (s->width.isRelative() || s->height.isRelative())
            {
                priv_depthStencil_deleteFromStruct (*s);
                s->resolve (vportW, vportH);
                priv_depthStencil_createFromStruct (*s);
            }
        }
    }

    //ricreo tutti i FrameBuffer che sono dipendenti dalla swapchain
    n = frameBufferDependentOnSwapChainList.getNElem();
    for (u32 i=0; i<n; i++)
    {
        GPUFrameBufferHandle handle = frameBufferDependentOnSwapChainList(i);

        gpu::FrameBuffer *sFB;
        if (priv_fromHandleToPointer (frameBufferList, handle, &sFB))
        {
            gos::logger::verbose ("recreating FrameBuffer handle=0x%08X\n", handle.viewAsU32());
            priv_frameBuffer_deleteFromStruct (sFB);
            priv_frameBuffer_recreate (sFB);
        }
    }

    //fine
    gos::logger::decIndent();
    return ret;  
}

//**********************************************************
void  GPU::waitIdle()
{
    vkDeviceWaitIdle(vulkan.dev);
}

//**********************************************************
void  GPU::waitIdle (eGPUQueueType q)
{
    vkQueueWaitIdle (vulkan.getQueueInfo(q)->vkQueueHandle);
}

//************************************
void  GPU::toggleFullscreen()
{
    gos::logger::log (eTextColor::yellow, "toggleFullscreen\n");
    gos::logger::incIndent();

    GLFWwindow *glfWin = window.getGLF();
    GLFWmonitor *monitor = glfwGetWindowMonitor(glfWin);
    if (NULL == monitor)
    {
        //andiamo in full
        window.storeCurrentPosAndSize();
        gos::logger::log ("going full screen, current win pos and size (%d,%d) (%d,%d)\n", window.storedX, window.storedY, window.storedW, window.storedH);

        monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode *mode = glfwGetVideoMode(monitor);
        glfwSetWindowMonitor (glfWin, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
    }    
    else
    {
        //torniamo in windowed
        gos::logger::log ("going in windowed mode, current win pos and size (%d,%d) (%d,%d)\n", window.storedX, window.storedY, window.storedW, window.storedH);
        glfwSetWindowMonitor(glfWin, NULL, window.storedX, window.storedY, window.storedW, window.storedH, 0);
    }

    gos::logger::decIndent();

    bRecreateSwapChainOnNextFrame = true;
}

//************************************
void GPU::vsync_enable (bool b)
{
    if (vSync == b)
        return;
    vSync = b;
    bRecreateSwapChainOnNextFrame = true;
}


//************************************
void GPU::priv_createHelperStagingBuffer (u32 size)
{
    if (helperStagingBuffer.isValid())
    {
        DBGBREAK;
        return;
    }

    stagingBuffer_create (size, &helperStagingBuffer);

}




/************************************************************************************************************
 * Shader
 * 
 * 
 *************************************************************************************************************/
bool GPU::priv_shader_createFromFile (const char *filename, eShaderType shaderType, const char *mainFnName, GPUShaderHandle *out_shaderHandle)
{
    assert (NULL != out_shaderHandle);
    
    gos::Allocator *scrapAllocator = gos::getScrapAllocator();

    u32 bufferSize;
    u8 *buffer = gos::fs::fileLoadInMemory (scrapAllocator, filename, &bufferSize);
    if (NULL == buffer)
    {
        out_shaderHandle->setInvalid();
        gos::logger::err ("GPU::priv_shader_createFromFile() => can't load shader %s\n", filename);
        return false;
    }    
    
    const bool ret = priv_shader_createFromMemory (buffer, bufferSize, shaderType, mainFnName, out_shaderHandle);
    GOSFREE(scrapAllocator, buffer);
    return ret;
}

//**********************************************************
bool GPU::priv_shader_createFromMemory (const u8 *buffer, u32 bufferSize, eShaderType shaderType, const char *mainFnName, GPUShaderHandle *out_shaderHandle)
{
    assert (NULL != out_shaderHandle);

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = bufferSize;
    createInfo.pCode = reinterpret_cast<const uint32_t*>(buffer);
    
    VkShaderModule vkHandle;
    const VkResult result = vkCreateShaderModule(vulkan.dev, &createInfo, nullptr, &vkHandle);
    if (VK_SUCCESS != result)
    {
        out_shaderHandle->setInvalid();
        gos::logger::err ("GPU::priv_shader_createFromMemory() => %s\n", string_VkResult(result));
        return false;
    }

    gpu::Shader *shader = shaderList.reserve(out_shaderHandle);
    if (NULL == shader)
    {
        vkDestroyShaderModule (vulkan.dev, vkHandle, nullptr);
        out_shaderHandle->setInvalid();
        gos::logger::err ("GPU::priv_shader_createFromMemory() => unable to reserve a new shader handle\n");
        return false;
    }

    shader->reset();
    shader->_vkHandle = vkHandle;
    shader->_shaderType = shaderType;
    sprintf_s (shader->_mainFnName, sizeof(shader->_mainFnName), "%s", mainFnName);
    return true;
}

//************************************
void GPU::deleteResource (GPUShaderHandle &shaderHandle)
{
    gpu::Shader *shader;
    if (priv_fromHandleToPointer(shaderList, shaderHandle, &shader))
    {
        if (VK_NULL_HANDLE != shader->_vkHandle)
            vkDestroyShaderModule (vulkan.dev, shader->_vkHandle, nullptr);
        
        shader->reset();
        shaderList.release(shaderHandle);
    }
    shaderHandle.setInvalid();
}

//************************************
VkShaderModule GPU::shader_getVkHandle (const GPUShaderHandle shaderHandle) const
{
    gpu::Shader *shader;
    if (priv_fromHandleToPointer(shaderList, shaderHandle, &shader))
        return shader->_vkHandle;
    return VK_NULL_HANDLE;
}

//************************************
const char* GPU::shader_getMainFnName (const GPUShaderHandle shaderHandle) const
{
    gpu::Shader *shader;
    if (priv_fromHandleToPointer(shaderList, shaderHandle, &shader))
        return shader->_mainFnName;
    return NULL;
}

//************************************
eShaderType GPU::shader_getType (const GPUShaderHandle shaderHandle) const
{
    gpu::Shader *shader;
    if (priv_fromHandleToPointer(shaderList, shaderHandle, &shader))
        return shader->_shaderType;
    return eShaderType::unknown;
}






/************************************************************************************************************
 * VtxDecl
 * 
 * 
 *************************************************************************************************************/
GPU::VtxDeclBuilder& GPU::vtxDecl_createNew (GPUVtxDeclHandle *out_handle)
{
    out_handle->setInvalid();
    vtxDeclBuilder.priv_begin(this, out_handle);
    return vtxDeclBuilder;
}

//************************************
void GPU::deleteResource (GPUVtxDeclHandle &handle)
{
    gpu::VtxDecl *s;
    if (priv_fromHandleToPointer(vtxDeclList, handle, &s))
    {
        s->reset();
        vtxDeclList.release(handle);
    }
    handle.setInvalid();
}

//************************************
void GPU::priv_vxtDecl_onBuilderEnds (VtxDeclBuilder *builder)
{
    builder->handle->setInvalid();

    if (!builder->priv_isValid())
        return;

    //creo un nuovo handle per la vtxDecl
    gpu::VtxDecl *vtxDecl = vtxDeclList.reserve (builder->handle);
    if (NULL == vtxDecl)
    {
        gos::logger::err ("GPU::priv_vxtDecl_onBuilderEnds() => unable to reserve a new handle\n");
        return;
    }

    //fillo vtxDecl con i dati raccolti dal builder
    vtxDecl->reset();
    vtxDecl->stream_setNum (builder->numStreamIndex);
    for (u8 i=0; i<builder->numStreamIndex; i++)
        vtxDecl->stream_setInputRate (i, builder->inputRatePerStream[i]);

    vtxDecl->attr_setNum (builder->numAttributeDesc);
    for (u8 i=0; i<builder->numAttributeDesc; i++)
    {
        vtxDecl->attr_setStreamIndex (i, builder->attributeDesc[i].streamIndex);
        vtxDecl->attr_setBindingLocation (i, builder->attributeDesc[i].bindingLocation);
        vtxDecl->attr_setDataFormat (i, builder->attributeDesc[i].format);
        vtxDecl->attr_setOffset (i, builder->attributeDesc[i].offset);
    }
}

//************************************
bool GPU::vtxDecl_query (const GPUVtxDeclHandle handle, gpu::VtxDecl *out) const
{
    assert (out);
    gpu::VtxDecl *p;
    if (priv_fromHandleToPointer(vtxDeclList, handle, &p))
    {
        //ritorno una copia dell'oggetto, non il pt all'oggetto
        (*out) = (*p);
        return true;
    }

    out->reset();
    return false;
}





/************************************************************************************************************
 * viewport
 * 
 * 
 *************************************************************************************************************/
bool GPU::viewport_create (const gos::Pos2D &x,const gos::Pos2D &y, const gos::Dim2D &w, const gos::Dim2D &h, GPUViewportHandle *out_handle)
{
    assert (NULL != out_handle);
    gpu::Viewport *v = viewportlList.reserve (out_handle);
    if (NULL == v)
    {
        gos::logger::err ("GPU::viewport_create() => can't reserve a new vport!\n");
        out_handle->setInvalid();
        return false;
    }

    viewportHandleList.append (*out_handle);

    //imposto la viewport
    v->x = x;
    v->y = y;
    v->width = w;
    v->height = h;

    int width, height;
    window.getCurrentSize (&width, &height);
    v->resolve ((i16)width, (i16)height);
    return true;
}

//************************************
void GPU::deleteResource (GPUViewportHandle &handle)
{
    gos::gpu::Viewport *v;
    if (viewportlList.fromHandleToPointer (handle, &v))
    {
        viewportlList.release(handle);
        viewportHandleList.findAndRemove(handle);
    }
    handle.setInvalid();
}

//************************************
const gpu::Viewport* GPU::viewport_get (const GPUViewportHandle &handle) const
{
    gos::gpu::Viewport *v;
    if (!viewportlList.fromHandleToPointer (handle, &v))
        return NULL;
    return v;
}


/************************************************************************************************************
 * Render target
 * 
 * 
 *************************************************************************************************************/
bool GPU::renderTarget_create (const gos::Dim2D &dimx, const gos::Dim2D &dimy, eImageFormat fmt, GPURenderTargetHandle *out_handle)
{
    assert (NULL != out_handle);
    out_handle->setInvalid();


    //riservo un handle
    gos::gpu::RenderTarget *rt = renderTargetList.reserve (out_handle);
    if (NULL == rt)
    {
        gos::logger::err ("GPU::renderTarget_create() => can't reserve a new depthStencil handle!\n");
        return false;
    }
    rt->reset();
    rt->format = gos::gpu::toVulkan(fmt);
    rt->width = dimx;
    rt->height = dimy;
    rt->usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |   //color buffer
                VK_IMAGE_USAGE_TRANSFER_SRC_BIT |      //la posso usare come src di un transfer
                VK_IMAGE_USAGE_TRANSFER_DST_BIT |      //la posso usare come dst di un transfer
                VK_IMAGE_USAGE_STORAGE_BIT;            //la posso usare come dst di un compute shader


    //alloco le risorse vulkan
    if (!priv_renderTarget_createFromStruct (*rt))
        return false;

    renderTargetHandleList.append (*out_handle);
    return true;
}

//************************************
void GPU::deleteResource (GPURenderTargetHandle &handle)
{
    gos::gpu::RenderTarget *s;
    if (renderTargetList.fromHandleToPointer (handle, &s))
    {
        priv_renderTarget_deleteFromStruct (*s);
        s->reset();
        renderTargetList.release (handle);
        renderTargetHandleList.findAndRemove (handle);
    }

    handle.setInvalid();
}

//************************************
bool GPU::priv_renderTarget_createFromStruct (gos::gpu::RenderTarget &rt)
{
    assert (VK_NULL_HANDLE == rt.image);
    assert (VK_NULL_HANDLE == rt.vkMemHandle);
    assert (VK_NULL_HANDLE == rt.view);
    assert (VK_FORMAT_UNDEFINED != rt.format);

    //risolvo la dimensione
    rt.resolve ((i16)vulkan.swapChainInfo.imageExtent.width, (i16)vulkan.swapChainInfo.imageExtent.height);


	VkImageCreateInfo imageCI{};
	imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCI.imageType = VK_IMAGE_TYPE_2D;
	imageCI.format = rt.format;
	imageCI.extent = { rt.resolvedW, rt.resolvedH, 1 };
	imageCI.mipLevels = 1;
	imageCI.arrayLayers = 1;
	imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCI.usage = rt.usage;

	VkResult result = vkCreateImage (vulkan.dev, &imageCI, nullptr, &rt.image);    
    if (VK_SUCCESS != result)
    {
        gos::logger::err ("GPU::priv_renderTarget_createFromStruct() => vkCreateImage() => %s\n", string_VkResult(result));
        return false;
    }

    VkMemoryRequirements memReqs{};
	vkGetImageMemoryRequirements (vulkan.dev, rt.image, &memReqs);
    rt.memoryAllocated = memReqs.size;

    VkMemoryAllocateInfo memAllloc{};
	memAllloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memAllloc.allocationSize = memReqs.size;
    vulkanGetMemoryType (vulkan.phyDevInfo, memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &memAllloc.memoryTypeIndex);

    if (!vulkanAllocMemory (vulkan, &memAllloc, nullptr, &rt.vkMemHandle))
    {
        gos::logger::err ("GPU::priv_renderTarget_createFromStruct() => error allocating memory\n");
        return false;
    }

	result = vkBindImageMemory (vulkan.dev, rt.image, rt.vkMemHandle, 0);
    if (VK_SUCCESS != result)
    {
        gos::logger::err ("GPU::priv_renderTarget_createFromStruct() => vkBindImageMemory() => %s\n", string_VkResult(result));
        return false;
    }

    VkImageViewCreateInfo imageViewCI{};
	imageViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
	imageViewCI.image = rt.image;
	imageViewCI.format = rt.format;
    imageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageViewCI.subresourceRange.baseMipLevel = 0;
	imageViewCI.subresourceRange.levelCount = 1;
	imageViewCI.subresourceRange.baseArrayLayer = 0;
    imageViewCI.subresourceRange.layerCount = 1;

	result = vkCreateImageView (vulkan.dev, &imageViewCI, nullptr, &rt.view);
    if (VK_SUCCESS != result)
    {
        gos::logger::err ("GPU::priv_depthStenicl_createFromStruct() => vkCreateImageView() => %s\n", string_VkResult(result));
        return false;
    }

    return true;
}        
        
//************************************
void GPU::priv_renderTarget_deleteFromStruct (gos::gpu::RenderTarget &rt)
{
    if (VK_NULL_HANDLE != rt.view)
    {
	    vkDestroyImageView (vulkan.dev, rt.view, nullptr);
        rt.view = VK_NULL_HANDLE;
    }
    
    if (VK_NULL_HANDLE != rt.image)
    {
	    vkDestroyImage(vulkan.dev, rt.image, nullptr);
        rt.image = VK_NULL_HANDLE;
    }

    if (VK_NULL_HANDLE != rt.vkMemHandle)
    {
	    vulkanFreeMemory (vulkan, rt.vkMemHandle, nullptr, rt.memoryAllocated);
        rt.vkMemHandle = VK_NULL_HANDLE;
    }
}

//************************************
const gpu::RenderTarget* GPU::getInfo (const GPURenderTargetHandle handle) const
{
    gpu::RenderTarget *s;
    if (priv_fromHandleToPointer (renderTargetList, handle, &s))
        return s;
    return NULL;
}

/************************************************************************************************************
 * DepthStencil
 * 
 * 
 *************************************************************************************************************/
bool GPU::depthStencil_create (const gos::eImageFormat fmt, const gos::Dim2D &widthIN, const gos::Dim2D &heightIN, bool bWithStencil, GPUDepthStencilHandle *out_handle)
{
    assert (NULL != out_handle);

    if (!image::isFormatWithDepth(fmt))
    {
        gos::logger::err ("GPU::depthStencil_create() => invalid depth format (%s). Must be a valid 'DEPTH_something'\n", gos::enumToString(fmt));
        return false;
    }

    if (bWithStencil)
    {
        if (!image::isFormatWithStencil(fmt))
        {
            gos::logger::err ("GPU::depthStencil_create() => invalid depth format (%s). Format must include a STENCIL option\n", gos::enumToString(fmt));
            return false;
        }
    }
    else
    {
        if (image::isFormatWithStencil(fmt))
        {
            gos::logger::err ("GPU::depthStencil_create() => invalid depth format (%s). Format must NOT include a STENCIL option\n", gos::enumToString(fmt));
            return false;
        }
    }

    //riservo un handle
    gos::gpu::DepthStencil *depthStencil = depthStencilList.reserve (out_handle);
    if (NULL == depthStencil)
    {
        gos::logger::err ("GPU::depthStencil_create() => can't reserve a new depthStencil handle!\n");
        out_handle->setInvalid();
        return false;
    }
    depthStencil->reset();
    depthStencil->depthFormat = gpu::toVulkan(fmt);
    depthStencil->bHasStencil = bWithStencil;

    //assegno width/height
    depthStencil->width = widthIN;
    depthStencil->height = heightIN;

    //alloco le risorse vulkan
    if (!priv_depthStencil_createFromStruct (*depthStencil))
        return false;

    depthStencilHandleList.append (*out_handle);
    return true;
}

//************************************
void GPU::deleteResource (GPUDepthStencilHandle &handle)
{
    gos::gpu::DepthStencil *s;
    if (depthStencilList.fromHandleToPointer (handle, &s))
    {
        priv_depthStencil_deleteFromStruct (*s);
        s->reset();
        depthStencilList.release (handle);
        depthStencilHandleList.findAndRemove (handle);
    }

    handle.setInvalid();
}

//************************************
bool GPU::priv_depthStencil_createFromStruct (gos::gpu::DepthStencil &depthStencil)
{
    assert (VK_NULL_HANDLE == depthStencil.image);
    assert (VK_NULL_HANDLE == depthStencil.vkMemHandle);
    assert (VK_NULL_HANDLE == depthStencil.view);
    assert (VK_FORMAT_UNDEFINED != depthStencil.depthFormat);

    //risolvo la dimensione
    depthStencil.resolve ((i16)vulkan.swapChainInfo.imageExtent.width, (i16)vulkan.swapChainInfo.imageExtent.height);


	VkImageCreateInfo imageCI{};
	imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCI.imageType = VK_IMAGE_TYPE_2D;
	imageCI.format = depthStencil.depthFormat;
	imageCI.extent = { depthStencil.resolvedW, depthStencil.resolvedH, 1 };
	imageCI.mipLevels = 1;
	imageCI.arrayLayers = 1;
	imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCI.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

	VkResult result = vkCreateImage (vulkan.dev, &imageCI, nullptr, &depthStencil.image);    
    if (VK_SUCCESS != result)
    {
        gos::logger::err ("GPU::priv_depthStenicl_createFromStruct() => vkCreateImage() => %s\n", string_VkResult(result));
        return false;
    }

    VkMemoryRequirements memReqs{};
	vkGetImageMemoryRequirements (vulkan.dev, depthStencil.image, &memReqs);
    depthStencil.memoryAllocated = memReqs.size;

    VkMemoryAllocateInfo memAllloc{};
	memAllloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memAllloc.allocationSize = memReqs.size;
    vulkanGetMemoryType (vulkan.phyDevInfo, memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &memAllloc.memoryTypeIndex);

    if (!vulkanAllocMemory (vulkan, &memAllloc, nullptr, &depthStencil.vkMemHandle))
    {
        gos::logger::err ("GPU::priv_depthStenicl_createFromStruct() => error allocating memory\n");
        return false;
    }

	result = vkBindImageMemory (vulkan.dev, depthStencil.image, depthStencil.vkMemHandle, 0);
    if (VK_SUCCESS != result)
    {
        gos::logger::err ("GPU::priv_depthStenicl_createFromStruct() => vkBindImageMemory() => %s\n", string_VkResult(result));
        return false;
    }

    VkImageViewCreateInfo imageViewCI{};
	imageViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
	imageViewCI.image = depthStencil.image;
	imageViewCI.format = depthStencil.depthFormat;
	imageViewCI.subresourceRange.baseMipLevel = 0;
	imageViewCI.subresourceRange.levelCount = 1;
	imageViewCI.subresourceRange.baseArrayLayer = 0;
	imageViewCI.subresourceRange.layerCount = 1;
	imageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	if (depthStencil.bHasStencil)
		imageViewCI.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;

	result = vkCreateImageView (vulkan.dev, &imageViewCI, nullptr, &depthStencil.view);
    if (VK_SUCCESS != result)
    {
        gos::logger::err ("GPU::priv_depthStenicl_createFromStruct() => vkCreateImageView() => %s\n", string_VkResult(result));
        return false;
    }

    return true;
}

//************************************
void GPU::priv_depthStencil_deleteFromStruct (gos::gpu::DepthStencil &depthStencil)
{
    if (VK_NULL_HANDLE != depthStencil.view)
    {
	    vkDestroyImageView (vulkan.dev, depthStencil.view, nullptr);
        depthStencil.view = VK_NULL_HANDLE;
    }
    
    if (VK_NULL_HANDLE != depthStencil.image)
    {
	    vkDestroyImage(vulkan.dev, depthStencil.image, nullptr);
        depthStencil.image = VK_NULL_HANDLE;
    }

    if (VK_NULL_HANDLE != depthStencil.vkMemHandle)
    {
	    vulkanFreeMemory (vulkan, depthStencil.vkMemHandle, nullptr, depthStencil.memoryAllocated);
        depthStencil.vkMemHandle = VK_NULL_HANDLE;
    }
}

//************************************
const gpu::DepthStencil* GPU::getInfo (const GPUDepthStencilHandle handle) const
{
    gpu::DepthStencil *s;
    if (priv_fromHandleToPointer (depthStencilList, handle, &s))
        return s;
    return NULL;
}


/************************************************************************************************************
 * FrameBuffer
 * 
 * 
 *************************************************************************************************************/
GPU::FrameBuffersBuilder& GPU::frameBuffer_createNew (const GPURenderLayoutHandle &renderLayoutHandle, GPUFrameBufferHandle *out_handle)
{
    assert (NULL != out_handle);
    out_handle->setInvalid();

    FrameBuffersBuilder *builder = GOSNEW(gos::getScrapAllocator(), GPU::FrameBuffersBuilder) (this, renderLayoutHandle, out_handle);
    return *builder;
}

//************************************
bool GPU::priv_frameBuffer_onBuilderEnds (FrameBuffersBuilder *builder)
{
    //aggiungo il builder alla lista dei builder da deletare
    toBeDeletedBuilder.add(builder);

    if (builder->anyError())
        return false;
        

    GPUFrameBufferHandle handle;
    gpu::FrameBuffer *s = frameBufferList.reserve (&handle);
    if (NULL == s)
    {
        gos::logger::err ("GPU::priv_frameBuffer_onBuilderEnds() => can't reserve a handle!\n");
        return false;
    }


    //Fillo la struttura con i dati di creazione recuperati dal builder
    s->reset();
    s->width = builder->width;
    s->height = builder->height;

    
    //render layout. Mi accerto che sia valido
    gpu::RenderLayout *sRL;
    if (!priv_fromHandleToPointer (renderLayoutList, builder->renderLayoutHandle, &sRL))
    {
        gos::logger::err ("GPU::priv_frameBuffer_onBuilderEnds() => invalid renderLayoutHandle\n");
        frameBufferList.release (handle);
        return false;
    }
    s->renderLayoutHandle = builder->renderLayoutHandle;
    

    //depthstencil. Se le sue dimensioni non sono assolute, allora vuol dire che dipendono dalla
    //dimensione della swapchain e quindi questo framBuffer devo marcalco come "bound to swapchain"
    //per poterlo ricreare in auto quando la swapchain cambia
    s->depthStencilHandle = builder->depthStencilHandle;
    if (s->depthStencilHandle.isValid())
    {
        gpu::DepthStencil *ds;
        if (!priv_fromHandleToPointer (depthStencilList, s->depthStencilHandle, &ds))
        {
            gos::logger::err ("GPU::priv_frameBuffer_onBuilderEnds() => invalid depthstencil handle\n");
            frameBufferList.release (handle);
            return false;
        }
        
        if (ds->width.isRelative() || ds->height.isRelative())
            s->boundToSwapChain = true;
    }

    //render target
    s->numRenderTaget = builder->numRenderTarget;
    for (u32 i=0;i<builder->numRenderTarget;i++)
    {
        const GPURenderTargetHandle rt = builder->renderTargetHandleList[i];
        s->renderTargetHandleList[i] = rt;

        if (rt == defaultRTHandle)
        {
            //ci stiamo bindando al default RT
            s->boundToSwapChain = true;
            s->boundToMainRT = true;
        }
        else
        {
            //come per il deptStencil, devo verificare se il RT e' a dimensioni assolute o relative
            gpu::RenderTarget *sRT;
            if (!priv_fromHandleToPointer (renderTargetList, rt, &sRT))
            {
                gos::logger::err ("GPU::priv_frameBuffer_onBuilderEnds() => invalid render target handle at index %d\n", i);
                frameBufferList.release (handle);
                return false;
            }

            if (sRT->width.isRelative() || sRT->height.isRelative())
                s->boundToSwapChain = true;
        }
    }


    //tutto ok
    *builder->out_handle = handle;
    if (s->boundToSwapChain)
        frameBufferDependentOnSwapChainList.append(handle);
    priv_frameBuffer_recreate (s);


    gos::logger::verbose ("created FrameBuffer handle=0x%08X\n", handle.viewAsU32());
    return true;
}

//************************************
void GPU::deleteResource (GPUFrameBufferHandle &handle)
{
    gpu::FrameBuffer *s;
    if (priv_fromHandleToPointer (frameBufferList, handle, &s))
    {
        if (s->boundToSwapChain)
            frameBufferDependentOnSwapChainList.findAndRemove(handle);
        priv_frameBuffer_deleteFromStruct (s);
        s->reset();
        frameBufferList.release (handle);
    }

    handle.setInvalid();
}

//************************************
void GPU::priv_frameBuffer_deleteFromStruct (gpu::FrameBuffer *s)
{
    if (s->boundToMainRT)
    {
        for (u32 i=0; i<s->numFrameBuffer; i++)
            vkDestroyFramebuffer (vulkan.dev, s->vkFrameBufferList[i], nullptr);
    }
    else
    {
        vkDestroyFramebuffer (vulkan.dev, s->vkFrameBufferList[0], nullptr);
    }
}

//************************************
bool GPU::priv_frameBuffer_recreate (gpu::FrameBuffer *s)
{
    //render area
    s->resolve ((i16)vulkan.swapChainInfo.imageExtent.width, (i16)vulkan.swapChainInfo.imageExtent.height);

    gos::logger::verbose ("GPU::priv_frameBuffer_recreate() => frame buffer size: %d %d\n", s->resolvedW, s->resolvedH);
    gos::logger::incIndent();
    const bool ret = priv_frameBuffer_do_recreate(s);
    gos::logger::decIndent();
    return ret;
}
bool GPU::priv_frameBuffer_do_recreate (gpu::FrameBuffer *s)
{
    //render layout
    gpu::RenderLayout *sRL;
    if (!priv_fromHandleToPointer (renderLayoutList, s->renderLayoutHandle, &sRL))
    {
        gos::logger::err ("invalid handler\n");
        return false;
    }

    //Se sono bindato al mainRT, devo creare N VulkanFrameBuffer, 1 per ogni immagine della swapchain
    s->numFrameBuffer = 1;
    if (s->boundToMainRT)
        s->numFrameBuffer = vulkan.swapChainInfo.imageCount;

    for (u32 t=0; t<s->numFrameBuffer; t++)
    {
        //render target
        VkImageView imageViewList[GOSGPU__NUM_MAX_ATTACHMENT + 1];
        u32 nViewList = 0;

        for (u32 i=0; i<s->numRenderTaget; i++)
        {
            if (s->renderTargetHandleList[i] == defaultRTHandle)
            {
                assert (s->boundToMainRT);
                imageViewList[nViewList++] = vulkan.swapChainInfo.vkImageListView[t];
            }
            else
            {
                gpu::RenderTarget *sRT;
                if (!priv_fromHandleToPointer (renderTargetList, s->renderTargetHandleList[i], &sRT))
                    return false;

                assert (NULL != sRT->view);
                imageViewList[nViewList++] = sRT->view;
            }
        }

        //depthStencil
        if (s->depthStencilHandle.isValid())
        {
            gpu::DepthStencil *zb;
            if (!priv_fromHandleToPointer (depthStencilList, s->depthStencilHandle, &zb))
                return false;
            gos::logger::log ("depth stencile size: %d %d\n", zb->resolvedW, zb->resolvedH);
            imageViewList[nViewList++] = zb->view;
        }

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = sRL->vkRenderPassHandle;
        framebufferInfo.attachmentCount = nViewList;
        framebufferInfo.pAttachments = imageViewList;
        framebufferInfo.width = s->resolvedW;
        framebufferInfo.height = s->resolvedH;
        framebufferInfo.layers = 1;

        const VkResult result = vkCreateFramebuffer(vulkan.dev, &framebufferInfo, nullptr, &s->vkFrameBufferList[t]);
        if (VK_SUCCESS != result)
        {
            gos::logger::err ("vkCreateFramebuffer => %s\n", string_VkResult(result));
            return false;
        }
    }

    return true;
}

//************************************
bool GPU::toVulkan (const GPUFrameBufferHandle handle, VkFramebuffer *out, u32 *out_renderAreaW, u32 *out_renderAreaH) const
{
    assert (NULL != out);
    assert (NULL != out_renderAreaW);
    assert (NULL != out_renderAreaH);

    gpu::FrameBuffer *s;
    if (!priv_fromHandleToPointer (frameBufferList, handle, &s))
    {
        *out = VK_NULL_HANDLE;
        gos::logger::err ("GPU::frameBuffer_toVulkan() => invalid handle\n");
        return false;        
    }

    *out_renderAreaW = s->resolvedW;
    *out_renderAreaH = s->resolvedH;

    if (s->boundToMainRT)
        *out = s->vkFrameBufferList[this->currentSwapChainImageIndex];
    else
        *out = s->vkFrameBufferList[0];

    return true;
}



/************************************************************************************************************
 * RenderTaskLayout
 * 
 * 
 *************************************************************************************************************/
GPU::RenderTaskLayoutBuilder& GPU::renderLayout_createNew (GPURenderLayoutHandle *out_handle)
{
    assert (NULL != out_handle);
    out_handle->setInvalid();

    RenderTaskLayoutBuilder *builder = GOSNEW(gos::getScrapAllocator(), GPU::RenderTaskLayoutBuilder) (this, out_handle);
    return *builder;
}

//************************************
bool GPU::priv_renderLayout_onBuilderEnds (RenderTaskLayoutBuilder *builder)
{
    //aggiungo il builder alla lista dei builder da deletare
    toBeDeletedBuilder.add(builder);

    if (builder->anyError())
        return false;
        
    gpu::RenderLayout *s = renderLayoutList.reserve (builder->out_handle);
    if (NULL == s)
    {
        gos::logger::err ("GPU::priv_renderLayout_onBuilderEnds() => can't reserve a handle!\n");
        return false;
    }

    s->vkRenderPassHandle = builder->vkRenderPassHandle;
    s->numColorBuffer = builder->numRenderTargetInfo;
    s->numAttachment = builder->numRenderTargetInfo;
    if (builder->depthBuffer.isRequired)
    {
        s->numAttachment++;
        s->indexOfDepthStencilBuffer = builder->depthBuffer.indexOfDepthStencilAttachment;
    }
    else
    {
        s->indexOfDepthStencilBuffer = 0xff;
    }
    return true;
}

//************************************
void GPU::deleteResource (GPURenderLayoutHandle &handle)
{
    gpu::RenderLayout *s;
    if (renderLayoutList.fromHandleToPointer (handle, &s))
    {
        vkDestroyRenderPass (vulkan.dev, s->vkRenderPassHandle, nullptr);
        s->reset();
        renderLayoutList.release (handle);
    }

    handle.setInvalid();
}

//************************************
const gpu::RenderLayout* GPU::getInfo (const GPURenderLayoutHandle handle) const
{
    gpu::RenderLayout *s;
    if (priv_fromHandleToPointer (renderLayoutList, handle, &s))
        return s;
    return NULL;
}

//************************************
bool GPU::toVulkan (const GPURenderLayoutHandle handle, VkRenderPass *out) const
{
    gpu::RenderLayout *s;
    if (priv_fromHandleToPointer(renderLayoutList, handle, &s))
    {
        *out = s->vkRenderPassHandle;
        return true;
    }

    gos::logger::err ("GPU::renderLayout_toVulkan() => invalid handle\n");
    return false;
}




/************************************************************************************************************
 * Pipeline
 * 
 * 
 *************************************************************************************************************/
GPU::PipelineBuilder& GPU::pipeline_createNew (const GPURenderLayoutHandle &renderLayoutHandle, GPUPipelineHandle *out_handle)
{
    assert (NULL != out_handle);
    out_handle->setInvalid();

    PipelineBuilder *builder = GOSNEW(gos::getScrapAllocator(), GPU::PipelineBuilder) (this, renderLayoutHandle, out_handle);
    return *builder;
}

//************************************
bool GPU::priv_pipeline_onBuilderEnds (PipelineBuilder *builder)
{
    //aggiungo il builder alla lista dei builder da deletare
    toBeDeletedBuilder.add(builder);

    if (builder->anyError())
        return false;
        
    gpu::sPipeline *s = pipelineList.reserve (builder->out_handle);
    if (NULL == s)
    {
        gos::logger::err ("GPU::priv_pipeline_onBuilderEnds() => can't reserve a handle!\n");
        return false;
    }

    s->reset();
    s->vkPipelineLayoutHandle = builder->vkPipelineLayoutHandle;
    s->vkPipelineHandle = builder->vkPipelineHandle;
    if (builder->pushConstantList.getNElem())
        memcpy (s->pushContantList, builder->pushConstantList._queryPointer(), sizeof(VkPushConstantRange) * builder->pushConstantList.getNElem());
    return true;
}

//************************************
void GPU::deleteResource (GPUPipelineHandle &handle)
{
    gpu::sPipeline *s;
    if (pipelineList.fromHandleToPointer (handle, &s))
    {
        vkDestroyPipelineLayout (vulkan.dev, s->vkPipelineLayoutHandle, nullptr);
        vkDestroyPipeline (vulkan.dev, s->vkPipelineHandle, nullptr);
        s->reset();
        pipelineList.release (handle);
    }

    handle.setInvalid();
}

//************************************
bool GPU::toVulkan (const GPUPipelineHandle handle, const gpu::sPipeline **out) const
{
    gpu::sPipeline *s;
    if (pipelineList.fromHandleToPointer (handle, &s))
    {
        *out = s;
        return true;
    }

    *out = NULL;
    gos::logger::err ("GPU::pipeline_toVulkan() => invalid handle\n");
    DBGBREAK;
    return false;
}




/************************************************************************************************************
 * Command buffer
 * 
 * 
 *************************************************************************************************************/
bool GPU::cmdBuffer_create (eGPUQueueType whichQ, GPUCmdBufferHandle *out_handle)
{
    assert (NULL != out_handle);
    out_handle->setInvalid();

    VkCommandBuffer vkCmdBufferHandle;
    if (!vulkanCreateCommandBuffer (vulkan, whichQ, &vkCmdBufferHandle))
    {
        gos::logger::log ("GPU::cmdBuffer_create() => failed\n");
        return false;
    }


    gpu::CommandBuffer *s =cmdBufferList.reserve (out_handle);
    if (NULL == s)
    {
        gos::logger::err ("GPU::cmdBuffer_create() => can't reserve a handle!\n");
        return false;
    }
    s->reset();
    s->vkHandle = vkCmdBufferHandle;
    s->whichQ = whichQ;
    return true;    
}

//************************************
void GPU::deleteResource (GPUCmdBufferHandle &handle)
{
    gpu::CommandBuffer *s;
    if (cmdBufferList.fromHandleToPointer (handle, &s))
    {
        vulkanDeleteCommandBuffer (vulkan, s->whichQ, s->vkHandle);
        s->reset();
        cmdBufferList.release (handle);
    }

    handle.setInvalid();
}

//************************************
bool GPU::toVulkan (const GPUCmdBufferHandle handle, VkCommandBuffer *out) const
{
    assert (NULL != out);
    gpu::CommandBuffer *s;
    if (cmdBufferList.fromHandleToPointer(handle, &s))
    {
        *out = s->vkHandle;
        return true;
    }

    *out = VK_NULL_HANDLE;
    gos::logger::err ("GPU::cmdBuffer_toVulkan() => invalid handle\n");
    return false;    
}



//************************************************************************************************************
bool GPU::priv_bufferCreate (VkBufferUsageFlags vkUsage, u32 sizeInByte, bool bCanBeUsedBy_gfxQ, bool bCanBeUsedBy_computeQ, bool bCanBeUsedBy_transferQ, eVIBufferMode mode, gpu::Buffer *out)
{
    VkMemoryPropertyFlags vkMemProperties;
    switch (mode)
    {
    default:
        gos::logger::err ("GPU::priv_bufferCreate() => invalid mode %d => '%s' \n", mode, gpu::enumToString(mode));
        return false;
        break;

    case eVIBufferMode::onGPU:
        vkMemProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        break;

    case eVIBufferMode::shared_cpuW_autoSync:
        vkMemProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        break;

    case eVIBufferMode::shared_cpuW_manualSync:
        vkMemProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        break;
    
    } 


    //chiedo a Vulkan di creare il buffer
    VkBuffer        vkHandle;
    VkDeviceMemory  vkMemHandle = VK_NULL_HANDLE;
    u64 realMemAllocated;
    if (!vulkanCreateBuffer (vulkan, sizeInByte, 
                        vkUsage,
                        vkMemProperties,
                        bCanBeUsedBy_gfxQ, bCanBeUsedBy_computeQ, bCanBeUsedBy_transferQ,
                        &vkHandle,
                        &vkMemHandle, &realMemAllocated))
    {
        gos::logger::err ("GPU::priv_bufferCreate() => failed to vulkanCreateBuffer()\n");
        return false;
    }

    //pare tutto ok, creo un nuovo handle
    out->reset();
    out->vkHandle = vkHandle;
    out->_vkMemHandle = vkMemHandle;
    out->mode = mode;
    out->bufferSize = sizeInByte;
    out->memoryAllocated = realMemAllocated;
    //out->mapped_offset = 0;
    //out->mapped_size = sizeInByte;    
    //out->mapped_host_pt = NULL;

    switch (mode)
    {
    default:
    case eVIBufferMode::shared_cpuW_manualSync:
    case eVIBufferMode::onGPU:
        break;

    case eVIBufferMode::shared_cpuW_autoSync:
        //mappo la memoria del buffer direttamente qui, visto che questo buffer e' sempre HOST_MAPPABLE e COHERENT
        {
            void *mappedPt;
            VkResult result = vkMapMemory (vulkan.dev, out->_vkMemHandle, 0, sizeInByte, 0, &mappedPt);
            if (VK_SUCCESS != result)
            {
                gos::logger::err ("GPU::priv_bufferCreate() => failed vkMapMemory() => %s\n", string_VkResult(result));
                vkDestroyBuffer (vulkan.dev, out->vkHandle, nullptr);
                return false;
            }    
            out->mapped_offset = 0;
            out->mapped_size = sizeInByte;
            out->mapped_host_pt = reinterpret_cast<u8*>(mappedPt);
        }
        break;
    }

   return true;
}

//************************************************************************************************************
void GPU::buffer_unmap (gpu::sMappedBuffer &m)
{
    if (NULL != m.host_pt)
    {
        vkUnmapMemory(vulkan.dev, m._vkMemHandle);
        memset (&m ,0, sizeof(gpu::sMappedBuffer));
    }
}

//************************************************************************************************************
void GPU::buffer_manualSync (const gpu::sMappedBuffer *list, u32 numElemInList)
{
    assert (numElemInList <= 64);

    VkMappedMemoryRange flush_range[64];
    for (u32 i=0; i<numElemInList; i++)
    {
        flush_range[i].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        flush_range[i].pNext = NULL;
        flush_range[i].memory = list[i]._vkMemHandle;
        flush_range[i].offset = list[i].offset;
        flush_range[i].size = list[i].size;
    }

    vkFlushMappedMemoryRanges (vulkan.dev, numElemInList, flush_range );
}

/************************************************************************************************************
 * Staging buffer
 * 
 * 
 *************************************************************************************************************/
bool GPU::stagingBuffer_create (u32 sizeInByte, GPUStgBufferHandle *out_handle)
{
    assert (NULL != out_handle);
    out_handle->setInvalid();


    gpu::Buffer *s = staginBufferList.reserve (out_handle);
    if (NULL == s)
    {
        gos::logger::err ("GPU::staginBuffer_create() => can't reserve a handle!\n");
        return false;
    }

    if (!priv_bufferCreate (VK_BUFFER_USAGE_TRANSFER_SRC_BIT, sizeInByte, false, false, false, eVIBufferMode::shared_cpuW_autoSync, s))
    {
        staginBufferList.release (*out_handle);
        out_handle->setInvalid();
        gos::logger::err ("GPU::staginBuffer_create() => failed\n");
        return false;
    }

    return true;    
}

//************************************
void GPU::deleteResource (GPUStgBufferHandle &handle)
{
    priv_bufferDestroy (staginBufferList, handle);
}

//************************************
bool GPU::stagingBuffer_uploadToGPUBuffer (const GPUStgBufferHandle handleSRC, const void *dataSRC, const GPUVtxBufferHandle handleDST, u32 offsetDST, u32 howManyByteToCopy)
{
    VkBuffer dstBuffer;
    if (!toVulkan (handleDST, &dstBuffer))
    {
        gos::logger::err ("GPU::stagingBuffer_uploadToGPUBuffer() => invalid handleDST\n");
        return false;
    }
    
    gpu::Buffer *s;
    if (!priv_fromHandleToPointer(staginBufferList, handleSRC, &s))
    {
        gos::logger::err ("GPU::stagingBuffer_uploadToGPUBuffer() => invalid handleSRC\n");
        return false;
    }

    //memcpy di dataSRC nello stagin buffer
    assert (s->mapped_size >= howManyByteToCopy);
    memcpy (s->mapped_host_pt, dataSRC, howManyByteToCopy);

    //copio lo staging buffer nel buffer in GPU
    helperImmediateTransferCmd.begin();
    helperImmediateTransferCmd.copyBuffer (s->vkHandle, dstBuffer, 0, offsetDST, howManyByteToCopy);
    helperImmediateTransferCmd.end();
    return true;
}

//************************************
bool GPU::stagingBuffer_uploadToGPUBuffer (const GPUStgBufferHandle handleSRC, const void *dataSRC, const GPUIdxBufferHandle handleDST, u32 offsetDST, u32 howManyByteToCopy)
{
    VkBuffer dstBuffer;
    if (!toVulkan (handleDST, &dstBuffer))
    {
        gos::logger::err ("GPU::stagingBuffer_uploadToGPUBuffer() => invalid handleDST\n");
        return false;
    }
    
    gpu::Buffer *s;
    if (!priv_fromHandleToPointer(staginBufferList, handleSRC, &s))
    {
        gos::logger::err ("GPU::stagingBuffer_uploadToGPUBuffer() => invalid handleSRC\n");
        return false;
    }

    //memcpy di dataSRC nello stgBuffer
    assert (s->mapped_size >= howManyByteToCopy);
    memcpy (s->mapped_host_pt, dataSRC, howManyByteToCopy);

    //copia di stgBuffer nel buffer in GPU
    helperImmediateTransferCmd.begin();
    helperImmediateTransferCmd.copyBuffer (s->vkHandle, dstBuffer, 0, offsetDST, howManyByteToCopy);
    helperImmediateTransferCmd.end();
    return true;
}



/************************************************************************************************************
 * Vertex buffer
 * 
 * 
 *************************************************************************************************************/
bool GPU::vertexBuffer_create (u32 sizeInByte, eVIBufferMode modeIN, GPUVtxBufferHandle *out_handle)
{
    assert (NULL != out_handle);
    out_handle->setInvalid();


    gpu::Buffer *s = vtxBufferList.reserve (out_handle);
    if (NULL == s)
    {
        gos::logger::err ("GPU::vertexBuffer_create() => can't reserve a handle!\n");
        return false;
    }

    VkBufferUsageFlags vkUsage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    if (modeIN == eVIBufferMode::onGPU)
        vkUsage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    if (!priv_bufferCreate (vkUsage, sizeInByte, true, false, false, modeIN, s))
    {
        vtxBufferList.release (*out_handle);
        out_handle->setInvalid();
        gos::logger::err ("GPU::vertexBuffer_create() => failed\n");
        return false;
    }

    return true;    
}

//************************************
bool GPU::toVulkan (const GPUVtxBufferHandle handle, VkBuffer *out) const
{
    gpu::Buffer *s;
    if (priv_fromHandleToPointer(vtxBufferList, handle, &s))
    {
        *out = s->vkHandle;
        return true;
    }

    *out = VK_NULL_HANDLE;
    gos::logger::err ("GPU::vertexBuffer_toVulkan() => invalid handle\n");
    return false;    
}



/************************************************************************************************************
 * Index buffer
 * 
 * 
 *************************************************************************************************************/
bool GPU::indexBuffer_create (u32 sizeInByte, eVIBufferMode modeIN, GPUIdxBufferHandle *out_handle)
{
    assert (NULL != out_handle);
    out_handle->setInvalid();


    gpu::Buffer *s = idxBufferList.reserve (out_handle);
    if (NULL == s)
    {
        gos::logger::err ("GPU::indexBuffer_create() => can't reserve a handle!\n");
        return false;
    }

    VkBufferUsageFlags vkUsage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    if (modeIN == eVIBufferMode::onGPU)
        vkUsage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    if (!priv_bufferCreate (vkUsage, sizeInByte, true, false, false, modeIN, s))
    {
        idxBufferList.release (*out_handle);
        out_handle->setInvalid();
        gos::logger::err ("GPU::indexBuffer_create() => failed\n");
        return false;
    }

    return true;    
}

//************************************
bool GPU::toVulkan (const GPUIdxBufferHandle handle, VkBuffer *out) const
{
    gpu::Buffer *s;
    if (priv_fromHandleToPointer(idxBufferList, handle, &s))
    {
        *out = s->vkHandle;
        return true;
    }

    *out = VK_NULL_HANDLE;
    gos::logger::err ("GPU::indexBuffer_toVulkan() => invalid handle\n");
    return false;    
}


/************************************************************************************************************
 * uniform buffer
 * 
 * 
 *************************************************************************************************************/
bool GPU::uniformBuffer_create (u32 sizeInByte, eVIBufferMode modeIN, GPUUniformBufferHandle *out_handle)
{
    assert (NULL != out_handle);
    out_handle->setInvalid();


    gpu::Buffer *s = uniformBufferList.reserve (out_handle);
    if (NULL == s)
    {
        gos::logger::err ("GPU::uniformBuffer_create() => can't reserve a handle!\n");
        return false;
    }
    
    VkBufferUsageFlags vkUsage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    if (modeIN == eVIBufferMode::onGPU)
        vkUsage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    if (!priv_bufferCreate (vkUsage, sizeInByte, true, false, false, modeIN, s))
    {
        uniformBufferList.release (*out_handle);
        out_handle->setInvalid();
        gos::logger::err ("GPU::uniformBuffer_create() => failed\n");
        return false;
    }

    return true;
}

//************************************
bool GPU::toVulkan (const GPUUniformBufferHandle handle, VkBuffer *out, u32 *out_bufferSize) const
{
    assert (NULL != out);
    assert (NULL != out_bufferSize);

    gpu::Buffer *s;
    if (priv_fromHandleToPointer (uniformBufferList, handle, &s))
    {
        *out = s->vkHandle;
        *out_bufferSize = s->bufferSize;
        return true;
    }

    *out = VK_NULL_HANDLE;
    gos::logger::err ("GPU::uniformBuffer_toVulkan() => invalid handle\n");
    return false;    
}



/************************************************************************************************************
 * storage buffer
 * 
 * 
 *************************************************************************************************************/
bool GPU::storageBuffer_create (u32 sizeInByte, eVIBufferMode modeIN, GPUStorageBufferHandle *out_handle)
{
    assert (NULL != out_handle);
    out_handle->setInvalid();


    gpu::Buffer *s = storageBufferList.reserve (out_handle);
    if (NULL == s)
    {
        gos::logger::err ("GPU::storageBuffer_create() => can't reserve a handle!\n");
        return false;
    }
    
    VkBufferUsageFlags vkUsage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    if (modeIN == eVIBufferMode::onGPU)
        vkUsage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    if (!priv_bufferCreate (vkUsage, sizeInByte, true, false, false, modeIN, s))
    {
        storageBufferList.release (*out_handle);
        out_handle->setInvalid();
        gos::logger::err ("GPU::storageBuffer_create() => failed\n");
        return false;
    }

    return true;
}

//************************************
bool GPU::toVulkan (const GPUStorageBufferHandle handle, VkBuffer *out, u32 *out_bufferSize) const
{
    assert (NULL != out);
    assert (NULL != out_bufferSize);

    gpu::Buffer *s;
    if (priv_fromHandleToPointer (storageBufferList, handle, &s))
    {
        *out = s->vkHandle;
        *out_bufferSize = s->bufferSize;
        return true;
    }

    *out = VK_NULL_HANDLE;
    gos::logger::err ("GPU::storageBuffer_toVulkan() => invalid handle\n");
    return false;    
}





/************************************************************************************************************
 * DescriptorSet Layput
 * 
 * 
 *************************************************************************************************************/
GPU::DescriptorSetLayoutBuilder& GPU::descrSetLayout_createStatic (GPUDescrSetLayoutHandle *out_handle)
{
    assert (NULL != out_handle);
    out_handle->setInvalid();

    DescriptorSetLayoutBuilder *builder = GOSNEW(gos::getScrapAllocator(), GPU::DescriptorSetLayoutBuilder) (this, 0, out_handle);
    return *builder;
}

GPU::DescriptorSetLayoutBuilder& GPU::descrSetLayout_createPushable (GPUDescrSetLayoutHandle *out_handle)
{
    assert (NULL != out_handle);
    out_handle->setInvalid();

    DescriptorSetLayoutBuilder *builder = GOSNEW(gos::getScrapAllocator(), GPU::DescriptorSetLayoutBuilder) (this, VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR, out_handle);
    return *builder;
}

GPU::DescriptorSetLayoutBuilder& GPU::descrSetLayout_createDynamic (GPUDescrSetLayoutHandle *out_handle)
{
    assert (NULL != out_handle);
    out_handle->setInvalid();

    DescriptorSetLayoutBuilder *builder = GOSNEW(gos::getScrapAllocator(), GPU::DescriptorSetLayoutBuilder) (this, VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT, out_handle);
    return *builder;
}

//************************************
bool GPU::priv_descrSetLayout_onBuilderEnds (DescriptorSetLayoutBuilder *builder)
{
    //aggiungo il builder alla lista dei builder da deletare
    toBeDeletedBuilder.add(builder);

    if (builder->anyError())
        return false;

    VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsCreateInfo {};
    VkDescriptorBindingFlags bindingFlags[GOSGPU__NUM_MAX_DESCRIPTOR_PER_SET];

    //TODO: cachare i descriptor-set ed eventualmente riutilizzarli visto che sono dei descrittori, non e' necessario
    //      crearne N diversi che descrivono la stessa cosa
    VkDescriptorSetLayoutCreateInfo creatInfo{};
    creatInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    creatInfo.flags = builder->createFlag;
    creatInfo.bindingCount = builder->numDescriptor;
    creatInfo.pBindings = builder->list;

    if ((creatInfo.flags & VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT) != 0)
    {
        for (u32 i=0; i<builder->numDescriptor; i++)
        {
            bindingFlags[i] = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT;
        }

        bindingFlagsCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
        bindingFlagsCreateInfo.pNext = nullptr;
        bindingFlagsCreateInfo.bindingCount = builder->numDescriptor;
        bindingFlagsCreateInfo.pBindingFlags = bindingFlags;

        creatInfo.pNext = &bindingFlagsCreateInfo;
    }
    else
    {
        creatInfo.pNext = NULL;
    }

    VkDescriptorSetLayout vkHandle;
    VkResult result = vkCreateDescriptorSetLayout (vulkan.dev, &creatInfo, nullptr, &vkHandle);
    if (VK_SUCCESS != result)
    {
        gos::logger::err ("GPU::priv_descrSetLayout_onBuilderEnds () => vkCreateDescriptorSetLayout failed => %s\n", string_VkResult(result));
        return false;
    }

    gpu::DescrSetLayout *s = descrSetLayoutList.reserve (builder->out_handle);
    if (NULL == s)
    {
        gos::logger::err ("GPU::priv_descrSetLayout_onBuilderEnds() => can't reserve a handle!\n");
        return false;
    }
    s->reset();
    s->vkHandle = vkHandle;
    return true;
}

//************************************
void GPU::deleteResource (GPUDescrSetLayoutHandle &handle)
{
    gpu::DescrSetLayout *s;
    if (descrSetLayoutList.fromHandleToPointer (handle, &s))
    {
        vkDestroyDescriptorSetLayout (vulkan.dev, s->vkHandle, nullptr);
        s->reset();
        descrSetLayoutList.release (handle);
    }

    handle.setInvalid();
}

//************************************
bool GPU::toVulkan (const GPUDescrSetLayoutHandle handle, VkDescriptorSetLayout *out) const
{
    gpu::DescrSetLayout *s;
    if (priv_fromHandleToPointer(descrSetLayoutList,handle, &s))
    {
        *out = s->vkHandle;
        return true;
    }

    *out = VK_NULL_HANDLE;
    gos::logger::err ("GPU::descrSetLayout_toVulkan() => invalid handle\n");
    return false;    
}




/************************************************************************************************************
 * Descriptor pool
 * 
 * 
 *************************************************************************************************************/
GPU::DescriptorPoolBuilder& GPU::descrPool_createNew (GPUDescrPoolHandle *out_handle)
{
    assert (NULL != out_handle);
    out_handle->setInvalid();

    DescriptorPoolBuilder *builder = GOSNEW(gos::getScrapAllocator(), GPU::DescriptorPoolBuilder) (this, out_handle);
    return *builder;
}    

//************************************
bool GPU::priv_descrPool_onBuilderEnds (DescriptorPoolBuilder *builder)
{
    //aggiungo il builder alla lista dei builder da deletare
    toBeDeletedBuilder.add(builder);

    if (builder->anyError())
        return false;

    VkDescriptorPoolCreateInfo creatInfo{};
    creatInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    creatInfo.poolSizeCount = builder->numPool;
    creatInfo.pPoolSizes = builder->list;
    creatInfo.maxSets = builder->numMaxDescriptorSets;
    creatInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;

    VkDescriptorPool vkHandle;
    VkResult result = vkCreateDescriptorPool (vulkan.dev, &creatInfo, nullptr, &vkHandle);
    if (VK_SUCCESS != result)
    {
        gos::logger::err ("GPU::priv_descrPool_onBuilderEnds () => vkCreateDescriptorPool failed => %s\n", string_VkResult(result));
        return false;
    }

    gpu::DescrPool *s = descrPoolList.reserve (builder->out_handle);
    if (NULL == s)
    {
        gos::logger::err ("GPU::priv_descrPool_onBuilderEnds() => can't reserve a handle!\n");
        return false;
    }
    s->reset();
    s->vkHandle = vkHandle;
    s->flags = builder->vkPoolFlags;
    return true;
}

//************************************
void GPU::deleteResource (GPUDescrPoolHandle &handle)
{
    gpu::DescrPool *s;
    if (descrPoolList.fromHandleToPointer (handle, &s))
    {
        vkDestroyDescriptorPool (vulkan.dev, s->vkHandle, nullptr);
        s->reset();
        descrPoolList.release (handle);
    }

    handle.setInvalid();
}

//************************************
bool GPU::toVulkan (const GPUDescrPoolHandle handle, VkDescriptorPool *out) const
{
    gpu::DescrPool *s;
    if (priv_fromHandleToPointer(descrPoolList,handle, &s))
    {
        *out = s->vkHandle;
        return true;
    }

    *out = VK_NULL_HANDLE;
    gos::logger::err ("GPU::descrPool_toVulkan() => invalid handle\n");
    return false;    
}





/************************************************************************************************************
 * DescriptorSet instance
 * 
 * 
 *************************************************************************************************************/
bool GPU::descrSetInstance_createNew (const GPUDescrPoolHandle &poolHandle, const GPUDescrSetLayoutHandle &descrSetLayoutHandle, GPUDescrSetInstanceHandle *out_handle)
{
    assert (NULL != out_handle);
    out_handle->setInvalid();

    gpu::DescrPool *pool;
    if (!descrPoolList.fromHandleToPointer (poolHandle, &pool))
    {
        gos::logger::err ("GPU::descrSetInstance_createNew() => invalid pool handle\n");
        return false;
    }


    VkDescriptorSetLayout vkDescSetLayoutHandle;
    if (!toVulkan (descrSetLayoutHandle, &vkDescSetLayoutHandle))
    {
        gos::logger::err ("GPU::descrSetInstance_createNew() => invalid descrSetLayoutHandle handle\n");
        return false;
    }


    gpu::DescrSetInstance *s = descrSetInstanceList.reserve (out_handle);
    if (NULL == s)
    {
        gos::logger::err ("GPU::descrSetInstance_createNew() => can't reserve a handle!\n");
        descrSetInstanceList.release(*out_handle);
        return false;
    }

    s->reset();
    s->vkPoolHandle = pool->vkHandle;
    s->bCanBeFreed = ( (pool->flags & VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT) != 0 );



    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = pool->vkHandle;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &vkDescSetLayoutHandle;

    const VkResult result = vkAllocateDescriptorSets (vulkan.dev, &allocInfo, &s->vkHandle);
    if (VK_SUCCESS == result)
        return true;


    s->reset();
    descrSetInstanceList.release(*out_handle);
    gos::logger::err ("GPU::descrSetInstance_createNew () => vkAllocateDescriptorSets failed => %s\n", string_VkResult(result));
    return false;
}

//************************************
void GPU::deleteResource (GPUDescrSetInstanceHandle &handle)
{
    gpu::DescrSetInstance *s;
    if (descrSetInstanceList.fromHandleToPointer (handle, &s))
    {
        if (s->bCanBeFreed)
            vkFreeDescriptorSets (vulkan.dev, s->vkPoolHandle, 1, &s->vkHandle);
        s->reset();
        descrSetInstanceList.release (handle);
    }

    handle.setInvalid();
}

//************************************
bool GPU::toVulkan (const GPUDescrSetInstanceHandle handle, VkDescriptorSet *out) const
{
    gpu::DescrSetInstance *s;
    if (priv_fromHandleToPointer(descrSetInstanceList,handle, &s))
    {
        *out = s->vkHandle;
        return true;
    }

    *out = VK_NULL_HANDLE;
    gos::logger::err ("GPU::descrSetInstance_toVulkan() => invalid handle\n");
    return false;    
}




/************************************************************************************************************
 * Texture
 * 
 * 
 *************************************************************************************************************/
bool GPU::texture_create2D (u16 dimx, u16 dimy, u8 nMipMap, eImageFormat fmt, const void *srcDATA, GPUTextureHandle *out_handle)
{
    assert (NULL != out_handle);
    assert (nMipMap >= 1);
    out_handle->setInvalid();



    //chiedo a Vulkan di creare img
    VkImage         vkImageHandle;
    VkDeviceMemory  vkMemHandle = VK_NULL_HANDLE;
    u32             imageMemSize = 0;

    if (!vulkanCreateImage2D (vulkan, dimx, dimy, nMipMap, gos::gpu::toVulkan(fmt),
                                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
                                VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                                VK_IMAGE_TILING_OPTIMAL,
                                &vkImageHandle, &vkMemHandle, &imageMemSize))
    {
        gos::logger::err ("GPU::texture_create2D() => failed\n");
        return false;
    }

    //srcData non lo posso usare "as-is", lo devo copiare in uno staging buffer
    if (helperStagingBuffer.isInvalid())
        priv_createHelperStagingBuffer(4096 * 4096);
    
    gpu::Buffer *stg;
    if (!staginBufferList.fromHandleToPointer (helperStagingBuffer, &stg))
    {
        gos::logger::err ("GPU::texture_create2D() => unable to access the 'helperStaginBuffer'\n");
        return false;
    }
    assert (stg->mapped_size >= imageMemSize);
    memcpy (stg->mapped_host_pt, srcDATA, imageMemSize);


#ifdef _DEBUG
    {
        u32 totalImgSize = 0;
        u32 w = dimx;
        u32 h = dimy;
        for (u8 i=0; i<nMipMap; i++)
        {
            totalImgSize += image::getFormatSize(fmt) * w * h;
            w/=2;
            h/=2;
        }
        assert (totalImgSize <= imageMemSize);
    }
#endif


    //L'immagine appena creata ha il layout VK_IMAGE_LAYOUT_UNDEFINED
    //Per poterci copiare dentro srcDATA, devo trasformarla in VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
    helperImmediateTransferCmd.begin();
    helperImmediateTransferCmd.transitionImageLayout (vkImageHandle, nMipMap, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    helperImmediateTransferCmd.end();

    //una volta che immagine è in stato VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, ci posso copiare dentro il contenuto dello stgBuffer
    helperImmediateTransferCmd.begin();
    {
        assert (nMipMap < 32);
        VkBufferImageCopy regionList[32];
        u32 w = dimx;
        u32 h = dimy;
        u32 offset = 0;
        for (u8 i=0; i<nMipMap; i++)
        {
            regionList[i].bufferOffset = offset;
            regionList[i].bufferRowLength = 0;
            regionList[i].bufferImageHeight = 0;

            regionList[i].imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            regionList[i].imageSubresource.mipLevel = i;
            regionList[i].imageSubresource.baseArrayLayer = 0;
            regionList[i].imageSubresource.layerCount = 1;

            regionList[i].imageOffset = {0, 0, 0};
            regionList[i].imageExtent = { w, h, 1};

            offset += w * h * image::getFormatSize(fmt);
            w/=2;
            h/=2;
        }

        vkCmdCopyBufferToImage(
            helperImmediateTransferCmd.vkCmdBuffer,
            stg->vkHandle,
            vkImageHandle,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            nMipMap,
            regionList
        );        
    }
    helperImmediateTransferCmd.end();


    //infine, devo transizionare l'immagine da VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL a VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    helperImmediateTransferCmd.begin();
    helperImmediateTransferCmd.transitionImageLayout (vkImageHandle, nMipMap, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    helperImmediateTransferCmd.end();


    //pare tutto ok, creo un nuovo handle
    gpu::Texture *s = textureList.reserve (out_handle);
    if (NULL == s)
    {
        gos::logger::err ("GPU::texture_create2D() => can't reserve a handle!\n");
        return false;
    }
    
    s->reset();
    s->dimx = dimx;
    s->dimy = dimy;
    s->nMipMap = nMipMap;
    s->nArray = 1;
    s->vkHandle = vkImageHandle;
    s->vkMemHandle = vkMemHandle;
    s->memoryAllocated = imageMemSize;


    //creo una view per la texture
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = vkImageHandle;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = gos::gpu::toVulkan(fmt);
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = nMipMap;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;    


    VkResult result = vkCreateImageView(vulkan.dev, &viewInfo, nullptr, &s->view);
    if (VK_SUCCESS != result)
    {
        gos::logger::err ("GPU::texture_create2D () => vkCreateImageView failed => %s\n", string_VkResult(result));
        return false;
    }

    return true;
}

//************************************
bool GPU::texture_create2D (const gos::Image *im, u8 srcTextureNum, GPUTextureHandle *out_handle)
{
    const image::sTextureHeader *header = image::getTextureInfo (*im, srcTextureNum);
    if (NULL == header)
    {
        gos::logger::err ("GPU::texture_create2D() => invalid image, can't extract header\n");
        return false;
    }

    image::sTextureData texData;
    if (!image::getTextureData (*im, srcTextureNum, 0, &texData))
    {
        gos::logger::err ("GPU::texture_create2D() => invalid image, can't extract texture data\n");
        return false;
    }

    return texture_create2D (header->width, header->height, header->numMipMap, header->fmt, texData.textureData, out_handle);
}

//************************************
void GPU::deleteResource (GPUTextureHandle &handle)
{
    gpu::Texture *s;
    if (textureList.fromHandleToPointer (handle, &s))
    {
        if (VK_NULL_HANDLE != s->view)
            vkDestroyImageView (vulkan.dev, s->view, nullptr);

        if (VK_NULL_HANDLE != s->vkHandle)
            vkDestroyImage (vulkan.dev, s->vkHandle, nullptr);

        if (VK_NULL_HANDLE != s->vkMemHandle)
            vulkanFreeMemory (vulkan, s->vkMemHandle, nullptr, s->memoryAllocated);

        s->reset();
        textureList.release (handle);
    }


    handle.setInvalid();
}

//************************************
bool GPU::toVulkan (const GPUTextureHandle handle, VkImageView *out) const
{
    gpu::Texture *s;
    if (priv_fromHandleToPointer(textureList,handle, &s))
    {
        *out = s->view;
        return true;
    }

    *out = VK_NULL_HANDLE;
    gos::logger::err ("GPU::texture_toVulkan() => invalid handle\n");
    return false;    
}


/************************************************************************************************************
 * sampler
 * 
 * 
 *************************************************************************************************************/
bool GPU::sampler_create (const gpu::SamplerDesc &desc, GPUSamplerHandle *out_handle)
{
    //prima di tutto cerco se esiste gia' un sampler con gli stessi parametri
    HashMap<u32, GPUSamplerHandle>::Position insertPosition;
    if (samplerDescrHashMap.findWithPos (desc.toU32(), out_handle, & insertPosition))
        return true;


    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

    samplerInfo.minFilter = gos::gpu::toVulkan(desc.minFilter);
    samplerInfo.magFilter = gos::gpu::toVulkan(desc.magFilter);
    if (desc.bAnisotropic)
    {
        samplerInfo.anisotropyEnable = VK_TRUE;
        samplerInfo.maxAnisotropy = vulkan.phyDevInfo.deviceProperties.limits.maxSamplerAnisotropy;
    }
    else
    {
        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.maxAnisotropy = 1.0f;
    }

    samplerInfo.addressModeU = static_cast<VkSamplerAddressMode>(desc.addressModeU);
    samplerInfo.addressModeV = static_cast<VkSamplerAddressMode>(desc.addressModeV);
    samplerInfo.addressModeW = static_cast<VkSamplerAddressMode>(desc.addressModeW);
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;

    if (eSamplerCompFunc::DISABLED == desc.compareFn)
    {
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    }
    else
    {
        samplerInfo.compareEnable = VK_TRUE;
        samplerInfo.compareOp = static_cast<VkCompareOp>(desc.compareFn);
    }

    switch (desc.mipFilter)
    {
    default:
        gos::logger::err ("gpu::sampler_create() => invalid mipFilter\n");
        return false;

    case eSamplerMipFilter::nearest:
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        break;

    case eSamplerMipFilter::linear:
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        break;
    }
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = VK_LOD_CLAMP_NONE;

    VkSampler vkHandle;
    VkResult result = vkCreateSampler(vulkan.dev, &samplerInfo, nullptr, &vkHandle);
    if (VK_SUCCESS != result)
    {
        gos::logger::err ("GPU::sampler_create () => vkCreateSampler failed => %s\n", string_VkResult(result));
        return false;
    }


   //pare tutto ok, creo un nuovo handle
    gpu::Sampler *s = samplerList.reserve (out_handle);
    if (NULL == s)
    {
        gos::logger::err ("GPU::sampler_create() => can't reserve a handle!\n");
        return false;
    }
    
    s->reset();
    s->vkHandle = vkHandle;
    s->desc = desc;

    samplerDescrHashMap.insertInPosition (insertPosition, *out_handle);
    return true;
}

//************************************
void GPU::priv_samplerDelete (GPUSamplerHandle &handle)
{
    gpu::Sampler *s;
    if (samplerList.fromHandleToPointer (handle, &s))
    {
        if (VK_NULL_HANDLE != s->vkHandle)
            vkDestroySampler (vulkan.dev, s->vkHandle, nullptr);

        s->reset();
        samplerList.release (handle);
    }


    handle.setInvalid();
}

//************************************
bool GPU::toVulkan (const GPUSamplerHandle handle, VkSampler *out) const
{
    gpu::Sampler *s;
    if (priv_fromHandleToPointer(samplerList,handle, &s))
    {
        *out = s->vkHandle;
        return true;
    }

    *out = VK_NULL_HANDLE;
    gos::logger::err ("GPU::sampler_toVulkan() => invalid handle\n");
    return false;    
}





