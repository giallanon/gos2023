#include "gosGPU.h"
#include "gosGPUUtils.h"
#include "vulkan/gosGPUVulkan.h"
#include "../gos/string/gosStringList.h"
#include "../gos/gos.h"
#include "../gos/memory/gosAllocatorHeap.h"

using namespace gos;


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
    currentSwapChainImageIndex = 0;
    bRecreateSwapChainOnNextFrame = false;
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

        toBeDeletedBuilder.deleteAll();
        toBeDeletedBuilder.unsetup();

        frameBufferDependentOnSwapChainList.unsetup();

        //elimino l'handle del default RT
        renderTargetList.release (defaultRTHandle);

        //elimino la vport di default
        deleteResource (defaultViewportHandle);

        priv_deinitandleLists();
        priv_deinitVulkan();
        priv_deinitWindowSystem();
    gos::logger::decIndent();

    GOSDELETE(gos::getSysHeapAllocator(), allocator);
    allocator = NULL;
}    

//********************************************************** 
bool GPU::init (u16 width, u16 height, bool vSyncIN, const char *appName)
{
    vSync = vSyncIN;

    gos::logger::log ("GPU::init (%d, %d)\n", width, height);
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
        if (!priv_initWindowSystem (width, height, appName))
            break;
        if (!priv_initHandleLists())
            break;
        if (!priv_initVulkan())
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
        rt->viewAsRT = NULL;
        rt->viewAsTexture = NULL;
        rt->width = vulkan.swapChainInfo.imageExtent.width;
        rt->height = vulkan.swapChainInfo.imageExtent.height;
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
bool GPU::priv_initWindowSystem(u16 width, u16 height, const char *appName)
{
    gos::logger::log ("GPU::priv_initWindowSystem(%d,%d)\n", width, height);
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    window.win = glfwCreateWindow (width, height, appName, nullptr, nullptr);
    return (window.win != NULL);
}

//**********************************************************
void  GPU::priv_deinitWindowSystem()
{
    gos::logger::log ("GPU::priv_deinitWindowSystem()\n");
    if (NULL != window.win)
    {
        glfwDestroyWindow (window.win);
        glfwTerminate();
        window.win = NULL;
    }    
}

//**********************************************************
bool GPU::priv_initVulkan ()
{
    gos::logger::log ("GPU::priv_initVulkan()\n");
    gos::Allocator *scrapAllocator = gos::getScrapAllocator();

    gos::StringList requiredVulkanExtensionList(scrapAllocator);
    gos::StringList requiredVulkanValidaionLayerList(scrapAllocator);

    //GLFW ha bisogno di un po' di estensioni di vulkan, le recupero e le addo all'elenco delle estensioni necessarie
    {
        u32 glfwExtensionCount = 0;
        const char **glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        for (u32 i=0; i<glfwExtensionCount; i++)
            requiredVulkanExtensionList.add (glfwExtensions[i]);
    }

#ifdef _DEBUG
    requiredVulkanValidaionLayerList.add ("VK_LAYER_KHRONOS_validation");
    requiredVulkanExtensionList.add (VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    
#endif

    //creazione dell'istanza di vulkan
    if (!vulkanCreateInstance (&vkInstance, requiredVulkanValidaionLayerList, requiredVulkanExtensionList))
    {
        gos::logger::err ("problem creating vulkan instance\n");
        return false;
    }
    
    //aggiungo una callback per il layer di debug, giusto per printare i msg di vulkan in un bel colore rosa
#ifdef _DEBUG    
    priv_vulkanAddDebugCallback();
#endif

    //creo una surface basata sulla [window]
    //GLFW fa tutto da solo, ma in linea di massima questa parte sarebbe dipendente da piattaforma
    VkResult result = glfwCreateWindowSurface(vkInstance, window.win, nullptr, &vkSurface);
    if (VK_SUCCESS != result)
    {
        gos::logger::err ("glfwCreateWindowSurface() returned %s\n", string_VkResult(result));
        return false;
    }

    //cerco un physical device che sia appropriato
    sPhyDeviceInfo vkPhysicalDevInfo;
    requiredVulkanExtensionList.reset();
    requiredVulkanExtensionList.add (VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    if (!vulkanScanAndSelectAPhysicalDevices(vkInstance, vkSurface, requiredVulkanExtensionList, &vkPhysicalDevInfo))
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
    if (!vulkanCreateDevice (vkPhysicalDevInfo, requiredVulkanExtensionList, &vulkan))
    {
        gos::logger::err ("can't create a logical device\n");
        return false;
    }
    gos::logger::log("\n");
    
    //initVulkan:: creazione swap chain
    if (!vulkanCreateSwapChain (vulkan, vkSurface, vSync, &vulkan.swapChainInfo))
    {
        gos::logger::err ("can't create swap chain\n");
        return false;
    }    
    gos::logger::log("\n");

    //tutto ok
    gos::logger::log("\n");
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
    renderLayoutList.setup (allocator);
    pipelineList.setup (allocator);
    frameBufferList.setup (allocator);
    vtxBufferList.setup (allocator);
    staginBufferList.setup (allocator);
    idxBufferList.setup (allocator);
    descrLayoutList.setup (allocator);
    uniformBufferList.setup (allocator);
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
    renderLayoutList.unsetup ();
    pipelineList.unsetup();
    frameBufferList.unsetup();
    vtxBufferList.unsetup();
    staginBufferList.unsetup();
    idxBufferList.unsetup();
    descrLayoutList.unsetup();
    uniformBufferList.unsetup();
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
bool GPU::newFrame (u64 timeout_ns, VkSemaphore semaphore, VkFence fence)
{
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
VkResult GPU::present (const VkSemaphore *semaphoreHandleList, u32 semaphoreCount)
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
    gos::logger::log (eTextColor::green, "GPU::swapChain_recreate()\n");
    gos::logger::incIndent();

    int width = 0;
    int height = 0;
    while (width == 0 || height == 0) 
    {
        gos::logger::log ("windows size is weird (w=%d, h=%d), waiting...\n", width, height);
        glfwGetFramebufferSize(window.win, &width, &height);
        glfwWaitEvents();
    }

    //attendo che Vulkan sia in idel
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

    //aggiorno tutti i depth buffer che hanno una dimensione/posizione relativa
    n = depthStencilHandleList.getNElem();
    for (u32 i=0; i<n; i++)
    {
        gos::gpu::DepthStencil *s;
        if (depthStencilList.fromHandleToPointer (depthStencilHandleList(i), &s))
        {
            if (!s->width.isAbsolute() || !s->height.isAbsolute())
            {
                priv_depthStencil_deleteFromStruct (*s);
                s->resolve (vportW, vportH);
                priv_depthStencil_createFromStruct (*s);
            }
        }
    }

    //ricredo tutti i FrameBuffer che sono dipendenti dalla swapchain
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

//************************************
bool GPU::createCommandBuffer (eGPUQueueType whichQ, VkCommandBuffer *out)
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = vulkan.getQueueInfo(whichQ)->vkPoolHandle;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    const VkResult result = vkAllocateCommandBuffers (vulkan.dev, &allocInfo, out);
    if (result == VK_SUCCESS)
        return true;
    
    gos::logger::log ("GPU::createCommandBuffer() => vkAllocateCommandBuffers() => %s\n", string_VkResult(result));
    return false;
}

//************************************
bool GPU::deleteCommandBuffer (eGPUQueueType whichQ, VkCommandBuffer &vkHandle)
{
    VkCommandBuffer vkCmdBufferList[] = { vkHandle };

    vkFreeCommandBuffers (vulkan.dev, vulkan.getQueueInfo(whichQ)->vkPoolHandle, 1, vkCmdBufferList);
    return true;
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

    GLFWmonitor *monitor = glfwGetWindowMonitor(window.win);
    if (NULL == monitor)
    {
        //andiamo in full
        window.storeCurrentPosAndSize();
        gos::logger::log ("going full screen, current win pos and size (%d,%d) (%d,%d)\n", window.storedX, window.storedY, window.storedW, window.storedH);

        monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode *mode = glfwGetVideoMode(monitor);
        glfwSetWindowMonitor (window.win, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
    }    
    else
    {
        //torniamo in windowed
        gos::logger::log ("going in windowed mode, current win pos and size (%d,%d) (%d,%d)\n", window.storedX, window.storedY, window.storedW, window.storedH);
        glfwSetWindowMonitor(window.win, NULL, window.storedX, window.storedY, window.storedW, window.storedH, 0);
    }

    gos::logger::decIndent();
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
bool GPU::priv_copyVulkanBuffer (const VkBuffer srcBuffer, const VkBuffer dstBuffer, u32 offsetSRC, u32 offsetDST, u32 howManyByteToCopy)
{
    //perparo un command buffer per il trasferimento dei dati
    VkCommandBuffer vkCmdBuffer;
    if (!createCommandBuffer (gos::eGPUQueueType::transfer, &vkCmdBuffer))
    {
        gos::logger::err ("GPU::priv_copyVulkanBuffer() => createCommandBuffer failed\n");
        return false;
    }

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer (vkCmdBuffer, &beginInfo);    

    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = offsetSRC;
    copyRegion.dstOffset = offsetDST;
    copyRegion.size = howManyByteToCopy;
    vkCmdCopyBuffer(vkCmdBuffer, srcBuffer, dstBuffer, 1, &copyRegion);    

    vkEndCommandBuffer(vkCmdBuffer);

    //submit
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &vkCmdBuffer;
    vkQueueSubmit (vulkan.getQueueInfo(eGPUQueueType::transfer)->vkQueueHandle, 1, &submitInfo, VK_NULL_HANDLE);
    
    //attendo
    waitIdle (eGPUQueueType::transfer);

    deleteCommandBuffer (gos::eGPUQueueType::transfer, vkCmdBuffer);
    return true;    
}

/************************************************************************************************************
 * Shader
 * 
 * 
 *************************************************************************************************************/
bool GPU::priv_shader_createFromFile (const u8 *filename, eShaderType shaderType, const char *mainFnName, GPUShaderHandle *out_shaderHandle)
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

/************************************
bool GPU::priv_fromHandleToPointer (const GPUShaderHandle handle, gpu::Shader **out) const
{
    assert (NULL != out);
    if (shaderList.fromHandleToPointer(handle, out))
        return true;
    
    *out = NULL;
    gos::logger::err ("GPU => unable to get shader from handle (handle=%0x08X)\n", handle.viewAsU32());
    return false;
}
*/

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

/************************************
bool GPU::priv_fromHandleToPointer (const GPUVtxDeclHandle handle, gpu::VtxDecl **out) const
{
    assert (NULL != out);
    if (vtxDeclList.fromHandleToPointer(handle, out))
        return true;
    
    *out = NULL;
    gos::logger::err ("GPU => unable to get VtxDecl from handle (handle=%0x08X)\n", handle.viewAsU32());
    return false;
}
*/

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
 * DepthStencil
 * 
 * 
 *************************************************************************************************************/
bool GPU::depthStencil_create (const gos::Dim2D &widthIN, const gos::Dim2D &heightIN, bool bWithStencil, GPUDepthStencilHandle *out_handle)
{
    assert (NULL != out_handle);

    //cerco il miglior formato disponibile 
    VkFormat depthStencilFormat;
    bool b = true;
    if (bWithStencil)
        b = gos::vulkanFindBestDepthStencilFormat (vulkan.phyDevInfo, &depthStencilFormat);
    else
        b = gos::vulkanFindBestDepthFormat (vulkan.phyDevInfo, &depthStencilFormat);
    if (!b)
    {
        gos::logger::err ("GPU::depthStencil_create() => can't find a suitabile format\n");
        return false;
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
    depthStencil->depthFormat = depthStencilFormat;
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
    {
        int winDimX;
        int winDimY;
        window.getCurrentSize (&winDimX, &winDimY);
        depthStencil.resolve ((i16)winDimX, (i16)winDimY);
    }

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

    VkMemoryAllocateInfo memAllloc{};
	memAllloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memAllloc.allocationSize = memReqs.size;
    vulkanGetMemoryType (vulkan.phyDevInfo, memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &memAllloc.memoryTypeIndex);

	result = vkAllocateMemory (vulkan.dev, &memAllloc, nullptr, &depthStencil.vkMemHandle);
    if (VK_SUCCESS != result)
    {
        gos::logger::err ("GPU::priv_depthStenicl_createFromStruct() => vkAllocateMemory() => %s\n", string_VkResult(result));
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
	    vkFreeMemory(vulkan.dev, depthStencil.vkMemHandle, nullptr);
        depthStencil.vkMemHandle = VK_NULL_HANDLE;
    }
}



/************************************************************************************************************
 * Render Target
 * 
 * 
 *************************************************************************************************************/




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
        gos::logger::err ("GPU::priv_renderTarget__onBuilderEnds() => can't reserve a handle!\n");
        return false;
    }


    //Fillo la struttura con i dati di creazione recuperati dal builder
    s->reset();
    s->width = builder->width;
    s->height = builder->height;

    
    //render layout. Mi accerto che sia valido
    sRenderLayout *sRL;
    if (!priv_fromHandleToPointer (renderLayoutList, builder->renderLayoutHandle, &sRL))
    {
        gos::logger::err ("GPU::priv_renderTarget__onBuilderEnds() => invalid renderLayoutHandle\n");
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
            gos::logger::err ("GPU::priv_renderTarget__onBuilderEnds() => invalid depthstencil handle\n");
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
                gos::logger::err ("GPU::priv_renderTarget__onBuilderEnds() => invalid render target handle at index %d\n", i);
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
#ifdef _DEBUG
    else    
        DBGBREAK;
#endif

    handle.setInvalid();
}

/************************************
bool GPU::priv_frameBuffer_fromHandleToPointer (const GPUFrameBufferHandle handle, gpu::FrameBuffer **out) const
{
    assert (NULL != out);
    if (frameBufferList.fromHandleToPointer (handle, out))
        return true;

    gos::logger::err ("GPU::priv_frameBuffer_fromHandleToPointer() => invalid handle\n");
    return false;
}
*/
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

    //render layout
    sRenderLayout *sRL;
    if (!priv_fromHandleToPointer (renderLayoutList, s->renderLayoutHandle, &sRL))
        return false;


    //Se sono bindato al mainRT, devo creare N VulkanFrameBuffer, 1 per ogni immagine della swapchain
    s->numFrameBuffer = 1;
    if (s->boundToMainRT)
        s->numFrameBuffer = vulkan.swapChainInfo.imageCount;

    for (u32 t=0; t<s->numFrameBuffer; t++)
    {
        //render target
        VkImageView imageViewList[GOSGPU__NUM_MAX_RENDER_TARGET + 1];
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

                assert (NULL != sRT->viewAsRT);
                imageViewList[nViewList++] = sRT->viewAsRT;
            }
        }

        //depthStencil
        if (s->depthStencilHandle.isValid())
        {
            //TODO
            DBGBREAK;
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
            gos::logger::err ("GPU::priv_frameBuffer_recreate() => vkCreateFramebuffer => %s\n", string_VkResult(result));
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
        
    sRenderLayout *s = renderLayoutList.reserve (builder->out_handle);
    if (NULL == s)
    {
        gos::logger::err ("GPU::priv_renderLayout_onBuilderEnds() => can't reserve a handle!\n");
        return false;
    }

    s->vkRenderPassHandle = builder->vkRenderPassHandle;
    return true;
}

//************************************
void GPU::deleteResource (GPURenderLayoutHandle &handle)
{
    sRenderLayout *s;
    if (renderLayoutList.fromHandleToPointer (handle, &s))
    {
        vkDestroyRenderPass (vulkan.dev, s->vkRenderPassHandle, nullptr);
        s->reset();
        renderLayoutList.release (handle);
    }

    handle.setInvalid();
}

//************************************
bool GPU::toVulkan (const GPURenderLayoutHandle handle, VkRenderPass *out) const
{
    sRenderLayout *s;
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
        
    sPipeline *s = pipelineList.reserve (builder->out_handle);
    if (NULL == s)
    {
        gos::logger::err ("GPU::priv_pipeline_onBuilderEnds() => can't reserve a handle!\n");
        return false;
    }

    s->vkPipelineLayoutHandle = builder->vkPipelineLayoutHandle;
    s->vkPipelineHandle = builder->vkPipelineHandle;
    return true;
}

//************************************
void GPU::deleteResource (GPUPipelineHandle &handle)
{
    sPipeline *s;
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
bool GPU::toVulkan (const GPUPipelineHandle handle, VkPipeline *out, VkPipelineLayout *out_layout) const
{
    sPipeline *s;
    if (pipelineList.fromHandleToPointer (handle, &s))
    {
        *out = s->vkPipelineHandle;
        *out_layout = s->vkPipelineLayoutHandle;
        return true;
    }

    *out = VK_NULL_HANDLE;
    *out_layout = VK_NULL_HANDLE;
    gos::logger::err ("GPU::pipeline_toVulkan() => invalid handle\n");
    DBGBREAK;
    return false;
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

    VkBufferUsageFlags      vkUsage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    VkMemoryPropertyFlags   vkMemProperties =0;
    bool                    bCanBeUsedByQueue_gfx = true;
    bool                    bCanBeUsedByQueue_compute = false;
    bool                    bCanBeUsedByQueue_transfer = false;

    switch (modeIN)
    {
    default:
        gos::logger::err ("GPU::vertexBuffer_create() => invalid param mode\n");
        return false;

    case eVIBufferMode::onGPU:
        //questo buffer e' accessibile solo da GPU, CPU non puo' mapparlo
        vkUsage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        vkMemProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

        //L'unico modo per accedere al buffer e' usare un CommandBuffer che trasferisca tramite la Q di transfer
        //dalla memoria CPU alla memoria GPU
        bCanBeUsedByQueue_transfer = true;
        break;

    case eVIBufferMode::mappale:
        //questo buffer e' accessibile anche da CPU tramite le fn map()/unmap()
        vkMemProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        break;
    }

    //chiedo a Vulkan di creare il buffer
    VkBuffer        vkHandle;
    VkDeviceMemory  vkMemHandle = VK_NULL_HANDLE;
    if (!vulkanCreateBuffer (vulkan, sizeInByte, 
                        vkUsage,
                        vkMemProperties,
                        bCanBeUsedByQueue_gfx, bCanBeUsedByQueue_compute, bCanBeUsedByQueue_transfer,
                        &vkHandle,
                        &vkMemHandle))
    {
        gos::logger::err ("GPU::vertexBuffer_create() => failed\n");
        return false;
    }


    //pare tutto ok, creo un nuovo handle
    gpu::VtxBuffer *s = vtxBufferList.reserve (out_handle);
    if (NULL == s)
    {
        gos::logger::err ("GPU::vertexBuffer_create() => can't reserve a handle!\n");
        return false;
    }
    s->reset();
    s->mode = modeIN;
    s->vkHandle = vkHandle;
    s->vkMemHandle = vkMemHandle;
    return true;
}

/************************************
bool GPU::priv_vertexBuffer_fromHandleToPointer (const GPUVtxBufferHandle handle, gpu::VtxBuffer **out) const
{
    assert (NULL != out);
    if (vtxBufferList.fromHandleToPointer (handle, out))
        return true;

    gos::logger::err ("GPU::priv_vertexBuffer_fromHandleToPointer() => invalid handle\n");
    DBGBREAK;
    return false;
}
*/

//************************************
void GPU::deleteResource (GPUVtxBufferHandle &handle)
{
    gpu::VtxBuffer *s;
    if (vtxBufferList.fromHandleToPointer (handle, &s))
    {
        vkDestroyBuffer (vulkan.dev, s->vkHandle, nullptr);
        vkFreeMemory (vulkan.dev, s->vkMemHandle, nullptr);
        s->reset();
        vtxBufferList.release (handle);
    }

    handle.setInvalid();
}

//************************************
bool GPU::toVulkan (const GPUVtxBufferHandle handle, VkBuffer *out) const
{
    gpu::VtxBuffer *s;
    if (priv_fromHandleToPointer(vtxBufferList, handle, &s))
    {
        *out = s->vkHandle;
        return true;
    }

    *out = VK_NULL_HANDLE;
    gos::logger::err ("GPU::vertexBuffer_toVulkan() => invalid handle\n");
    return false;    
}

//************************************
bool GPU::vertexBuffer_map (const GPUVtxBufferHandle handle, u32 offsetDST, u32 sizeInByte, void **out) const
{
    assert (NULL != out);

    gpu::VtxBuffer *s;
    if (!priv_fromHandleToPointer(vtxBufferList, handle, &s))
    {
        gos::logger::err ("GPU::vertexBuffer_Map() => invalid handle\n");
        return false;
    }

    if (eVIBufferMode::mappale != s->mode)
    {
        gos::logger::err ("GPU::vertexBuffer_Map() => invalid buffer mode. Buffer mode must be MAPPABLE, current mode is %s\n", gpu::enumToString(s->mode));
        return false;
    }

    VkResult result = vkMapMemory (vulkan.dev, s->vkMemHandle, offsetDST, sizeInByte, 0, out);
    if (VK_SUCCESS != result)
    {
        *out = NULL;
        gos::logger::err ("GPU::vertexBuffer_Map(d) => vkMapMemory() => %s\n", string_VkResult(result));
        return false;
    }

    return true;
}

//************************************
bool GPU::vertexBuffer_unmap  (const GPUVtxBufferHandle handle)
{
    gpu::VtxBuffer *s;
    if (!priv_fromHandleToPointer(vtxBufferList, handle, &s))
    {
        gos::logger::err ("GPU::vertexBuffer_Unmap() => invalid handle\n");
        return false;
    }

    vkUnmapMemory(vulkan.dev, s->vkMemHandle);
    return true;
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

    VkBufferUsageFlags      vkUsage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    VkMemoryPropertyFlags   vkMemProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;


    //chiedo a Vulkan di creare il buffer
    VkBuffer        vkHandle;
    VkDeviceMemory  vkMemHandle = VK_NULL_HANDLE;
    if (!vulkanCreateBuffer (vulkan, sizeInByte, 
                        vkUsage,
                        vkMemProperties,
                        false, false, false,
                        &vkHandle,
                        &vkMemHandle))
    {
        gos::logger::err ("GPU::staginBuffer_create() => failed\n");
        return false;
    }

    //pare tutto ok, creo un nuovo handle
    gpu::StagingBuffer *s = staginBufferList.reserve (out_handle);
    if (NULL == s)
    {
        gos::logger::err ("GPU::staginBuffer_create() => can't reserve a handle!\n");
        return false;
    }
    s->reset();
    s->vkHandle = vkHandle;
    s->vkMemHandle = vkMemHandle;
    s->allocatedSizeInByte = sizeInByte;
    return true;    
}

//************************************
void GPU::deleteResource (GPUStgBufferHandle &handle)
{
    gpu::StagingBuffer *s;
    if (staginBufferList.fromHandleToPointer (handle, &s))
    {
        vkDestroyBuffer (vulkan.dev, s->vkHandle, nullptr);
        vkFreeMemory (vulkan.dev, s->vkMemHandle, nullptr);
        s->reset();
        staginBufferList.release (handle);
    }

    handle.setInvalid();
}

//************************************
bool GPU::toVulkan (const GPUStgBufferHandle handle, VkBuffer *out) const
{
    gpu::StagingBuffer *s;
    if (priv_fromHandleToPointer(staginBufferList, handle, &s))
    {
        *out = s->vkHandle;
        return true;
    }

    *out = VK_NULL_HANDLE;
    gos::logger::err ("GPU::stagingBuffer_toVulkan() => invalid handle\n");
    return false;    
}

/************************************
bool GPU::priv_stagingBuffer_fromHandleToPointer (const GPUStgBufferHandle handle, gpu::StagingBuffer **out) const
{
    assert (NULL != out);
    if (staginBufferList.fromHandleToPointer (handle, out))
        return true;

    gos::logger::err ("GPU::priv_staginBuffer_fromHandleToPointer() => invalid handle\n");
    DBGBREAK;
    return false;
}
*/

//************************************
bool GPU::stagingBuffer_map (const GPUStgBufferHandle handle, u32 offsetDST, u32 sizeInByte, void **out) const
{
    assert (NULL != out);

    gpu::StagingBuffer *s;
    if (!priv_fromHandleToPointer(staginBufferList, handle, &s))
    {
        gos::logger::err ("GPU::stagingBuffer_map() => invalid handle\n");
        return false;
    }

    if (NULL == s->mapped_pt)
    {

        VkResult result = vkMapMemory (vulkan.dev, s->vkMemHandle, offsetDST, sizeInByte, 0, &s->mapped_pt);
        if (VK_SUCCESS != result)
        {
            *out = NULL;
            gos::logger::err ("GPU::stagingBuffer_map(d) => vkMapMemory() => %s\n", string_VkResult(result));
            return false;
        }
        s->mapped_offset = offsetDST;
        s->mapped_size = sizeInByte;
    }

    //gli uniform sono sempre mappati in memoria, lo faccio durante la create
    if (sizeInByte > s->mapped_size)
    {
        gos::logger::err ("GPU::uniformBuffer_map() => invalid params1 (%d, %d). Buffer size is %d\n", offsetDST, sizeInByte, s->mapped_size);
        return false;
    }

    if (offsetDST + sizeInByte > s->mapped_size)
    {
        gos::logger::err ("GPU::uniformBuffer_map() => invalid params2 (%d, %d). Buffer size is %d, mapped from %d\n", offsetDST, sizeInByte, s->mapped_size, s->mapped_offset);
        return false;
    }

    *out = s->mapped_pt;

    return true;
}

//************************************
bool GPU::stagingBuffer_unmap  (const GPUStgBufferHandle handle)
{
    gpu::StagingBuffer *s;
    if (!priv_fromHandleToPointer(staginBufferList, handle, &s))
    {
        gos::logger::err ("GPU::stagingBuffer_unmap() => invalid handle\n");
        return false;
    }

    s->mapped_pt = NULL;
    s->mapped_offset = 0;
    s->mapped_size = 0;

    vkUnmapMemory(vulkan.dev, s->vkMemHandle);
    return true;
}

//************************************
bool GPU::stagingBuffer_copyToBuffer (const GPUStgBufferHandle handleSRC, const GPUVtxBufferHandle handleDST, u32 offsetSRC, u32 offsetDST, u32 howManyByteToCopy)
{
    //ora creo un job per copiare via GPU il buffer di staging nel vxtBuffer
    VkBuffer srcBuffer;
    if (!toVulkan (handleSRC, &srcBuffer))
    {
        gos::logger::err ("GPU::stagingBuffer_copyToVtxBuffer() => invalid handleSRC\n");
        return false;
    }    

    VkBuffer dstBuffer;
    if (!toVulkan (handleDST, &dstBuffer))
    {
        gos::logger::err ("GPU::stagingBuffer_copyToVtxBuffer() => invalid handleDST\n");
        return false;
    }    

    return priv_copyVulkanBuffer (srcBuffer, dstBuffer, offsetSRC, offsetDST, howManyByteToCopy);
}

//************************************
bool GPU::stagingBuffer_copyToBuffer (const GPUStgBufferHandle handleSRC, const GPUIdxBufferHandle handleDST, u32 offsetSRC, u32 offsetDST, u32 howManyByteToCopy)
{
    //ora creo un job per copiare via GPU il buffer di staging nel vxtBuffer
    VkBuffer srcBuffer;
    if (!toVulkan (handleSRC, &srcBuffer))
    {
        gos::logger::err ("GPU::stagingBuffer_copyToIdxBuffer() => invalid handleSRC\n");
        return false;
    }    

    VkBuffer dstBuffer;
    if (!toVulkan (handleDST, &dstBuffer))
    {
        gos::logger::err ("GPU::stagingBuffer_copyToIdxBuffer() => invalid handleDST\n");
        return false;
    }    

    return priv_copyVulkanBuffer (srcBuffer, dstBuffer, offsetSRC, offsetDST, howManyByteToCopy);
}




/************************************************************************************************************
 * Vertex buffer
 * 
 * 
 *************************************************************************************************************/
bool GPU::indexBuffer_create (u32 sizeInByte, eVIBufferMode modeIN, GPUIdxBufferHandle *out_handle)
{
    assert (NULL != out_handle);
    out_handle->setInvalid();

    VkBufferUsageFlags      vkUsage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    VkMemoryPropertyFlags   vkMemProperties =0;
    bool                    bCanBeUsedByQueue_gfx = true;
    bool                    bCanBeUsedByQueue_compute = false;
    bool                    bCanBeUsedByQueue_transfer = false;

    switch (modeIN)
    {
    default:
        gos::logger::err ("GPU::indexBuffer_create() => invalid param mode\n");
        return false;

    case eVIBufferMode::onGPU:
        //questo buffer e' accessibile solo da GPU, CPU non puo' mapparlo
        vkUsage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        vkMemProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

        //L'unico modo per accedere al buffer e' usare un CommandBuffer che trasferisca tramite la Q di transfer
        //dalla memoria CPU alla memoria GPU
        bCanBeUsedByQueue_transfer = true;
        break;

    case eVIBufferMode::mappale:
        //questo buffer e' accessibile anche da CPU tramite le fn map()/unmap()
        vkMemProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        break;
    }

    //chiedo a Vulkan di creare il buffer
    VkBuffer        vkHandle;
    VkDeviceMemory  vkMemHandle = VK_NULL_HANDLE;
    if (!vulkanCreateBuffer (vulkan, sizeInByte, 
                        vkUsage,
                        vkMemProperties,
                        bCanBeUsedByQueue_gfx, bCanBeUsedByQueue_compute, bCanBeUsedByQueue_transfer,
                        &vkHandle,
                        &vkMemHandle))
    {
        gos::logger::err ("GPU::indexBuffer_create() => failed\n");
        return false;
    }


    //pare tutto ok, creo un nuovo handle
    gpu::IdxBuffer *s = idxBufferList.reserve (out_handle);
    if (NULL == s)
    {
        gos::logger::err ("GPU::indexBuffer_create() => can't reserve a handle!\n");
        return false;
    }
    s->reset();
    s->mode = modeIN;
    s->vkHandle = vkHandle;
    s->vkMemHandle = vkMemHandle;
    return true;
}

/************************************
bool GPU::priv_indexBuffer_fromHandleToPointer (const GPUIdxBufferHandle handle, gpu::IdxBuffer **out) const
{
    assert (NULL != out);
    if (idxBufferList.fromHandleToPointer (handle, out))
        return true;

    gos::logger::err ("GPU::priv_indexBuffer_fromHandleToPointer() => invalid handle\n");
    DBGBREAK;
    return false;
}
*/

//************************************
void GPU::deleteResource (GPUIdxBufferHandle &handle)
{
    gpu::IdxBuffer *s;
    if (idxBufferList.fromHandleToPointer (handle, &s))
    {
        vkDestroyBuffer (vulkan.dev, s->vkHandle, nullptr);
        vkFreeMemory (vulkan.dev, s->vkMemHandle, nullptr);
        s->reset();
        idxBufferList.release (handle);
    }

    handle.setInvalid();
}

//************************************
bool GPU::toVulkan (const GPUIdxBufferHandle handle, VkBuffer *out) const
{
    gpu::IdxBuffer *s;
    if (priv_fromHandleToPointer(idxBufferList, handle, &s))
    {
        *out = s->vkHandle;
        return true;
    }

    *out = VK_NULL_HANDLE;
    gos::logger::err ("GPU::indexBuffer_toVulkan() => invalid handle\n");
    return false;    
}

//************************************
bool GPU::indexBuffer_map (const GPUIdxBufferHandle handle, u32 offsetDST, u32 sizeInByte, void **out) const
{
    assert (NULL != out);

    gpu::IdxBuffer *s;
    if (!priv_fromHandleToPointer(idxBufferList, handle, &s))
    {
        gos::logger::err ("GPU::indexBuffer_Map() => invalid handle\n");
        return false;
    }

    if (eVIBufferMode::mappale != s->mode)
    {
        gos::logger::err ("GPU::indexBuffer_Map() => invalid buffer mode. Buffer mode must be MAPPABLE, current mode is %s\n", gpu::enumToString(s->mode));
        return false;
    }

    VkResult result = vkMapMemory (vulkan.dev, s->vkMemHandle, offsetDST, sizeInByte, 0, out);
    if (VK_SUCCESS != result)
    {
        *out = NULL;
        gos::logger::err ("GPU::indexBuffer_Map(d) => vkMapMemory() => %s\n", string_VkResult(result));
        return false;
    }

    return true;
}

//************************************
bool GPU::indexBuffer_unmap  (const GPUIdxBufferHandle handle)
{
    gpu::IdxBuffer *s;
    if (!priv_fromHandleToPointer(idxBufferList,handle, &s))
    {
        gos::logger::err ("GPU::indexBuffer_Unmap() => invalid handle\n");
        return false;
    }

    vkUnmapMemory(vulkan.dev, s->vkMemHandle);
    return true;
}




/************************************************************************************************************
 * Description Layput
 * 
 * 
 *************************************************************************************************************/
bool GPU::descrLayout_create (const VkDescriptorSetLayoutCreateInfo &creatInfo, GPUDescrLayoutHandle *out_handle)
{
    VkDescriptorSetLayout vkHandle;
    VkResult result = vkCreateDescriptorSetLayout(vulkan.dev, &creatInfo, nullptr, &vkHandle);
    if (VK_SUCCESS != result)
    {
        gos::logger::err ("GPU::descrLayout_create () => vkCreateDescriptorSetLayout failed => %s\n", string_VkResult(result));
        return false;
    }

    gpu::DescrLayout *s = descrLayoutList.reserve (out_handle);
    if (NULL == s)
    {
        gos::logger::err ("GPU::descrLayout_create() => can't reserve a handle!\n");
        return false;
    }
    s->reset();
    s->vkHandle = vkHandle;
    return true;
}

//************************************
void GPU::deleteResource (GPUDescrLayoutHandle &handle)
{
    gpu::DescrLayout *s;
    if (descrLayoutList.fromHandleToPointer (handle, &s))
    {
        vkDestroyDescriptorSetLayout (vulkan.dev, s->vkHandle, nullptr);
        s->reset();
        descrLayoutList.release (handle);
    }

    handle.setInvalid();
}

//************************************
bool GPU::toVulkan (const GPUDescrLayoutHandle handle, VkDescriptorSetLayout *out) const
{
    gpu::DescrLayout *s;
    if (priv_fromHandleToPointer(descrLayoutList,handle, &s))
    {
        *out = s->vkHandle;
        return true;
    }

    *out = VK_NULL_HANDLE;
    gos::logger::err ("GPU::descrLayout_toVulkan() => invalid handle\n");
    return false;    
}



/************************************************************************************************************
 * Description Layput
 * 
 * 
 *************************************************************************************************************/
bool GPU::uniformBuffer_create (u32 sizeInByte, GPUUniformBufferHandle *out_handle)
{
    assert (NULL != out_handle);
    out_handle->setInvalid();

    const VkBufferUsageFlags      vkUsage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    const VkMemoryPropertyFlags   vkMemProperties =VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    const bool                    bCanBeUsedByQueue_gfx = true;
    const bool                    bCanBeUsedByQueue_compute = false;
    const bool                    bCanBeUsedByQueue_transfer = false;


    //chiedo a Vulkan di creare il buffer
    VkBuffer        vkHandle;
    VkDeviceMemory  vkMemHandle = VK_NULL_HANDLE;
    if (!vulkanCreateBuffer (vulkan, sizeInByte, 
                        vkUsage,
                        vkMemProperties,
                        bCanBeUsedByQueue_gfx, bCanBeUsedByQueue_compute, bCanBeUsedByQueue_transfer,
                        &vkHandle,
                        &vkMemHandle))
    {
        gos::logger::err ("GPU::uniformBuffer_create() => failed\n");
        return false;
    }

    //dato che e' sempre HOST_VISIBLE e HOST_COHERENT, lo mappo direttamente ora e lo lascio per sempre mappato
    void *host_pt;
    VkResult result = vkMapMemory (vulkan.dev, vkMemHandle, 0, sizeInByte, 0, &host_pt);
    if (VK_SUCCESS != result)
    {
        gos::logger::err ("GPU::uniformBuffer_create() => fail to map buffer. vkMapMemory() => %s\n", string_VkResult(result));
        return false;
    }


    //pare tutto ok, creo un nuovo handle
    gpu::UniformBuffer *s = uniformBufferList.reserve (out_handle);
    if (NULL == s)
    {
        gos::logger::err ("GPU::uniformBuffer_create() => can't reserve a handle!\n");
        return false;
    }
    s->reset();
    s->vkHandle = vkHandle;
    s->vkMemHandle = vkMemHandle;
    s->mapped_host_pt = host_pt;
    s->mapped_offset = 0;
    s->bufferSize = sizeInByte;


    return true;
}

//************************************
void GPU::deleteResource (GPUUniformBufferHandle &handle)
{
    gpu::UniformBuffer *s;
    if (uniformBufferList.fromHandleToPointer (handle, &s))
    {
        vkDestroyBuffer (vulkan.dev, s->vkHandle, nullptr);
        vkFreeMemory (vulkan.dev, s->vkMemHandle, nullptr);
        s->reset();
        uniformBufferList.release (handle);
    }
    handle.setInvalid();
}

//************************************
bool GPU::toVulkan (const GPUUniformBufferHandle handle, VkBuffer *out) const
{
    gpu::UniformBuffer *s;
    if (priv_fromHandleToPointer (uniformBufferList, handle, &s))
    {
        *out = s->vkHandle;
        return true;
    }

    *out = VK_NULL_HANDLE;
    gos::logger::err ("GPU::uniformBuffer_toVulkan() => invalid handle\n");
    return false;    
}

//************************************
bool GPU::uniformBuffer_map (const GPUUniformBufferHandle handle, u32 offsetDST, u32 sizeInByte, void **out) const
{
    assert (NULL != out);

    gpu::UniformBuffer *s;
    if (!priv_fromHandleToPointer(uniformBufferList, handle, &s))
    {
        gos::logger::err ("GPU::uniformBuffer_map() => invalid handle\n");
        return false;
    }

    //gli uniform sono sempre mappati in memoria, lo faccio durante la create
    if (sizeInByte > s->bufferSize)
    {
        gos::logger::err ("GPU::uniformBuffer_map() => invalid params1 (%d, %d). Buffer size is %d\n", offsetDST, sizeInByte, s->bufferSize);
        return false;
    }

    if (offsetDST + sizeInByte > s->bufferSize)
    {
        gos::logger::err ("GPU::uniformBuffer_map() => invalid params2 (%d, %d). Buffer size is %d, mapped from %d\n", offsetDST, sizeInByte, s->bufferSize, s->mapped_offset);
        return false;
    }

    *out = s->mapped_host_pt;
    return true;
}

//************************************
bool GPU::uniformBuffer_mapCopyUnmap (const GPUUniformBufferHandle handle, u32 offsetDST, u32 sizeInByte, const void *src) const
{
    void *p;
    if (uniformBuffer_map (handle, offsetDST, sizeInByte, &p))
    {
        memcpy (p, src, sizeInByte);
        //uniformBuffer_unmap (handle);
        return true;
    }

    return false;
}

