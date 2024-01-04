#include "gosGPU.h"
#include "vulkan/gosGPUVulkan.h"
#include "../gos/string/gosStringList.h"
#include "../gos/gos.h"
#include "../gos/memory/gosAllocatorHeap.h"

using namespace gos;


typedef gos::AllocatorHeap<gos::AllocPolicy_Track_simple, gos::AllocPolicy_Thread_Safe>		GOSGPUMemAllocatorTS;

//********************************************************** 
GPU::GPU()
{
    this->allocator = NULL;
    vkInstance = VK_NULL_HANDLE;
    vkSurface = VK_NULL_HANDLE;
    vkDebugMessenger = VK_NULL_HANDLE;
    defaultViewportHandle.setInvalid();
    defaultRTHandleList = NULL;
    currentSwapChainImageIndex = 0;
    bNeedToRecreateSwapchain = false;
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

        //elimino i default RT
        for (u32 i=0; i<vulkan.swapChainInfo.imageCount; i++)
            renderTargetList.release (defaultRTHandleList[i]);
        GOSFREE(allocator, defaultRTHandleList);


        deleteResource (defaultViewportHandle);

        priv_deinitandleLists();
        priv_deinitVulkan();
        priv_deinitWindowSystem();
    gos::logger::decIndent();

    GOSDELETE(gos::getSysHeapAllocator(), allocator);
    allocator = NULL;
}    

//********************************************************** 
bool GPU::init (u16 width, u16 height, const char *appName)
{
    gos::logger::log ("GPU::init (%d, %d)\n", width, height);
    gos::logger::incIndent();

    //Creo un allocatore dedicato per la GPU
    GOSGPUMemAllocatorTS *gpuAllocator = GOSNEW(gos::getSysHeapAllocator(), GOSGPUMemAllocatorTS)("GPU");
    gpuAllocator->setup (1024 * 1024 * 128); //128MB
    this->allocator = gpuAllocator;

    //lista dei builder temporanei da deletare
    toBeDeletedBuilder.setup();


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

    //creo un renderTarget per ogni image della swapchain
    defaultRTHandleList = GOSALLOCT(GPURenderTargetHandle*, allocator, sizeof(GPURenderTargetHandle) * vulkan.swapChainInfo.imageCount);
    for (u32 i=0; i<vulkan.swapChainInfo.imageCount; i++)
    {
        gpu::RenderTarget *rt = renderTargetList.reserve(&defaultRTHandleList[i]);
        rt->reset();
        rt->format = vulkan.swapChainInfo.imageFormat;
        rt->image = VK_NULL_HANDLE;
        rt->mem = VK_NULL_HANDLE;
        rt->viewAsRT = vulkan.swapChainInfo.vkImageListView[i];
        rt->viewAsTexture = VK_NULL_HANDLE;
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
        gos::logger::log (eTextColor::green, "\nselected device is at index %d, gfx Q family at index:%d, compute Q family at index: %d\n", vkPhysicalDevInfo.devIndex, vkPhysicalDevInfo.gfxQIndex, vkPhysicalDevInfo.computeQIndex);
    }
    gos::logger::log("\n");

    //creazione del device logico di vulkan
    if (!vulkanCreateDevice (vkPhysicalDevInfo, requiredVulkanExtensionList, &vulkan))
    {
        gos::logger::err ("can't create a logical device\n");
        return false;
    }
    gos::logger::log("\n");
    
    //creazione swap chain
    if (!vulkanCreateSwapChain (vulkan, vkSurface, &vulkan.swapChainInfo))
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
}

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

    gos::logger::logWithPrefix (eTextColor::magenta, prefix, "%s\n", pCallbackData->pMessage);
    return VK_FALSE;
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
VkResult GPU::fence_wait (const VkFence *fenceHandleList, u32 fenceCount, u64 timeout_ns)
{
    return vkWaitForFences (vulkan.dev, fenceCount, fenceHandleList, VK_TRUE, timeout_ns);
}

//************************************
void GPU::fence_reset (const VkFence *fenceHandleList, u32 fenceCount)
{
    vkResetFences (vulkan.dev, fenceCount, fenceHandleList);
}

//************************************
bool GPU::newFrame (bool *out_bSwapchainWasRecreated, u32 *out_imageIndex,u64 timeout_ns, VkSemaphore semaphore, VkFence fence)
{
    const u64 timeNow_msec = gos::getTimeSinceStart_msec();
    
    toBeDeletedBuilder.check (timeNow_msec);

    const VkResult result = vkAcquireNextImageKHR (vulkan.dev, vulkan.swapChainInfo.vkSwapChain, timeout_ns, semaphore, fence, &currentSwapChainImageIndex);
    *out_imageIndex = currentSwapChainImageIndex;

    switch (result)
    {
    default:
        gos::logger::err ("GPU::beginFrame() => vkAcquireNextImageKHR() => %s\n", string_VkResult(result));
        *out_bSwapchainWasRecreated = false;
        return false;

    case VK_SUCCESS:
        *out_bSwapchainWasRecreated = false;
        return true;

    case VK_ERROR_OUT_OF_DATE_KHR:
        priv_swapChain_recreate();
        *out_bSwapchainWasRecreated = true;
        return false;
                
    case VK_SUBOPTIMAL_KHR:
        //posso ancora renderizzare, ma alla fine del prossimo present() la swapchain verra' ricreata
        bNeedToRecreateSwapchain = true;
        *out_bSwapchainWasRecreated = true;
        return true;
    }
}

//************************************
VkResult GPU::present (const VkSemaphore *semaphoreHandleList, u32 semaphoreCount, u32 imageIndex)
{
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = semaphoreCount;
    presentInfo.pWaitSemaphores = semaphoreHandleList; //prima di presentare, aspetta che GPU segnali tutti i semafori di [semaphoreHandleList]
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &vulkan.swapChainInfo.vkSwapChain;
    presentInfo.pImageIndices = &imageIndex;
    
    const VkResult result = vkQueuePresentKHR (vulkan.gfxQ, &presentInfo);

    if (bNeedToRecreateSwapchain)
    {
        priv_swapChain_recreate();
        bNeedToRecreateSwapchain = false;
    }

    return result;
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


    bool ret = true;
    vkDeviceWaitIdle (vulkan.dev);

    vulkan.swapChainInfo.destroy(vulkan.dev);

    //ricreazione swap chain
    if (!vulkanCreateSwapChain (vulkan, this->vkSurface, &vulkan.swapChainInfo))
    {
        gos::logger::err ("can't create swap chain\n");
        ret = false;
    }
    
    //attuale dimensione della vport
    const i16 vportW = (i16)vulkan.swapChainInfo.imageExtent.width;
    const i16 vportH = (i16)vulkan.swapChainInfo.imageExtent.height;

    //aggiorno le view del default RT
    for (u32 i=0; i<vulkan.swapChainInfo.imageCount; i++)
    {
        gpu::RenderTarget *rt;
        priv_renderTarget_fromHandleToPointer (defaultRTHandleList[i], &rt);
        rt->format = vulkan.swapChainInfo.imageFormat;
        rt->viewAsRT = vulkan.swapChainInfo.vkImageListView[i];
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
                priv_depthStenicl_deleteFromStruct (*s);
                s->resolve (vportW, vportH);
                priv_depthStenicl_createFromStruct (*s);
            }
        }
    }



    gos::logger::decIndent();
    return ret;  
}

//************************************
bool GPU::createCommandBuffer (VkCommandBuffer *out)
{
    gos::logger::log ("GPU::createCommandBuffer: ");

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = vulkan.commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    const VkResult result = vkAllocateCommandBuffers (vulkan.dev, &allocInfo, out);
    if (result == VK_SUCCESS)
    {
        gos::logger::log (eTextColor::green, "OK\n");
        return true;
    }
    
    gos::logger::err ("%s\n", string_VkResult(result));
    return false;
}

//**********************************************************
void  GPU::waitIdle()
{
    vkDeviceWaitIdle(vulkan.dev);
}

//************************************
bool GPU::priv_shader_fromHandleToPointer (const GPUShaderHandle handle, gpu::Shader **out) const
{
    assert (NULL != out);
    if (shaderList.fromHandleToPointer(handle, out))
        return true;
    
    *out = NULL;
    gos::logger::err ("GPU => unable to get shader from handle (handle=%0x08X)\n", handle.viewAsU32());
    return false;
}

//**********************************************************
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

//************************************
void GPU::deleteResource (GPUShaderHandle &shaderHandle)
{
    gpu::Shader *shader;
    if (priv_shader_fromHandleToPointer(shaderHandle, &shader))
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
    if (priv_shader_fromHandleToPointer(shaderHandle, &shader))
        return shader->_vkHandle;
    return VK_NULL_HANDLE;
}

//************************************
const char* GPU::shader_getMainFnName (const GPUShaderHandle shaderHandle) const
{
    gpu::Shader *shader;
    if (priv_shader_fromHandleToPointer(shaderHandle, &shader))
        return shader->_mainFnName;
    return NULL;
}

//************************************
eShaderType GPU::shader_getType (const GPUShaderHandle shaderHandle) const
{
    gpu::Shader *shader;
    if (priv_shader_fromHandleToPointer(shaderHandle, &shader))
        return shader->_shaderType;
    return eShaderType::unknown;
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
bool GPU::priv_vxtDecl_fromHandleToPointer (const GPUVtxDeclHandle handle, gpu::VtxDecl **out) const
{
    assert (NULL != out);
    if (vtxDeclList.fromHandleToPointer(handle, out))
        return true;
    
    *out = NULL;
    gos::logger::err ("GPU => unable to get VtxDecl from handle (handle=%0x08X)\n", handle.viewAsU32());
    return false;
}

//************************************
bool GPU::vtxDecl_query (const GPUVtxDeclHandle handle, gpu::VtxDecl *out) const
{
    assert (out);
    gpu::VtxDecl *p;
    if (priv_vxtDecl_fromHandleToPointer(handle, &p))
    {
        //ritorno una copia dell'oggetto, non il pt all'oggetto
        (*out) = (*p);
        return true;
    }

    out->reset();
    return false;
}

//************************************
GPU::VtxDeclBuilder& GPU::vtxDecl_createNew (GPUVtxDeclHandle *out_handle)
{
    out_handle->setInvalid();
    vtxDeclBuilder.priv_begin(this, out_handle);
    return vtxDeclBuilder;
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
void GPU::deleteResource (GPUVtxDeclHandle &handle)
{
    gpu::VtxDecl *s;
    if (priv_vxtDecl_fromHandleToPointer(handle, &s))
    {
        s->reset();
        vtxDeclList.release(handle);
    }
    handle.setInvalid();
}

//************************************
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
const gpu::Viewport* GPU::viewport_get (const GPUViewportHandle &handle) const
{
    gos::gpu::Viewport *v;
    if (!viewportlList.fromHandleToPointer (handle, &v))
        return NULL;
    return v;
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
bool GPU::depthBuffer_create (const gos::Dim2D &widthIN, const gos::Dim2D &heightIN, bool bWithStencil, GPUDepthStencilHandle *out_handle)
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
        gos::logger::err ("GPU::depthBuffer_create() => can't find a suitabile format\n");
        return false;
    }


    //riservo un handle
    gos::gpu::DepthStencil *depthStencil = depthStencilList.reserve (out_handle);
    if (NULL == depthStencil)
    {
        gos::logger::err ("GPU::depthBuffer_create() => can't reserve a new depthStencil handle!\n");
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
    if (!priv_depthStenicl_createFromStruct (*depthStencil))
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
        priv_depthStenicl_deleteFromStruct (*s);
        s->reset();
        depthStencilList.release (handle);
        depthStencilHandleList.findAndRemove (handle);
    }

    handle.setInvalid();
}

//************************************
bool GPU::priv_depthStenicl_createFromStruct (gos::gpu::DepthStencil &depthStencil)
{
    assert (VK_NULL_HANDLE == depthStencil.image);
    assert (VK_NULL_HANDLE == depthStencil.mem);
    assert (VK_NULL_HANDLE == depthStencil.view);
    assert (VK_FORMAT_UNDEFINED != depthStencil.depthFormat);

    //risolvo la dimensione
    {
        int winDimX;
        int winDimY;
        window.getCurrentSize (&winDimX, &winDimY);
        depthStencil.resolve (winDimX, winDimY);
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

	result = vkAllocateMemory (vulkan.dev, &memAllloc, nullptr, &depthStencil.mem);
    if (VK_SUCCESS != result)
    {
        gos::logger::err ("GPU::priv_depthStenicl_createFromStruct() => vkAllocateMemory() => %s\n", string_VkResult(result));
        return false;
    }

	result = vkBindImageMemory (vulkan.dev, depthStencil.image, depthStencil.mem, 0);
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
void GPU::priv_depthStenicl_deleteFromStruct (gos::gpu::DepthStencil &depthStencil)
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

    if (VK_NULL_HANDLE != depthStencil.mem)
    {
	    vkFreeMemory(vulkan.dev, depthStencil.mem, nullptr);
        depthStencil.mem = VK_NULL_HANDLE;
    }
}

//************************************
bool GPU::priv_renderTarget_fromHandleToPointer (const GPURenderTargetHandle handle, gpu::RenderTarget **out) const
{
    assert (NULL != out);
    if (renderTargetList.fromHandleToPointer(handle, out))
        return true;
    
    *out = NULL;
    gos::logger::err ("GPU => unable to get renderTarget from handle (handle=%0x08X)\n", handle.viewAsU32());
    return false;
}



//************************************
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
    bool ret = true;

    while (1)
    {
        if (builder->anyError())
        {
            ret = false;
            break;
        }
        
        sRenderLayout *s = renderLayoutList.reserve (builder->out_handle);
        if (NULL == s)
        {
            gos::logger::err ("GPU::priv_renderLayout_onBuilderEnds() => can't reserve a handle!\n");
            ret = false;
            break;
        }

        s->vkRenderPassHandle = builder->vkRenderPassHandle;
        break;
    }

    //aggiungo il builder alla lista dei builder da deletare
    toBeDeletedBuilder.add(builder);

    //fine
    return ret;
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
bool GPU::renderLayout_toVulkan (const GPURenderLayoutHandle handle, VkRenderPass *out) const
{
    sRenderLayout *s;
    if (renderLayoutList.fromHandleToPointer (handle, &s))
    {
        *out = s->vkRenderPassHandle;
        return true;
    }

    gos::logger::err ("GPU::renderLayout_toVulkan() => invalid handle\n");
    DBGBREAK;
    return false;
}



//************************************
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
    bool ret = true;

    while (1)
    {
        if (builder->anyError())
        {
            ret = false;
            break;
        }
        
        sPipeline *s = pipelineList.reserve (builder->out_handle);
        if (NULL == s)
        {
            gos::logger::err ("GPU::priv_pipeline_onBuilderEnds() => can't reserve a handle!\n");
            ret = false;
            break;
        }

        s->vkPipelineLayoutHandle = builder->vkPipelineLayoutHandle;
        s->vkPipelineHandle = builder->vkPipelineHandle;
        break;
    }

    //aggiungo il builder alla lista dei builder da deletare
    toBeDeletedBuilder.add(builder);

    //fine
    return ret;
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
bool GPU::pipeline_toVulkan (const GPUPipelineHandle handle, VkPipeline *out, VkPipelineLayout *out_layout) const
{
    sPipeline *s;
    if (pipelineList.fromHandleToPointer (handle, &s))
    {
        *out = s->vkPipelineHandle;
        *out_layout = s->vkPipelineLayoutHandle;
        return true;
    }

    gos::logger::err ("GPU::pipeline_toVulkan() => invalid handle\n");
    DBGBREAK;
    return false;
}

