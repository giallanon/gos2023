#include "gosGPU.h"
#include "../gos/string/gosStringList.h"
#include "../gos/gos.h"
#include "../gos/memory/gosAllocatorHeap.h"

using namespace gos;

//PFN_vkCmdPushDescriptorSetKHR   GPU::vkCmdPushDescriptorSetKHR = VK_NULL_HANDLE;

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
static void GOSGPU_trapOnWindowResize (int w, int h, void *userpt)
{
    GPU *gpu = reinterpret_cast<GPU*>(userpt);
    gpu->_internal__onWindowResized(w, h);
}

//********************************************************** 
bool GPU::shader_compile (const char *shaderSRCFile, const char *shaderStage, const char *spaceSeparateDefineList, const char *shaderDSTFile, bool bIncludeDebugInfo)
{
    //se esistono delle define da passare al compilatore...
    char defineList[2048];
    memset (defineList, 0, sizeof(defineList));
    if (NULL != spaceSeparateDefineList)
    {
        string::utf8::StringListParser parser;
        parser.toStart (spaceSeparateDefineList, ' ');
        
        char def[256];
        while (parser.next (def, sizeof(def)))
        {
            strcat_s (defineList, sizeof(defineList), "-D");
            strcat_s (defineList, sizeof(defineList), def);
            strcat_s (defineList, sizeof(defineList), " ");
        }
    }

    //glslc -fshader-stage=vert --target-env=vulkan1.3 lineRenderer.vert.shader -g -O -o lineRenderer.vert.spv
    char cmd[1024];
    if (bIncludeDebugInfo)
        sprintf_s (cmd, sizeof(cmd), "glslc -fshader-stage=%s --target-env=vulkan1.3 %s %s -g -O0 -o %s 2>&1",  shaderStage, defineList, shaderSRCFile, shaderDSTFile);
    else
        sprintf_s (cmd, sizeof(cmd), "glslc -fshader-stage=%s --target-env=vulkan1.3 %s %s -O -o %s 2>&1",  shaderStage, defineList, shaderSRCFile, shaderDSTFile);
    //gos::logger::log ("%s\n", cmd);

    char *result;
    u32 len;
    if (!gos::runShellScriptAndStoreResult (cmd, gos::getScrapAllocator(), &result, &len))
        return false;

    if (NULL == result)
        return true;

    //c'e' stato qualche errore di compilazione
    gos::logger::err ("ERR => %s\n", result);
    GOSFREE_SCRAP(result);
    return false;
}

//********************************************************** 
GPU::GPU()
{
    this->allocator = NULL;
    vkInstance = VK_NULL_HANDLE;
    vkSurfaceKHR = VK_NULL_HANDLE;
    vkDebugMessenger = VK_NULL_HANDLE;
    defaultViewportHandle.setInvalid();
    currentSwapChainImageIndex = 0;
    timeToRecreateSwapchain_msec = 0;
    bSwapChainRecreatedDuringThisFrame = false;
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


    //init vero e proprio
    bool bSuccess = false;
    while (1)
    {
        //if (!priv_initWindowSystem (width, height, appName))
        //    break;
        this->mainWindow.winHandle = mainWin;
        input::window_trapOn_resize (mainWin, GOSGPU_trapOnWindowResize, this);

        if (!priv_initHandleLists())
            break;
        if (!priv_initVulkan(eVulkanVersion::v1_3))
            break;
        bSuccess = true;
        break;
    }

    if (!bSuccess)
        return false;

    //formato zbuffer (cerca il meglio disponibile)
    {
        VkFormat depthStencilFormat = VK_FORMAT_UNDEFINED;
        vulkan.findBestDepthStencilFormat (&depthStencilFormat);        
        zbuffer_bestFmt_withStencil = gpu::fromVulkan(depthStencilFormat);

        depthStencilFormat = VK_FORMAT_UNDEFINED;
        vulkan.findBestDepthOnlyFormat (&depthStencilFormat);
        zbuffer_bestFmt_noStencil = gpu::fromVulkan(depthStencilFormat);
    }

    if (mainWindow.isValid())
    {
        //default viewport
        viewport_create ("0", "0", "0-", "0-", &defaultViewportHandle);
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

    if (mainWindow.isValid())
    {
        //creo una surface basata sulla [mainWindow]
        //GLFW fa tutto da solo, ma in linea di massima questa parte sarebbe dipendente da piattaforma
        GLFWwindow *glfWin = mainWindow.getGLF();
        VkResult result = glfwCreateWindowSurface(vkInstance, glfWin, nullptr, &vkSurfaceKHR);
        if (VK_SUCCESS != result)
        {
            gos::logger::err ("glfwCreateWindowSurface() returned %s\n", string_VkResult(result));
            return false;
        }
    }

    //cerco un physical device che sia appropriato
    {
        gos::StringList vkDevice_requiredExtensionList(scrapAllocator);
        if (mainWindow.isValid())
            vkDevice_requiredExtensionList.add (VK_KHR_SWAPCHAIN_EXTENSION_NAME);
        vkDevice_requiredExtensionList.add (VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME);
        vkDevice_requiredExtensionList.add (VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);
        //vkDevice_requiredExtensionList.add (VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME);
        //vkDevice_requiredExtensionList.add (VK_KHR_MAINTENANCE1_EXTENSION_NAME);
        //vkDevice_requiredExtensionList.add (VK_KHR_MAINTENANCE3_EXTENSION_NAME);

        if (!vulkanScanAndSelectAPhysicalDevices(vkInstance, vkSurfaceKHR, vkDevice_requiredExtensionList, vulkanVersion, &physicalDevInfo))
        {
            gos::logger::err ("\ncan't find a good enough vulkan device\n");
            return false;
        }
        else
        {
            gos::logger::log (eTextColor::green, "\nselected device is at index %d\n   gfxQ familyIndex=%d, count=%d\n   computeQ familyIndex=%d, count=%d\n   transferQ familyIndex=%d, count=%d\n",
                physicalDevInfo.devIndex,
                physicalDevInfo.queue_gfx.familyIndex,      physicalDevInfo.queue_gfx.count,
                physicalDevInfo.queue_compute.familyIndex,  physicalDevInfo.queue_compute.count,
                physicalDevInfo.queue_transfer.familyIndex, physicalDevInfo.queue_compute.count);
        }
        gos::logger::log("\n");


        //creazione del device logico di vulkan
        if (!vulkan.setup (physicalDevInfo, vkDevice_requiredExtensionList, vulkanVersion))
        {
            gos::logger::err ("can't create a logical device\n");
            return false;
        }
        gos::logger::log("\n");
    }

    /*
    vkCmdPushDescriptorSetKHR = (PFN_vkCmdPushDescriptorSetKHR)vkGetDeviceProcAddr(vulkan.vkDev, "vkCmdPushDescriptorSetKHR");
    if (!vkCmdPushDescriptorSetKHR) 
    {
        gos::logger::err ("Could not get a valid function pointer for vkCmdPushDescriptorSetKHR\n");
        return false;
    }
    */



    //initVulkan:: creazione swap chain
    swapchain.reset();
    if (VK_NULL_HANDLE != vkSurfaceKHR)
    {
        if (!vulkan.swapchain_create (vkSurfaceKHR, vSync, &swapchain))
        {
            gos::logger::err ("can't create swap chain\n");
            return false;
        }    
        gos::logger::log("\n");
    }

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
        vulkan.swapchain_delete(swapchain);
        vulkan.unsetup();

        if (VK_NULL_HANDLE != vkDebugMessenger)
        {
            auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(vkInstance, "vkDestroyDebugUtilsMessengerEXT");
            if (NULL != func)
                func(vkInstance, vkDebugMessenger, NULL);
        }

        if (VK_NULL_HANDLE != vkSurfaceKHR)
            vkDestroySurfaceKHR(vkInstance, vkSurfaceKHR, nullptr);

        vkDestroyInstance(vkInstance, NULL);
        vkInstance = VK_NULL_HANDLE;
    }
}    

//**********************************************************
bool GPU::priv_initHandleLists()
{
    gos::logger::log ("GPU::priv_initHandleLists()\n");
    shaderList.setup (allocator);

    viewportlList.setup (allocator);
    viewportHandleList.setup (allocator, 32);   //questa mi serve per tenere traccia di tutti gli handle creati dato che durante il resize della window, devo andare ad aggiustare tutte le viewport

    depthStencilList.setup (allocator);
    depthStencilHandleList.setup (allocator, 32);   //questa mi serve per tenere traccia di tutti gli handle creati dato che durante il resize della window, devo andare ad aggiustare tutte i swpth buffer (nel caso che siano bindati alla dimensione della vport)

    renderTargetList.setup (allocator);
    renderTargetHandleList.setup (allocator, 64);   //questa mi serve per tenere traccia di tutti gli handle creati dato che durante il resize della window, devo andare ad aggiustare tutte i rt buffer (nel caso che siano bindati alla dimensione della vport)

    pipelineList.setup (allocator);
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
    
    viewportlList.unsetup();
    viewportHandleList.unsetup();

    depthStencilList.unsetup();
    depthStencilHandleList.unsetup();

    renderTargetList.unsetup ();
    renderTargetHandleList.unsetup();

    pipelineList.unsetup();
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


//************************************
bool GPU::swapChain_acquireImage (gos::gpu::SwapchainImg *out, u64 timeout_ns, VkSemaphore semaphore, VkFence fence)
{
    assert (NULL != out);

    bSwapChainRecreatedDuringThisFrame = false;

    const u64 timeNow_msec = gos::getTimeSinceStart_msec();
    toBeDeletedBuilder.check (timeNow_msec);

    //se ho in canna un rebuild della swapchain, lo faccio ora
    if (timeToRecreateSwapchain_msec)
    {
        if (timeNow_msec >= timeToRecreateSwapchain_msec)
        {
            timeToRecreateSwapchain_msec = 0;
            priv_swapChain_recreate();
        }
        else
            return false;
    }

    u32 imageIndex;
    const VkResult result = vulkan.swapChain_acquireImage (swapchain, timeout_ns, semaphore, fence, &imageIndex);

    switch (result)
    {
    default:
        gos::logger::err ("GPU::swapChain_acquireImage() => vkAcquireNextImageKHR() => %s\n", string_VkResult(result));
        break;

    case VK_SUCCESS:
        out->imageIndex = imageIndex;
        out->image = swapchain.vkImageList[imageIndex];
        out->imageView = swapchain.vkImageListView[imageIndex];
        return true;

    case VK_SUBOPTIMAL_KHR:
        //posso ancora renderizzare, ma schedulo un rebuild della swapchain
        out->imageIndex = imageIndex;
        out->image = swapchain.vkImageList[imageIndex];
        out->imageView = swapchain.vkImageListView[imageIndex];

        if (0 == timeToRecreateSwapchain_msec)
        {
            gos::logger::log (eTextColor::yellow, "GPU::swapChain_acquireImage() => vkAcquireNextImageKHR() => %s\n", string_VkResult(result));
            timeToRecreateSwapchain_msec = timeNow_msec + 1000;
        }
        return true;

    case VK_ERROR_OUT_OF_DATE_KHR:
        priv_swapChain_recreate();
        break;

    case VK_TIMEOUT:
    case VK_NOT_READY:
        break;
    }


    out->imageIndex = u32MAX;
    out->image = VK_NULL_HANDLE;
    out->imageView = VK_NULL_HANDLE;
    return false;

}


//**********************************************************
bool GPU::priv_swapChain_recreate ()
{
    if (!mainWindow.isValid())
    {
        //non ci dovremmo mai arrivare qui
        DBGBREAK;
        return true;
    }

    bSwapChainRecreatedDuringThisFrame = true;
    swapchainAutoID++;
    gos::logger::log (eTextColor::green, "GPU::swapChain_recreate()\n");
    gos::logger::incIndent();

    int width = 0;
    int height = 0;
    GLFWwindow *glfWin = mainWindow.getGLF();
    glfwGetFramebufferSize (glfWin, &width, &height);
    while (width == 0 || height == 0) 
    {
        gos::logger::log ("windows size is weird (w=%d, h=%d), waiting...\n", width, height);

        glfwWaitEvents();
        //glfwGetWindowSize (glfWin, &width, &height);
        glfwGetFramebufferSize (glfWin, &width, &height);
    }

    //attendo che Vulkan sia in idle
    gos::logger::log ("target windows size is (w=%d, h=%d), waiting vulkan idle...\n", width, height);
    bool ret = true;
    vulkan.waitIdle();

    //distruggo la swapchain
    vulkan.swapchain_delete(swapchain);
    

    //ricreazione swap chain
    if (!vulkan.swapchain_create (vkSurfaceKHR, vSync, &swapchain))
    {
        gos::logger::err ("can't create swap chain\n");
        ret = false;
    }
    
    //attuale dimensione della vport
    const i16 vportW = (i16)swapchain.imageExtent.width;
    const i16 vportH = (i16)swapchain.imageExtent.height;

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

    //fine
    gos::logger::decIndent();
    return ret;  
}

//************************************
void  GPU::_internal__onWindowResized (int w, int h)
{
    gos::logger::log ("window resized, new size (%d,%d)\n", w,h);
    this->timeToRecreateSwapchain_msec = gos::getTimeSinceStart_msec() + 500;
}

//************************************
void  GPU::toggleFullscreen()
{
    if (!mainWindow.isValid())
        return;

    gos::logger::log (eTextColor::yellow, "toggleFullscreen\n");
    gos::logger::incIndent();

    GLFWwindow *glfWin = mainWindow.getGLF();
    GLFWmonitor *monitor = glfwGetWindowMonitor(glfWin);
    if (NULL == monitor)
    {
        //andiamo in full
        mainWindow.storeCurrentPosAndSize();
        gos::logger::log ("going full screen, current win pos and size (%d,%d) (%d,%d)\n", mainWindow.storedX, mainWindow.storedY, mainWindow.storedW, mainWindow.storedH);

        monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode *mode = glfwGetVideoMode(monitor);
        glfwSetWindowMonitor (glfWin, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
    }    
    else
    {
        //torniamo in windowed
        gos::logger::log ("going in windowed mode, current win pos and size (%d,%d) (%d,%d)\n", mainWindow.storedX, mainWindow.storedY, mainWindow.storedW, mainWindow.storedH);
        glfwSetWindowMonitor(glfWin, NULL, mainWindow.storedX, mainWindow.storedY, mainWindow.storedW, mainWindow.storedH, 0);
    }

    gos::logger::decIndent();
}

//************************************
void GPU::vsync_enable (bool b)
{
    if (vSync == b)
        return;
    vSync = b;
    timeToRecreateSwapchain_msec = gos::getTimeSinceStart_msec() + 200;
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
bool GPU::priv_shader_createFromMemory (const void *bufferIN, u32 bufferSize, eShaderType shaderType, const char *mainFnName, GPUShaderHandle *out_shaderHandle)
{
    assert (NULL != out_shaderHandle);
    
    VkShaderModule vkHandle;
    const VkResult result = vulkan.shader_create (bufferIN, bufferSize, &vkHandle);
    if (VK_SUCCESS != result)
    {
        out_shaderHandle->setInvalid();
        gos::logger::err ("GPU::priv_shader_createFromMemory() => %s\n", string_VkResult(result));
        return false;
    }

    gpu::Shader *shader = shaderList.reserve(out_shaderHandle);
    if (NULL == shader)
    {
        vulkan.shader_delete (vkHandle);
        out_shaderHandle->setInvalid();
        gos::logger::err ("GPU::priv_shader_createFromMemory() => unable to reserve a new shader handle\n");
        return false;
    }

    shader->reset();
    shader->vkHandle = vkHandle;
    shader->shaderType = shaderType;
    sprintf_s (shader->mainFnName, sizeof(shader->mainFnName), "%s", mainFnName);
    return true;
}

//************************************
void GPU::deleteResource (GPUShaderHandle &shaderHandle)
{
    gpu::Shader *shader;
    if (priv_fromHandleToPointer(shaderList, shaderHandle, &shader))
    {
        if (VK_NULL_HANDLE != shader->vkHandle)
            vulkan.shader_delete (shader->vkHandle);
        
        shader->reset();
        shaderList.release(shaderHandle);
    }
    shaderHandle.setInvalid();
}

//************************************
const gpu::Shader* GPU::getInfo (const GPUShaderHandle handle) const
{
    gpu::Shader *s;
    if (priv_fromHandleToPointer(shaderList, handle, &s))
        return s;

    gos::logger::err ("GPU::shader_geInfo() => invalid handle\n");
    return NULL;
}




/************************************************************************************************************
 * viewport
 * 
 * 
 *************************************************************************************************************/
bool GPU::viewport_create (const gos::Pos2D &x,const gos::Pos2D &y, const gos::Dim2D &w, const gos::Dim2D &h, GPUViewportHandle *out_handle)
{
    if (!mainWindow.isValid())
    {
        if (x.isRelative() || y.isRelative() || w.isRelative() || h.isRelative())
        {
            //non essendoci una mainWindow, non posso usare dimensioni relative
            DBGBREAK;
            return false;
        }
    }

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

    if (mainWindow.isValid())
    {
        int width, height;
        mainWindow.getCurrentSize (&width, &height);
        v->resolve ((i16)width, (i16)height);
    }
    else
        v->resolve (0,0);
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
const gpu::Viewport* GPU::getInfo (const GPUViewportHandle &handle) const
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
bool GPU::renderTarget_create (const gos::Dim2D &dimx, const gos::Dim2D &dimy, eImageFormat fmt, eMemAccessMode memAccessMode, GPURenderTargetHandle *out_handle)
{
    assert (NULL != out_handle);
    out_handle->setInvalid();

	if (eImageFormat::_SAME_AS_CURRENT_SWAPCHAIN == fmt)
		fmt = this->swapChain_getImageFormat();

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
    rt->memAccessMode = memAccessMode;
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
    rt.resolve ((i16)swapchain.getWidth(), (i16)swapchain.getHeight());

    //chiedo a Vulkan di creare img
    if (!vulkan.image_create2D (rt.resolvedW, rt.resolvedH, 1, rt.format, rt.memAccessMode, rt.usage, &rt.image, &rt.vkMemHandle, &rt.memoryAllocated))
    {
        gos::logger::err ("GPU::priv_renderTarget_createFromStruct() => failed\n");
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

	const VkResult result = vulkan.imageView_create (imageViewCI, &rt.view);
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
        vulkan.imageView_delete (rt.view);
        rt.view = VK_NULL_HANDLE;
    }
    
    if (VK_NULL_HANDLE != rt.image)
    {
        vulkan.image_delete (rt.image, rt.vkMemHandle, rt.memoryAllocated);
        rt.image = VK_NULL_HANDLE;
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

//************************************
bool GPU::map (const GPURenderTargetHandle handle, gpu::sMappedImage *out) const
{
    assert (NULL != out);
    const gpu::RenderTarget *s = getInfo (handle);
    if (NULL == s)
    {
        gos::logger::err ("GPU::render_target::map() => invalid handle\n");
        return false;
    }

    memset (out, 0, sizeof(gpu::sMappedImage));

    if (eMemAccessMode::readback != s->memAccessMode)
    {
        gos::logger::err ("GPU::render_target::map() => invalid mem accesso mode. Buffer mode must be [readback], current mode is %s\n", gpu::enumToString(s->memAccessMode));
        return false;
    }

    // Get layout of the image (including row pitch)
    VkImageSubresource subResource { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0 };
    VkSubresourceLayout subResourceLayout;
    vulkan.image_getSubresourceLayout (s->image, &subResource, &subResourceLayout);

    // Map image memory so we can start copying from it
    VkResult result = vulkan.memory_map (s->vkMemHandle, (u32)subResourceLayout.offset, (u32)subResourceLayout.size, 0, &out->host_image_pt);
    if (VK_SUCCESS != result)
    {
        out->host_image_pt = NULL;
        gos::logger::err ("GPU::render_target::map() => vkMapMemory() => %s\n", string_VkResult(result));
        return false;
    }

    out->size = subResourceLayout.size;
    out->offset = static_cast<u32>(subResourceLayout.offset);
    out->row_stride = static_cast<u32>(subResourceLayout.rowPitch);
    out->_vkMemHandle = s->vkMemHandle;
    return true;
}

/************************************************************************************************************
 * zbuffer
 * 
 * 
 *************************************************************************************************************/
bool GPU::zbuffer_create (const gos::Dim2D &widthIN, const gos::Dim2D &heightIN, eImageFormat fmt, eMemAccessMode memAccessMode, GPUZBufferHandle *out_handle)
{
    assert (NULL != out_handle);

    if (eImageFormat::_DEPTH_BEST == fmt)
        fmt = this->zbuffer_getBestFormat();

    if (!utils::isFormatWithDepth(fmt))
    {
        gos::logger::err ("GPU::zbuffer_create() => invalid depth format (%s). Must be a valid 'DEPTH_something'\n", utils::enumToString(fmt));
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
    depthStencil->depthFormat = gpu::toVulkan(fmt);
    depthStencil->bHasStencil = utils::isFormatWithStencil(fmt);

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
void GPU::deleteResource (GPUZBufferHandle &handle)
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
    depthStencil.resolve ((i16)swapchain.getWidth(), (i16)swapchain.getHeight());


    if (!vulkan.image_create2D (depthStencil.resolvedW, depthStencil.resolvedH, 1, depthStencil.depthFormat, eMemAccessMode::onGPU, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, &depthStencil.image, &depthStencil.vkMemHandle, &depthStencil.memoryAllocated))
    {
        gos::logger::err ("GPU::priv_depthStenicl_createFromStruct() => failed\n");
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

    VkResult result = vulkan.imageView_create (imageViewCI, &depthStencil.view);
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
        vulkan.imageView_delete (depthStencil.view);
        depthStencil.view = VK_NULL_HANDLE;
    }
    
    if (VK_NULL_HANDLE != depthStencil.image)
    {
        vulkan.image_delete (depthStencil.image, depthStencil.vkMemHandle, depthStencil.memoryAllocated);
        depthStencil.image = VK_NULL_HANDLE;
        depthStencil.vkMemHandle = VK_NULL_HANDLE;
    }
}

//************************************
const gpu::DepthStencil* GPU::getInfo (const GPUZBufferHandle handle) const
{
    gpu::DepthStencil *s;
    if (priv_fromHandleToPointer (depthStencilList, handle, &s))
        return s;
    return NULL;
}




/************************************************************************************************************
 * Command buffer
 * 
 * 
 *************************************************************************************************************/
bool GPU::cmdBuffer_create (eGPUQueueFamily whichQ, GPUCmdBufferHandle *out_handle, u32 threadID)
{
    assert (NULL != out_handle);
    out_handle->setInvalid();

    VkCommandPool   vkPool;
    VkCommandBuffer vkCmdBufferHandle;
    if (!vulkan.commandBuffer_create (whichQ, threadID, &vkPool, &vkCmdBufferHandle))
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
    s->vkPool = vkPool;
    s->whichQ = whichQ;
    return true;    
}

//************************************
void GPU::deleteResource (GPUCmdBufferHandle &handle)
{
    gpu::CommandBuffer *s;
    if (cmdBufferList.fromHandleToPointer (handle, &s))
    {
        vulkan.commandBuffer_delete (s->whichQ, s->vkPool, s->vkHandle);
        s->reset();
        cmdBufferList.release (handle);
    }

    handle.setInvalid();
}

//************************************
const gpu::CommandBuffer* GPU::getInfo (const GPUCmdBufferHandle handle) const
{
    gpu::CommandBuffer *s;
    if (cmdBufferList.fromHandleToPointer(handle, &s))
        return s;
    gos::logger::err ("GPU::cmdBuffer_getInfo() => invalid handle\n");
    return NULL;
}



//************************************************************************************************************
bool GPU::priv_bufferCreate (VkBufferUsageFlags vkUsage, u32 sizeInByte, bool bCanBeUsedBy_gfxQ, bool bCanBeUsedBy_computeQ, bool bCanBeUsedBy_transferQ, eMemAccessMode mode, gpu::Buffer *out)
{
    VkMemoryPropertyFlags vkMemProperties;
    switch (mode)
    {
    default:
        gos::logger::err ("GPU::priv_bufferCreate() => invalid mode %d => '%s' \n", mode, gpu::enumToString(mode));
        return false;
        break;

    case eMemAccessMode::onGPU:
        vkMemProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        break;

    case eMemAccessMode::shared_cpuW_autoSync:
        vkMemProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        break;

    case eMemAccessMode::shared_cpuW_manualSync:
        vkMemProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        break;
    
    } 


    //chiedo a Vulkan di creare il buffer
    VkBuffer        vkHandle;
    VkDeviceMemory  vkMemHandle = VK_NULL_HANDLE;
    u32 realMemAllocated;
    if (!vulkan.buffer_create (sizeInByte, 
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
    case eMemAccessMode::shared_cpuW_manualSync:
    case eMemAccessMode::onGPU:
        break;

    case eMemAccessMode::shared_cpuW_autoSync:
        //mappo la memoria del buffer direttamente qui, visto che questo buffer e' sempre HOST_MAPPABLE e COHERENT
        {
            void *mappedPt;
            VkResult result = vulkan.memory_map (out->_vkMemHandle, 0, u32MAX, 0, &mappedPt);
            if (VK_SUCCESS != result)
            {
                gos::logger::err ("GPU::priv_bufferCreate() => failed vkMapMemory() => %s\n", string_VkResult(result));
                vulkan.buffer_delete (out->vkHandle, out->_vkMemHandle, out->memoryAllocated);
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
void GPU::image_unmap (gpu::sMappedImage &m)
{
    if (NULL != m.host_image_pt)
    {
        vulkan.memory_unmap (m._vkMemHandle);
        memset (&m ,0, sizeof(gpu::sMappedImage));
    }
}

//************************************************************************************************************
void GPU::image_manualSync_cpuRead (const gpu::sMappedImage *list, u32 numElemInList)
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

    vulkan.memory_invalidateRanges (numElemInList, flush_range);
}

//************************************************************************************************************
void GPU::buffer_unmap (gpu::sMappedBuffer &m)
{
    if (NULL != m.host_pt)
    {
        vulkan.memory_unmap(m._vkMemHandle);
        memset (&m ,0, sizeof(gpu::sMappedBuffer));
    }
}

//************************************************************************************************************
void GPU::buffer_manualSync_cpuWrite (const gpu::sMappedBuffer &mapped_buffer, u32 offset, u32 size)
{
    VkMappedMemoryRange flush_range;
    flush_range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    flush_range.pNext = NULL;
    flush_range.memory = mapped_buffer._vkMemHandle;
    flush_range.offset = offset;

    if (u32MAX == size)
        flush_range.size = VK_WHOLE_SIZE;
    else
        flush_range.size = size;

    vulkan.memory_flushRanges (1, &flush_range);
}

//************************************************************************************************************
void GPU::buffer_manualSync_cpuRead (const gpu::sMappedBuffer &mapped_buffer, u32 offset, u32 size)
{
    VkMappedMemoryRange flush_range;
    flush_range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    flush_range.pNext = NULL;
    flush_range.memory = mapped_buffer._vkMemHandle;
    flush_range.offset = offset;
    flush_range.size = size;

    vulkan.memory_invalidateRanges (1, &flush_range);
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

    if (!priv_bufferCreate (VK_BUFFER_USAGE_TRANSFER_SRC_BIT, sizeInByte, false, false, false, eMemAccessMode::shared_cpuW_autoSync, s))
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
const gpu::Buffer* GPU::getInfo (const GPUStgBufferHandle handle) const
{
    gpu::Buffer *s;
    if (priv_fromHandleToPointer(staginBufferList, handle, &s))
        return s;

    gos::logger::err ("GPU::stageBuffer_getInfo() => invalid handle\n");
    return NULL;
}

//************************************
bool GPU::stagingBuffer_memcpy (GPUStgBufferHandle &handleDST, u32 offsetDST, const void *dataSRC, u32 sizeof_dataSRC)
{
    gpu::Buffer *dst;
    if (!priv_fromHandleToPointer(staginBufferList, handleDST, &dst))
    {
        gos::logger::err ("GPU::stagingBuffer_memcpy() => invalid handleDST\n");
        return false;
    }

    if (offsetDST + sizeof_dataSRC > dst->mapped_size)
    {
        DBGBREAK;
        return false;
    }
    memcpy (&dst->mapped_host_pt[offsetDST], dataSRC, sizeof_dataSRC);
    return true;
}

/************************************************************************************************************
 * Vertex buffer
 * 
 * 
 *************************************************************************************************************/
bool GPU::vertexBuffer_create (u32 sizeInByte, eMemAccessMode modeIN, GPUVtxBufferHandle *out_handle)
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
    if (modeIN == eMemAccessMode::onGPU)
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
const gpu::Buffer* GPU::getInfo (const GPUVtxBufferHandle handle) const
{
    gpu::Buffer *s;
    if (priv_fromHandleToPointer(vtxBufferList, handle, &s))
        return s;

    gos::logger::err ("GPU::vertexBuffer_getInfo() => invalid handle\n");
    return NULL;
}



/************************************************************************************************************
 * Index buffer
 * 
 * 
 *************************************************************************************************************/
bool GPU::indexBuffer_create (u32 sizeInByte, eMemAccessMode modeIN, GPUIdxBufferHandle *out_handle)
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
    if (modeIN == eMemAccessMode::onGPU)
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
const gpu::Buffer* GPU::getInfo (const GPUIdxBufferHandle handle) const
{
    gpu::Buffer *s;
    if (priv_fromHandleToPointer(idxBufferList, handle, &s))
        return s;
    gos::logger::err ("GPU::indexBuffer_getInfo() => invalid handle\n");
    return NULL;
}


/************************************************************************************************************
 * uniform buffer
 * 
 * 
 *************************************************************************************************************/
bool GPU::uniformBuffer_create (u32 sizeInByte, eMemAccessMode modeIN, GPUUniformBufferHandle *out_handle)
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
    if (modeIN == eMemAccessMode::onGPU)
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
const gpu::Buffer* GPU::getInfo (const GPUUniformBufferHandle handle) const
{
    gpu::Buffer *s;
    if (priv_fromHandleToPointer (uniformBufferList, handle, &s))
        return s;
    gos::logger::err ("GPU::uniformBuffer_getInfo() => invalid handle\n");
    return NULL;
}




/************************************************************************************************************
 * storage buffer
 * 
 * 
 *************************************************************************************************************/
bool GPU::storageBuffer_create (u32 sizeInByte, eMemAccessMode modeIN, GPUStorageBufferHandle *out_handle)
{
    assert (NULL != out_handle);
    out_handle->setInvalid();

    if (sizeInByte > vulkan.limits_get_maxStorageBufferRange())
    {
        gos::logger::err ("GPU::storageBuffer_create() => too big. Trying to allocate %d when max is %d\n", sizeInByte, vulkan.limits_get_maxStorageBufferRange());
        return false;
    }

    gpu::Buffer *s = storageBufferList.reserve (out_handle);
    if (NULL == s)
    {
        gos::logger::err ("GPU::storageBuffer_create() => can't reserve a handle!\n");
        return false;
    }
    
    VkBufferUsageFlags vkUsage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    if (modeIN == eMemAccessMode::onGPU)
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
const gpu::Buffer* GPU::getInfo (const GPUStorageBufferHandle handle) const
{
    gpu::Buffer *s;
    if (priv_fromHandleToPointer (storageBufferList, handle, &s))
        return s;
    gos::logger::err ("GPU::storageBuffer_getInfo() => invalid handle\n");
    return NULL;
}





/************************************************************************************************************
 * DescriptorSet Layout
 * 
 * 
 *************************************************************************************************************/
void GPU::deleteResource (GPUDescrSetLayoutHandle &handle)
{
    gpu::DescrSetLayout *s;
    if (descrSetLayoutList.fromHandleToPointer (handle, &s))
    {
        vulkan.descSetLayout_delete (s->vkHandle);
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
    VkResult result = vulkan.descPool_create (creatInfo, &vkHandle);
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
        vulkan.descPool_delete (s->vkHandle);
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
bool GPU::descrSetInstance_create (const GPUDescrPoolHandle &poolHandle, const GPUDescrSetLayoutHandle &descrSetLayoutHandle, GPUDescrSetInstanceHandle *out_handle)
{
    assert (NULL != out_handle);
    out_handle->setInvalid();

    gpu::DescrPool *pool;
    if (!descrPoolList.fromHandleToPointer (poolHandle, &pool))
    {
        gos::logger::err ("GPU::descrSetInstance_create() => invalid pool handle\n");
        return false;
    }


    VkDescriptorSetLayout vkDescSetLayoutHandle;
    if (!toVulkan (descrSetLayoutHandle, &vkDescSetLayoutHandle))
    {
        gos::logger::err ("GPU::descrSetInstance_create() => invalid descrSetLayoutHandle handle\n");
        return false;
    }


    gpu::DescrSetInstance *s = descrSetInstanceList.reserve (out_handle);
    if (NULL == s)
    {
        gos::logger::err ("GPU::descrSetInstance_create() => can't reserve a handle!\n");
        descrSetInstanceList.release(*out_handle);
        return false;
    }

    s->reset();
    s->vkPoolHandle = pool->vkHandle;
    s->bCanBeFreed = ( (pool->flags & VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT) != 0 );

    const VkResult result = vulkan.descriptorSet_create (pool->vkHandle, vkDescSetLayoutHandle, &s->vkHandle);
    if (VK_SUCCESS == result)
        return true;


    s->reset();
    descrSetInstanceList.release(*out_handle);
    gos::logger::err ("GPU::descrSetInstance_create () => vkAllocateDescriptorSets failed => %s\n", string_VkResult(result));
    return false;
}

//************************************
void GPU::deleteResource (GPUDescrSetInstanceHandle &handle)
{
    gpu::DescrSetInstance *s;
    if (descrSetInstanceList.fromHandleToPointer (handle, &s))
    {
        if (s->bCanBeFreed)
            vulkan.descriptorSet_delete (s->vkPoolHandle, s->vkHandle);
        s->reset();
        descrSetInstanceList.release (handle);
    }

    handle.setInvalid();
}

//************************************
const gpu::DescrSetInstance* GPU::getInfo (const GPUDescrSetInstanceHandle handle) const
{
    gpu::DescrSetInstance *s;
    if (priv_fromHandleToPointer (descrSetInstanceList, handle, &s))
        return s;
    gos::logger::err ("GPU::descrSetInstance_getInfo() => invalid handle\n");
    return NULL;
}




/************************************************************************************************************
 * Texture
 * 
 * 
 *************************************************************************************************************/
bool GPU::texture_create2D (u16 dimx, u16 dimy, u8 nMipMap, eImageFormat fmt, eMemAccessMode memAccessMode, const void *srcDATA, GPUTextureHandle *out_handle, gpu::StageHelper &helper)
{
    assert (NULL != out_handle);
    assert (nMipMap >= 1);
    out_handle->setInvalid();


    //chiedo a Vulkan di creare img
    VkImage         vkImageHandle;
    VkDeviceMemory  vkMemHandle = VK_NULL_HANDLE;
    u32             imageMemSize = 0;

    if (!vulkan.image_create2D (dimx, dimy, nMipMap, gos::gpu::toVulkan(fmt), memAccessMode, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, &vkImageHandle, &vkMemHandle, &imageMemSize))
    {
        gos::logger::err ("GPU::texture_create2D() => failed\n");
        return false;
    }

    if (NULL != srcDATA)
    {
        //L'immagine appena creata ha il layout VK_IMAGE_LAYOUT_UNDEFINED
        //Per poterci copiare dentro srcDATA, devo trasformarla in VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
		helper.begin()
			.imageTransition (vkImageHandle, eImageLayout::undefined, eImageLayout::transfer_dst)
			.mem_to_stgBuffer (srcDATA, imageMemSize, NULL);

#ifdef _DEBUG
        {
            u32 totalImgSize = 0;
            u32 w = dimx;
            u32 h = dimy;
            for (u8 i=0; i<nMipMap; i++)
            {
                totalImgSize += utils::getFormatSize(fmt) * w * h;
                w/=2;
                h/=2;
            }
            assert (totalImgSize <= imageMemSize);
        }
#endif

        //una volta che immagine è in stato VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, ci posso copiare dentro il contenuto dello stgBuffer
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

                offset += w * h * utils::getFormatSize(fmt);
                w/=2;
                h/=2;
            }

			const gpu::CommandBuffer *vkCmdBuffer = getInfo(helper.get_cmdBuffer_handle());
			const gpu::Buffer *stgBuffer = getInfo (helper.get_stagBuffer_handle());
            vkCmdCopyBufferToImage(
                vkCmdBuffer->vkHandle,
                stgBuffer->vkHandle,
                vkImageHandle,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                nMipMap,
                regionList
            );        
        }

        //infine, devo transizionare l'immagine da VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL a VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        helper.imageTransition (vkImageHandle, eImageLayout::transfer_dst, eImageLayout::shader_readonly);
        helper.submit();
    }

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


    VkResult result = vulkan.imageView_create (viewInfo, &s->view);
    if (VK_SUCCESS != result)
    {
        gos::logger::err ("GPU::texture_create2D () => vkCreateImageView failed => %s\n", string_VkResult(result));
        return false;
    }

    return true;
}

bool GPU::texture_create2D (const gos::Image *im, u8 srcTextureNum, eMemAccessMode memAccessMode, GPUTextureHandle *out_handle, gpu::StageHelper &helper)
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

    return texture_create2D (header->width, header->height, header->numMipMap, header->fmt, memAccessMode, texData.textureData, out_handle, helper);
}

void GPU::deleteResource (GPUTextureHandle &handle)
{
    gpu::Texture *s;
    if (textureList.fromHandleToPointer (handle, &s))
    {
        if (VK_NULL_HANDLE != s->view)
            vulkan.imageView_delete (s->view);

        if (VK_NULL_HANDLE != s->vkHandle)
            vulkan.image_delete (s->vkHandle, s->vkMemHandle, s->memoryAllocated);

        s->reset();
        textureList.release (handle);
    }


    handle.setInvalid();
}

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

const gpu::Texture* GPU::getInfo (const GPUTextureHandle handle) const
{
    gpu::Texture *s;
    if (priv_fromHandleToPointer(textureList,handle, &s))
        return s;

    gos::logger::err ("GPU::texture_getInfo() => invalid handle\n");
    return NULL; 
}


/************************************************************************************************************
 * sampler
 * 
 * 
 *************************************************************************************************************/
bool GPU::sampler_create (const gpu::SamplerDesc &desc, GPUSamplerHandle *out_handle)
{
    //prima di tutto cerco se esiste gia' un sampler con gli stessi parametri
    FastHashMap<u32, GPUSamplerHandle>::Position insertPosition;
    if (samplerDescrHashMap.findWithPos (desc.toU32(), out_handle, & insertPosition))
        return true;


    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

    samplerInfo.minFilter = gos::gpu::toVulkan(desc.minFilter);
    samplerInfo.magFilter = gos::gpu::toVulkan(desc.magFilter);
    if (desc.bAnisotropic)
    {
        samplerInfo.anisotropyEnable = VK_TRUE;
        samplerInfo.maxAnisotropy = (f32)vulkan.limits_get_maxSamplerAnisotropy();
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
    VkResult result = vulkan.sampler_create (samplerInfo, &vkHandle);
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
            vulkan.sampler_delete (s->vkHandle);

        s->reset();
        samplerList.release (handle);
    }


    handle.setInvalid();
}

//************************************
const gpu::Sampler* GPU::getInfo (const GPUSamplerHandle handle) const
{
    gpu::Sampler *s;
    if (samplerList.fromHandleToPointer (handle, &s))
        return s;

    gos::logger::err ("GPU::sampler_geInfo() => invalid handle\n");
    return NULL;
}



/************************************************************************************************************
 * pipeline_v2_
 * 
 * 
 *************************************************************************************************************/
bool GPU::descrSetLayout_create (const gpu::Pipeline_def::DescriptorSet &ds, GPUDescrSetLayoutHandle *out_handle)
{
    VkDescriptorSetLayout vkHandle;
    return priv_descrSetLayout_build_v2 (ds, out_handle, &vkHandle);
}

bool GPU::priv_descrSetLayout_build_v2 (const gpu::Pipeline_def::DescriptorSet &ds, GPUDescrSetLayoutHandle *out_handle, VkDescriptorSetLayout *out_vkHandle)
{
    //TODO: cachare i descriptor-set ed eventualmente riutilizzarli visto che sono dei descrittori, non e' necessario
    //      crearne N diversi che descrivono la stessa cosa

    //elenco dei descrittori all'interno del set
    VkDescriptorSetLayoutBinding    bindingList[GOSGPU__NUM_MAX_DESCRIPTOR_PER_SET];

    memset (bindingList, 0, sizeof(bindingList));
    for (u32 i=0; i<ds.numDescriptor; i++)
    {
        bindingList[i].binding = ds.list[i].binding;
        bindingList[i].stageFlags = ds.list[i].usageBitmask.bitmask;
        bindingList[i].descriptorCount = ds.list[i].count;

        switch (ds.list[i].descrType)
        {
        default:                                                return false;;
        case eGPUDescriptrorType::UNIFORM_BUFFER:               bindingList[i].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; break;
        case eGPUDescriptrorType::DYNAMIC_UNIFORM_BUFFER:       bindingList[i].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC; break;
        case eGPUDescriptrorType::STORAGE_BUFFER:               bindingList[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; break;
        case eGPUDescriptrorType::DYNAMIC_STORAGE_BUFFER:       bindingList[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC; break;
        case eGPUDescriptrorType::COMBINED_IMAGE_SAMPLER:       bindingList[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; break;
        case eGPUDescriptrorType::SAMPLER:                      bindingList[i].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER; break;
        case eGPUDescriptrorType::TEXTURE2D:                    bindingList[i].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE; break;
        }
    }


    //creazione del descriptor-set
    VkDescriptorSetLayoutCreateInfo creatInfo{};

    memset (&creatInfo, 0, sizeof(creatInfo));
    creatInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    creatInfo.flags = ds.flag;
    creatInfo.bindingCount = ds.numDescriptor;
    creatInfo.pBindings = bindingList;


    //opzioni addizionali
    VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsCreateInfo {};
    VkDescriptorBindingFlags bindingFlags[GOSGPU__NUM_MAX_DESCRIPTOR_PER_SET];
    if ((creatInfo.flags & VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT) != 0)
    {
        memset (bindingFlags, 0, sizeof(bindingFlags));
        bindingFlagsCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
        bindingFlagsCreateInfo.bindingCount = creatInfo.bindingCount;
        bindingFlagsCreateInfo.pBindingFlags = bindingFlags;

        creatInfo.pNext = &bindingFlagsCreateInfo;

        for (u32 i=0; i<creatInfo.bindingCount; i++)
            bindingFlags[i] = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT;
    }

    //chiamo vulkan
    VkResult result = vulkan.descSetLayout_create (creatInfo, out_vkHandle);
    if (VK_SUCCESS != result)
    {
        gos::logger::err ("GPU::priv_descrSetLayout_build_v2 () => vkCreateDescriptorSetLayout failed => %s\n", string_VkResult(result));
        return false;
    }

    //alloco il mio handle interno
    gpu::DescrSetLayout *s = descrSetLayoutList.reserve (out_handle);
    if (NULL == s)
    {
        gos::logger::err ("GPU::priv_descrSetLayout_build_v2() => can't reserve a handle!\n");
        return false;
    }
    s->reset();
    s->vkHandle = *out_vkHandle;
    return true;
}

/************************************************************************************************************
 * Pipeline
 * 
 * 
 *************************************************************************************************************/
void GPU::deleteResource (GPUPipelineHandle &handle)
{
    gpu::Pipeline2 *s;
    if (pipelineList.fromHandleToPointer (handle, &s))
    {
        vulkan.pipelineLayout_delete (s->vkPipelineLayoutHandle);
        vulkan.pipeline_delete (s->vkPipelineHandle);
        for (u32 i=0; i<s->descrset_num; i++)
            deleteResource (s->descrset_handle_defList[i]);
        s->reset();
        pipelineList.release (handle);
    }

    handle.setInvalid();
}

//************************************
const gpu::Pipeline2* GPU::getInfo (const GPUPipelineHandle handle) const
{
    gpu::Pipeline2 *s;
    if (pipelineList.fromHandleToPointer (handle, &s))
        return s;
    gos::logger::err ("GPU::pipeline_geInfo() => invalid handle\n");
    return NULL;
}


//************************************
bool GPU::pipeline_createNew (const gpu::Pipeline_def &rpd, GPUPipelineHandle *out_handle)
{
    assert (NULL != out_handle);
    out_handle->setInvalid();


    //riservo un handle
    gpu::Pipeline2 *s = pipelineList.reserve (out_handle);
    if (NULL == s)
    {
        gos::logger::err ("GPU::pipeline_createNew() => can't reserve a handle!\n");
        return false;
    }
    s->reset();

    if (!priv_pipeline2_doCreate(rpd, s))
    {
        gos::logger::err ("GPU::pipeline_createNew() => error creating the pipe!\n");
        pipelineList.release(*out_handle);
        out_handle->setInvalid();
        return false;
    }

    return true;
}

//************************************
bool GPU::priv_pipeline2_doCreate (const gpu::Pipeline_def &rpd, gpu::Pipeline2 *out)
{
    assert (NULL != out);
    out->reset();
    

#ifdef _DEBUG
    if (0 == rpd.numRT && !rpd.zbuffer_is_enabled())
    {
        //non hai definito nemmeno 1 rt e nemmeno lo ZB, mi sa che e' un errore
        DBGBREAK;
    }
#endif

    VkResult result;
    VkGraphicsPipelineCreateInfo pipelineCreateInfo;
    memset (&pipelineCreateInfo, 0, sizeof(pipelineCreateInfo));
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
    pipelineCreateInfo.basePipelineIndex = -1; // Optional
    
    //Pipeline layout
    //Serve ad indicare il numero/tipo di "const push" disponibili alla pipe e il numero/tipo di descriptorSets della pipe
    pipelineCreateInfo.layout = VK_NULL_HANDLE;
    {
        //in pipeline_def ho dichiarato una serie di pushConst indicandone l'offeset e lo shader di appartenenza
        //In base a questi dati, devo creare una serie di pushConstantRange, uno per ogni tipo diverso di shader
        struct PCRange
        {
            VkShaderStageFlags    stageFlags;
            u32 start_offset;
            u32 end_offset;
        };
        PCRange pcRangeList[GOSGPU__NUM_MAX_PUSH_CONSTANT_RANGE_PER_PIPELINE];
        for (u32 i=0; i<GOSGPU__NUM_MAX_PUSH_CONSTANT_RANGE_PER_PIPELINE; i++)
        {
            pcRangeList[i].stageFlags = 0;
            pcRangeList[i].start_offset = u32MAX;
            pcRangeList[i].end_offset = 0;
        }

        for (u32 i=0; i< rpd.numPushConst; i++)
        {
            const eShaderTypeBitmask shaderTypeBitmask = rpd.pushConstList[i].shaderTypeBitmask;

            //computo gli stageFlags di questa pushConst
            VkShaderStageFlags stageFlags = 0;
            {
                u8 iter;
                eShaderType shaderType;
                shaderTypeBitmask.beginFetch (&iter);
                while (shaderTypeBitmask.fetch(iter, &shaderType))
                {
                    switch (shaderType)
                    {
                    default:
                        gos::logger::err ("GPU::pipeline_createNew() => invalid shaderType for pushContant\n");
                        return false;
                    // case eShaderType::vtxShader: stageFlags = VK_SHADER_STAGE_VERTEX_BIT; break;
                    // case eShaderType::pxlShader: stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT; break;
                    case eShaderType::vtxShader: 
                    case eShaderType::pxlShader:
                        stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
                        break;

                    case eShaderType::compute:   stageFlags = VK_SHADER_STAGE_COMPUTE_BIT; break;
                    }                    
                }
            }

            //in base agli <stageFlags>, determino a quale range appartiene
            u32 whichRange = 0xff;
            for (u32 i2=0; i2<GOSGPU__NUM_MAX_PUSH_CONSTANT_RANGE_PER_PIPELINE; i2++)
            {
                if (pcRangeList[i2].stageFlags == stageFlags)
                {
                    whichRange = i2;
                    break;
                }

                if (0 == pcRangeList[i2].stageFlags)
                {
                    pcRangeList[i2].stageFlags = stageFlags;
                    whichRange = i2;
                    break;
                }                
            }
            if (0xFF == whichRange)
            {
                gos::logger::err ("GPU::pipeline_createNew() => too many ranges for pushContant\n");
                return false;
            }

            //aggiorno le info di questa pushConst
            const u32 size = rpd.pushConstList[i].sizeInByte;
            u32 offset = rpd.pushConstList[i].offset;

            out->pcList[i].offset = offset;
            out->pcList[i].size = size;
            out->pcList[i].whichRange = (u8)whichRange;

            //aggiorno gli offset min/max del range
            if (pcRangeList[whichRange].start_offset > offset)
                pcRangeList[whichRange].start_offset = offset;

            offset += size;
            if (pcRangeList[whichRange].end_offset < offset)
                pcRangeList[whichRange].end_offset = offset;
        }

        //riporto le info sui range in <out>
        out->pcRange_num = 0;
        for (u32 i=0; i<GOSGPU__NUM_MAX_PUSH_CONSTANT_RANGE_PER_PIPELINE; i++)
        {
            if (u32MAX == pcRangeList[i].start_offset)
                continue;

            out->pcRange_list[out->pcRange_num].offset = pcRangeList[i].start_offset;
            out->pcRange_list[out->pcRange_num].size = pcRangeList[i].end_offset - pcRangeList[i].start_offset;
            out->pcRange_list[out->pcRange_num].stageFlags = pcRangeList[i].stageFlags;
            out->pcRange_num++;
        }


        VkPipelineLayoutCreateInfo  pipelineLayoutInfo{};
        VkDescriptorSetLayout       descrSetLayoutHandleList[GOSGPU__NUM_MAX_DESCRIPTOR_SETS*GOSGPU__NUM_MAX_DESCRIPTOR_PER_SET];
        
        memset (&pipelineLayoutInfo, 0, sizeof(pipelineLayoutInfo));
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.pushConstantRangeCount = out->pcRange_num;
        pipelineLayoutInfo.pPushConstantRanges = out->pcRange_list;
        pipelineLayoutInfo.setLayoutCount = rpd.numDescrSet;
        pipelineLayoutInfo.pSetLayouts = descrSetLayoutHandleList;


        for (u32 i=0; i<rpd.numDescrSet; i++)
        {
            out->descrset_num++;
            if (!priv_descrSetLayout_build_v2 (rpd.descriptorSetList[i], &out->descrset_handle_defList[i], &descrSetLayoutHandleList[i]))
            {
                gos::logger::err ("GPU::pipeline_createNew() => error creating descriptset\n");
                return false;
            }
        }

        result = vulkan.pipelineLayout_create (pipelineLayoutInfo, &pipelineCreateInfo.layout);
        if (VK_SUCCESS != result)
        {
            gos::logger::err ("GPU::pipeline_createNew() => vkCreatePipelineLayout() => %s\n", string_VkResult(result));
            return false;
        }
    }


    //shader
    VkPipelineShaderStageCreateInfo shadersCreateInfoArray[GOSGPU__NUM_MAX_SHADER_PER_PIPELINE] = {};
    pipelineCreateInfo.stageCount = rpd.numShader;
    pipelineCreateInfo.pStages = shadersCreateInfoArray;
    {
        memset (shadersCreateInfoArray, 0, sizeof(shadersCreateInfoArray));
        for (u32 i=0; i<rpd.numShader; i++)
        {
            const GPUShaderHandle handle = rpd.shaderHandleList[i];            
            shadersCreateInfoArray[i].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;

            const gpu::Shader *shader = getInfo(handle);
            switch (shader->shaderType)
            {
            default:
                gos::logger::err ("GPU::pipeline_createNew => unsupported shader type\n");
                DBGBREAK;
                return false;
                break;

            case eShaderType::vtxShader:
                shadersCreateInfoArray[i].stage = VK_SHADER_STAGE_VERTEX_BIT;
                break;

            case eShaderType::pxlShader:
                shadersCreateInfoArray[i].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
                break;
            
            case eShaderType::compute:
                shadersCreateInfoArray[i].stage = VK_SHADER_STAGE_COMPUTE_BIT;
                break;        
            }
            
            shadersCreateInfoArray[i].module = shader->vkHandle;
            shadersCreateInfoArray[i].pName = shader->mainFnName;
        }
    }


    //VtxDeclaration
    VkPipelineVertexInputStateCreateInfo vkVertexInputStateCreateInfo;
    VkVertexInputBindingDescription vxtBindingDescrList[GOSGPU__NUM_MAX_VXTDECL_STREAM];
    VkVertexInputAttributeDescription vtxAttributeDescrList[GOSGPU__NUM_MAX_VXTDECL_STREAM * GOSGPU__NUM_MAX_VTXDECL_ATTR];
    pipelineCreateInfo.pVertexInputState = &vkVertexInputStateCreateInfo;
    {
        memset (&vkVertexInputStateCreateInfo, 0, sizeof(vkVertexInputStateCreateInfo));
        vkVertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        out->vtx_numStream = static_cast<u8>(rpd.numVtxStream);
        if (0 == rpd.numVtxStream)
        {
            vkVertexInputStateCreateInfo.vertexBindingDescriptionCount = 0;
            vkVertexInputStateCreateInfo.pVertexBindingDescriptions = nullptr; // Optional
            vkVertexInputStateCreateInfo.vertexAttributeDescriptionCount = 0;
            vkVertexInputStateCreateInfo.pVertexAttributeDescriptions = nullptr; // Optional
        }
        else
        {
            u32 totNumAttributeDescr = 0;
            for (u32 i=0; i<rpd.numVtxStream; i++)
            {
                vxtBindingDescrList[i].binding = i;

                if (eVtxStreamInputRate::perInstance == rpd.vtxStreamList[i].inputRate)
                    vxtBindingDescrList[i].inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;
                else
                    vxtBindingDescrList[i].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

                vxtBindingDescrList[i].stride = rpd.vtxStreamList[i].calcStride();
                out->vtx_stridePerStream[i] = static_cast<u8>( vxtBindingDescrList[i].stride );
                

                for (u32 i2=0; i2<rpd.vtxStreamList[i].numLayout; i2++)
                {
                    vtxAttributeDescrList[totNumAttributeDescr].binding  = rpd.vtxStreamList[i].streamIndex;
                    vtxAttributeDescrList[totNumAttributeDescr].location = rpd.vtxStreamList[i].list[i2].bindingLocation;
                    vtxAttributeDescrList[totNumAttributeDescr].offset = rpd.vtxStreamList[i].list[i2].offset;
                    vtxAttributeDescrList[totNumAttributeDescr].format = gos::gpu::toVulkan (rpd.vtxStreamList[i].list[i2].format);
                    
                    totNumAttributeDescr++;
                }            
            }

            vkVertexInputStateCreateInfo.vertexBindingDescriptionCount = rpd.numVtxStream;
            vkVertexInputStateCreateInfo.pVertexBindingDescriptions = vxtBindingDescrList;
            vkVertexInputStateCreateInfo.vertexAttributeDescriptionCount = totNumAttributeDescr;
            vkVertexInputStateCreateInfo.pVertexAttributeDescriptions = vtxAttributeDescrList;        
        }
    }

    //input assemply (aka draw primitive)
    VkPipelineInputAssemblyStateCreateInfo inputAssembly;
    pipelineCreateInfo.pInputAssemblyState = &inputAssembly;
    {
        memset (&inputAssembly, 0, sizeof(inputAssembly));
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = gos::gpu::toVulkan(rpd.drawPrimitive);
        inputAssembly.primitiveRestartEnable = VK_FALSE;
    }
    
    //fill mode, cull mode...
    VkPipelineRasterizationStateCreateInfo rasterizer;
    pipelineCreateInfo.pRasterizationState = &rasterizer;
    {
        memset (&rasterizer, 0, sizeof(rasterizer));
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = rpd.bWireframe ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;      //fill o wirefram
        rasterizer.lineWidth = 1.0f;
        
        switch (rpd.cullMode)
        {
        case eCullMode::NONE:
            rasterizer.cullMode = VK_CULL_MODE_NONE;
            break;
        
        case eCullMode::CCW:
            rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;     //i tris "front face" sono quelli con i vtx in ordine orario
            rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;        //scarto tutti i triangoli backfacing
            break;

        case eCullMode::CW:
            rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE; //i tris "front face" sono quelli con i vtx in ordine anti orario
            rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;            //scarto tutti i triangoli backfacing
            break;
        }
        rasterizer.depthBiasEnable = VK_FALSE;
        rasterizer.depthBiasConstantFactor = 0.0f; // Optional
        rasterizer.depthBiasClamp = 0.0f; // Optional
        rasterizer.depthBiasSlopeFactor = 0.0f; // Optional
    }


    //depth & stencil
    VkPipelineDepthStencilStateCreateInfo depthStencil;
    pipelineCreateInfo.pDepthStencilState = &depthStencil;
    {
        memset (&depthStencil, 0, sizeof(depthStencil));
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = (rpd.zbuffer_is_enabled()) ? VK_TRUE : VK_FALSE;
        depthStencil.depthWriteEnable = rpd.zbuffer_is_write_enabled() ? VK_TRUE : VK_FALSE;
        depthStencil.depthCompareOp = gpu::toVulkan (rpd.zbuffer_cmpFn);
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.minDepthBounds = 0.0f; // Optional
        depthStencil.maxDepthBounds = 1.0f; // Optional    
        
        depthStencil.stencilTestEnable = VK_FALSE;
        depthStencil.front.failOp = VK_STENCIL_OP_KEEP;
        depthStencil.front.depthFailOp = VK_STENCIL_OP_KEEP;
        depthStencil.front.passOp = VK_STENCIL_OP_KEEP;
        depthStencil.front.compareOp = VK_COMPARE_OP_ALWAYS;
        depthStencil.front.compareMask = 0;
        depthStencil.front.writeMask = 0;
        depthStencil.front.reference = 0;

        depthStencil.back.failOp = VK_STENCIL_OP_KEEP;
        depthStencil.back.depthFailOp = VK_STENCIL_OP_KEEP;
        depthStencil.back.passOp = VK_STENCIL_OP_KEEP;
        depthStencil.back.compareOp = VK_COMPARE_OP_ALWAYS;
        depthStencil.back.compareMask = 0;
        depthStencil.back.writeMask = 0;
        depthStencil.back.reference = 0;
    }


    //pipeline dynamic state
    //Si indicano qui quali stati delle pipeline sono dinamici e che quindi possono essere settati dinamicamente di volta in volta.
    //Gli stati non dinamici (cioe' quasi tutti), sono definiti nella pipeline e non potranno mai cambiare (alpha blending, culling, etc)
    u32 num_dynamic_state = 0;
    VkDynamicState dynamicStateList[8];
        dynamicStateList[num_dynamic_state++] = VK_DYNAMIC_STATE_VIEWPORT;
        dynamicStateList[num_dynamic_state++] = VK_DYNAMIC_STATE_SCISSOR;

        if (rpd.zbuffer_is_depthTestEnablingDisabling_enabled())      dynamicStateList[num_dynamic_state++] = VK_DYNAMIC_STATE_DEPTH_TEST_ENABLE;
        if (rpd.zbuffer_is_depthWriteEnablingDisabling_enabled())     dynamicStateList[num_dynamic_state++] = VK_DYNAMIC_STATE_DEPTH_WRITE_ENABLE;

    VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo;
    pipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
    {
        memset (&dynamicStateCreateInfo, 0, sizeof(dynamicStateCreateInfo));
        dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicStateCreateInfo.dynamicStateCount = num_dynamic_state;
        dynamicStateCreateInfo.pDynamicStates = dynamicStateList;
    }

    //Viewport (dinamica, va settata ogni volta con i comandi del renderBuffer
    VkPipelineViewportStateCreateInfo viewportStateCreateInfo;
    pipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
    {
        memset (&viewportStateCreateInfo, 0, sizeof(viewportStateCreateInfo));
        viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportStateCreateInfo.viewportCount = 1;
        viewportStateCreateInfo.scissorCount = 1;
        //viewportState.pViewports = &viewport; //non serve visto che ho definto la pipeline con viewport dinamica  
    }


    //multisample disabled
    VkPipelineMultisampleStateCreateInfo multisamplingCreateInfo;
    pipelineCreateInfo.pMultisampleState = &multisamplingCreateInfo;
    {
        memset (&multisamplingCreateInfo, 0, sizeof(multisamplingCreateInfo));
        multisamplingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisamplingCreateInfo.sampleShadingEnable = VK_FALSE;
        multisamplingCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisamplingCreateInfo.minSampleShading = 1.0f; // Optional
        multisamplingCreateInfo.pSampleMask = nullptr; // Optional
        multisamplingCreateInfo.alphaToCoverageEnable = VK_FALSE; // Optional
        multisamplingCreateInfo.alphaToOneEnable = VK_FALSE; // Optional
    }


    //color blend mode
    /*  Questa di seguito è l'esatto algo applicato dai driver per determinare il colore finale in base ai parametri
        if (blendEnable) {
            finalColor.rgb = (srcColorBlendFactor * newColor.rgb) <colorBlendOp> (dstColorBlendFactor * oldColor.rgb);
            finalColor.a = (srcAlphaBlendFactor * newColor.a) <alphaBlendOp> (dstAlphaBlendFactor * oldColor.a);
        } else {
            finalColor = newColor;
        }

        finalColor = finalColor & colorWriteMask;
    */
    VkPipelineColorBlendStateCreateInfo colorBlendingCreateInfo;
    VkPipelineColorBlendAttachmentState colorBlendAttachment[GOSGPU__NUM_MAX_ATTACHMENT];
    pipelineCreateInfo.pColorBlendState = &colorBlendingCreateInfo;
    {
        memset (&colorBlendingCreateInfo, 0, sizeof(colorBlendingCreateInfo));
        colorBlendingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlendingCreateInfo.logicOpEnable = VK_FALSE;
        colorBlendingCreateInfo.logicOp = VK_LOGIC_OP_COPY; // Optional
        colorBlendingCreateInfo.attachmentCount = rpd.numRT;
        colorBlendingCreateInfo.pAttachments = colorBlendAttachment;
        colorBlendingCreateInfo.blendConstants[0] = 0.0f; // Optional
        colorBlendingCreateInfo.blendConstants[1] = 0.0f; // Optional
        colorBlendingCreateInfo.blendConstants[2] = 0.0f; // Optional
        colorBlendingCreateInfo.blendConstants[3] = 0.0f; // Optional 

        memset (colorBlendAttachment, 0, sizeof(colorBlendAttachment));
        for (u32 i = 0; i < rpd.numRT; i++)
        {
            colorBlendAttachment[i].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
            colorBlendAttachment[i].blendEnable = VK_FALSE;
            colorBlendAttachment[i].srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
            colorBlendAttachment[i].dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
            colorBlendAttachment[i].colorBlendOp = VK_BLEND_OP_ADD; // Optional
            colorBlendAttachment[i].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
            colorBlendAttachment[i].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
            colorBlendAttachment[i].alphaBlendOp = VK_BLEND_OP_ADD; // Optional        
        }
    }


    //info addizionali per il dynamic rendering
    VkPipelineRenderingCreateInfo pipeline_rendering_create_info;
    VkFormat colorAttachmentsFormat[GOSGPU__NUM_MAX_ATTACHMENT];
    pipelineCreateInfo.pNext = &pipeline_rendering_create_info;
    {
        memset (&pipeline_rendering_create_info, 0, sizeof(pipeline_rendering_create_info));
        pipeline_rendering_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;

        if (rpd.zbuffer_is_enabled())
        {
            eImageFormat fmt = rpd.zbuffer_format;
            if (eImageFormat::_DEPTH_BEST == fmt)
                fmt = this->zbuffer_getBestFormat();

            pipeline_rendering_create_info.depthAttachmentFormat = gpu::toVulkan(fmt);
            //pipeline_rendering_create_info.stencilAttachmentFormat = gpu::toVulkan(fmt);
        }


        pipeline_rendering_create_info.colorAttachmentCount = rpd.numRT;
        pipeline_rendering_create_info.pColorAttachmentFormats = colorAttachmentsFormat;
        for (u32 i=0; i<rpd.numRT; i++)
        {
            if (eImageFormat::_SAME_AS_CURRENT_SWAPCHAIN == rpd.renderTargetFormat[i])
                colorAttachmentsFormat[i] = swapchain.getImageFormat();
            else
                colorAttachmentsFormat[i] = gpu::toVulkan(rpd.renderTargetFormat[i]);
        }

    }


    VkPipeline vkPipelineHandle;
    result = vulkan.pipeline_create (pipelineCreateInfo, &vkPipelineHandle);
    if (VK_SUCCESS != result)
    {
        gos::logger::err ("GPU::pipeline_createNew => vkCreateGraphicsPipelines() error: %s\n", string_VkResult(result)); 
        return false;
    }


    //riservo un handle
    out->vkPipelineLayoutHandle = pipelineCreateInfo.layout;
    out->vkPipelineHandle = vkPipelineHandle;
    return true;    
}

//************************************
bool GPU::descrSetInstance_create (const GPUDescrPoolHandle &poolHandle, const GPUPipelineHandle pipelineHandle, u8 descrSetNum, GPUDescrSetInstanceHandle *out_handle)
{
    const gpu::Pipeline2 *s = getInfo(pipelineHandle);
    if (NULL == s)
        return false;

    if (descrSetNum >= s->descrset_num)
    {
        DBGBREAK;
        return false;
    }

    return descrSetInstance_create (poolHandle, s->descrset_handle_defList[descrSetNum], out_handle);
}






