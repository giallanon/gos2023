#include "VKExample1.h"
#include "dataTypes/gosTimer.h"

using namespace gos;

//************************************
VulkanExample1::VulkanExample1()
{
    window = NULL;
    vkInstance = VK_NULL_HANDLE;
    vkSurface = VK_NULL_HANDLE;
    vkDebugMessenger = VK_NULL_HANDLE;
}

//************************************
void VulkanExample1::cleanup() 
{
    if (VK_NULL_HANDLE != vkInstance)
    {
        if (VK_NULL_HANDLE != vulkan.dev)
        {
            pipe1.destroy (vulkan.dev);
            vulkan.destroy();
        }
        
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
    glfwDestroyWindow(window);
    glfwTerminate();
}    

//************************************
bool VulkanExample1::init()
{
    gos::Allocator *allocator = gos::getScrapAllocator();

    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    window = glfwCreateWindow(800, 600, gos::getAppName(), nullptr, nullptr);

    StringList requiredVulkanExtensionList(allocator);
    StringList requiredVulkanValidaionLayerList(allocator);

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
    gos::logger::log("\n");

    //aggiungo una callback per il layer di debug, giusto per printare i msg di vulkan in un bel colore rosa
#ifdef _DEBUG    
    vulkanAddDebugCallback();
#endif

    //creo una surface basata sulla [window]
    //GLFW fa tutto da solo, ma in linea di massima questa parte sarebbe dipendente da piattaforma
    VkResult result = glfwCreateWindowSurface(vkInstance, window, nullptr, &vkSurface);
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


    //creo una pipeline / render pass
    if (!vulkanCreatePipeline1(vulkan, &pipe1))
    {
        gos::logger::err ("can't creating pipeline1\n");
        return false;
    }
    gos::logger::log("\n");

    //creo il frame buffer, compatibile con la swapChain e il renderPass delle pipeline
    if (!vulkan.swapChainInfo.createFrameBuffer (vulkan.dev, pipe1.renderPass))
    {
        gos::logger::err ("can't create frame buffer\n");
        return false;
    }
    gos::logger::log("\n");


    return true;
}    

//************************************
bool VulkanExample1::vulkanCreateCommandBuffer (sVkDevice &vulkan, VkCommandBuffer *out)
{
    gos::logger::log ("vulkanCreateCommandBuffer: ");

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
    
    gos::logger::err ("vkAllocateCommandBuffers() => %s\n", string_VkResult(result));
    return false;
}

//************************************
bool VulkanExample1::vulkanCreatePipeline1 (sVkDevice &vulkan, sVkPipeline *out)
{
    gos::logger::log ("vulkanCreatePipeline1\n");
    gos::logger::incIndent();

    bool ret = false;
    VkShaderModule vShader = VK_NULL_HANDLE;    
    VkShaderModule pShader = VK_NULL_HANDLE;
    
    //carico gli shader
    while (1)
    {
        u8  s[1024];
        VkResult result;

        //shaders
        VkPipelineShaderStageCreateInfo shadersCreateInfoArray[2] = {};
        {
            //vtx shader
            gos::string::utf8::spf (s, sizeof(s), "%s/shader/vert1.spv", gos::getPhysicalPathToWritableFolder());
            if (!vulkanCreateShaderFromFile (s, vulkan.dev, &vShader))
            {
                gos::logger::err ("can't load shader %s\n", s);
                break;
            }    

            shadersCreateInfoArray[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            shadersCreateInfoArray[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
            shadersCreateInfoArray[0].module = vShader;
            shadersCreateInfoArray[0].pName = "main";


            //frag shader
            gos::string::utf8::spf (s, sizeof(s), "%s/shader/frag1.spv", gos::getPhysicalPathToWritableFolder());
            if (!vulkanCreateShaderFromFile (s, vulkan.dev, &pShader))
            {
                gos::logger::err ("can't load shader %s\n", s);
                break;
            }    
            shadersCreateInfoArray[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            shadersCreateInfoArray[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
            shadersCreateInfoArray[1].module = pShader;
            shadersCreateInfoArray[1].pName = "main";
        }

        //vertex shader input
        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 0;
        vertexInputInfo.pVertexBindingDescriptions = nullptr; // Optional
        vertexInputInfo.vertexAttributeDescriptionCount = 0;
        vertexInputInfo.pVertexAttributeDescriptions = nullptr; // Optional

        //input assemply 
        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;


        //pipeline dynamic state
        //Si indicano qui quali stati delle pipeline sono dinamici e che quindi posso essere settati
        //dinamicamente di volta in volta.
        //Gli stati non dinamici (cioe' quasi tutti), sono definiti nella pipeline e non potranno mai
        //cambiare (alpha blending, culling, etc)
        const VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = sizeof(dynamicStates) / sizeof(VkDynamicState);
        dynamicState.pDynamicStates = dynamicStates;


        //viewport
        /*VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float) vulkan.swapChainInfo.imageExtent.width;
        viewport.height = (float) vulkan.swapChainInfo.imageExtent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        */

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;
        //viewportState.pViewports = &viewport; //non serve visto che ho defintio la pipeline con viewport dinamica


        //fill mode, cull mode...
        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;
        rasterizer.depthBiasConstantFactor = 0.0f; // Optional
        rasterizer.depthBiasClamp = 0.0f; // Optional
        rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

        //multisplame disabled
        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisampling.minSampleShading = 1.0f; // Optional
        multisampling.pSampleMask = nullptr; // Optional
        multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
        multisampling.alphaToOneEnable = VK_FALSE; // Optional        

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
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional        

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f; // Optional
        colorBlending.blendConstants[1] = 0.0f; // Optional
        colorBlending.blendConstants[2] = 0.0f; // Optional
        colorBlending.blendConstants[3] = 0.0f; // Optional


        //uniform & pipelinelayput
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 0; // Optional
        pipelineLayoutInfo.pSetLayouts = nullptr; // Optional
        pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
        pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

        result = vkCreatePipelineLayout (vulkan.dev, &pipelineLayoutInfo, nullptr, &out->layout);
        if (VK_SUCCESS != result)
        {
            gos::logger::err ("vkCreatePipelineLayout() error: %s\n", string_VkResult(result)); 
            break;
        }

        //creo renderPass1 
        if (!vulkanCreateRenderPass(vulkan, &out->renderPass))
            break;


        //finalmente, creazione della pipe
        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shadersCreateInfoArray;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = nullptr; // Optional
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = out->layout;
        pipelineInfo.renderPass = out->renderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
        pipelineInfo.basePipelineIndex = -1; // Optional

        result = vkCreateGraphicsPipelines(vulkan.dev, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &out->pipe);
        if (VK_SUCCESS != result)
        {
            gos::logger::err ("vkCreateGraphicsPipelines() error: %s\n", string_VkResult(result)); 
            break;
        }

        //esci dal while(1)
        ret = true;
        break;
    }

    //elimino gli shader
    if (VK_NULL_HANDLE != vShader)
        vkDestroyShaderModule (vulkan.dev, vShader, nullptr);

    if (VK_NULL_HANDLE != pShader)
        vkDestroyShaderModule (vulkan.dev, pShader, nullptr);
    
    
    if (ret)
        gos::logger::log (eTextColor::green, "OK\n");
    gos::logger::decIndent();
    return ret;
}

//************************************
bool VulkanExample1::vulkanCreateRenderPass (sVkDevice &vulkan, VkRenderPass *out)
{
    gos::logger::log ("vulkanCreateRenderPass: ");
    bool ret = false;

    //color buffer attachment
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = vulkan.swapChainInfo.imageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;       //pulisci il buffer all'inizio del render pass
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;     //mantiene le info scritte in questo buffer alla fine del pass
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;        //stencil (dont care)
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;              //non mi interessa lo stato del buffer all'inizio, tanto lo pulisco
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;          //il formato finale deve essere prensentabile

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    //in questo esempio, voglio 1 solo "subpass" che opera sul color buffer
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    //dipendenza di questo subpassa da altri subpass (in questo caso non ce ne sono)=
    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;    //indica il subpassa before questo
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;    //aspetto che swapchain abbia letto tutto quanto
    dependency.srcAccessMask = 0;    
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    //creazione del render pass
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;    

    VkResult result = vkCreateRenderPass(vulkan.dev, &renderPassInfo, nullptr, out);
    if (VK_SUCCESS == result)
    {
        ret = true;
        gos::logger::log (eTextColor::green, "OK\n");
    }
    else
    {
        gos::logger::err ("vkCreateRenderPass() error: %s\n", string_VkResult(result)); 
    }  
        
    return ret;
}

//************************************
bool VulkanExample1::vulkanCreateShaderFromFile (const u8 *filename, VkDevice &dev, VkShaderModule *out)
{
    gos::Allocator *allocator = gos::getScrapAllocator();

    u32 bufferSize;
    u8 *buffer = gos::fs::fileLoadInMemory (allocator, filename, &bufferSize);
    if (NULL == buffer)
    {
        gos::logger::err ("vulkanCreateShaderFromFile => can't load shader %s\n", filename);
        return false;
    }    
    
    const bool ret = vulkanCreateShaderFromMemory (buffer, bufferSize, dev, out);
    GOSFREE(allocator, buffer);
    return ret;
}

//************************************
bool VulkanExample1::vulkanCreateShaderFromMemory (const u8 *buffer, u32 bufferSize, VkDevice &dev, VkShaderModule *out)
{
    assert (NULL != out);

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = bufferSize;
    createInfo.pCode = reinterpret_cast<const uint32_t*>(buffer);
    
    const VkResult result = vkCreateShaderModule(dev, &createInfo, nullptr, out);
    if (VK_SUCCESS == result)
        return true;

    gos::logger::err ("vulkanCreateShaderFromMemory() => %s\n", string_VkResult(result));
    return false;
}

//************************************
static VKAPI_ATTR VkBool32 VKAPI_CALL vulkanDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, UNUSED_PARAM(VkDebugUtilsMessageTypeFlagsEXT messageType), const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, UNUSED_PARAM(void* pUserData)) 
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
void VulkanExample1::vulkanAddDebugCallback()
{
    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = vulkanDebugCallback;
    createInfo.pUserData = nullptr; // Optional

    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(vkInstance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr)
        func(vkInstance, &createInfo, NULL, &vkDebugMessenger);
}

/*********************************************
 * [requiredValidationLayerList] e' una lista di stringhe separate da virgola che contiene un elenco di validation layer da attivare
 *                              es: VK_LAYER_KHRONOS_validation,VK_LAYER_LUNARG_monitor
 * [requiredExtensionList] come sopra, ma per le estensioni
 */
bool VulkanExample1::vulkanCreateInstance (VkInstance *out, const StringList &requiredValidationLayerList, const StringList &requiredExtensionList)
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
bool VulkanExample1::vulkanScanAndSelectAPhysicalDevices (VkInstance &vkInstance, const VkSurfaceKHR &vkSurface, const StringList &requiredExtensionList, sPhyDeviceInfo *out)
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

        //deve assolutamente essere una GPU dedicata
        if (deviceProperties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            gos::logger::log (eTextColor::red, "this device is not VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU\n");
            continue;
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
    return out->isValid();
}

/*********************************************
 * Dato il [vkPhyDevice] e una lista di estensioni richieste [requiredExtensionList], crea il device logico
 * create le queue e filla out_vulkan con queste informazioni
 */
bool VulkanExample1::vulkanCreateDevice (sPhyDeviceInfo &vkPhyDevInfo, const StringList &requiredExtensionList, sVkDevice *out_vulkan)
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
    memset (queueCreateInfo, 0, sizeof(queueCreateInfo));
    queueCreateInfo[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo[0].queueFamilyIndex = vkPhyDevInfo.gfxQIndex;
    queueCreateInfo[0].queueCount = 1;
    queueCreateInfo[0].pQueuePriorities = &queuePriority;

    queueCreateInfo[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo[1].queueFamilyIndex = vkPhyDevInfo.computeQIndex;
    queueCreateInfo[1].queueCount = 1;
    queueCreateInfo[1].pQueuePriorities = &queuePriority;


    //quali feature voglio usare?
    VkPhysicalDeviceFeatures deviceFeatures{};

    //creo il device
    const char *foundExtensions[128];
    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = queueCreateInfo;
    createInfo.queueCreateInfoCount = 2;
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
bool VulkanExample1::vulkanCreateSwapChain (sVkDevice &vulkan, const VkSurfaceKHR &vkSurface, sSwapChainInfo *out)
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
        //recupero gli handle delle image. Ho chiesto di crearne almeno [createInfo.minImageCount] ma il driver
        //potrebbe averne create di +
        vkGetSwapchainImagesKHR (vulkan.dev, out->vkSwapChain, &out->imageCount, NULL);
        vkGetSwapchainImagesKHR (vulkan.dev, out->vkSwapChain, &out->imageCount, out->vkImage);
        //creo le image view
        for (u8 i=0; i<out->imageCount; i++)
        {
            VkImageViewCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = out->vkImage[i];
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = out->imageFormat;
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;  

            result = vkCreateImageView (vulkan.dev, &createInfo, nullptr, &out->vkImageView[i]);
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
bool VulkanExample1::vulkanCreateSemaphore  (sVkDevice &vulkan, VkSemaphore *out)
{
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;    

    const VkResult result = vkCreateSemaphore (vulkan.dev, &semaphoreInfo, nullptr, out);
    if (VK_SUCCESS == result)
        return true;
    gos::logger::err ("vulkanCreateSemaphore() => %s\n", string_VkResult(result));
    return false;
}

//*********************************************
void VulkanExample1::vulkanDestroySemaphore  (sVkDevice &vulkan, VkSemaphore &in)
{
    if (VK_NULL_HANDLE != vulkan.dev && VK_NULL_HANDLE != in)
    {
        vkDestroySemaphore (vulkan.dev, in, nullptr);
        in = VK_NULL_HANDLE;
    }
}

//*********************************************
bool VulkanExample1::vulkanCreateFence  (sVkDevice &vulkan, bool bStartAsSignaled, VkFence *out)
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

//*********************************************
void VulkanExample1::vulkanDestroyFence  (sVkDevice &vulkan, VkFence &in)
{
    if (VK_NULL_HANDLE != vulkan.dev && VK_NULL_HANDLE != in)
    {
        vkDestroyFence (vulkan.dev, in, nullptr);
        in = VK_NULL_HANDLE;
    }
}

//************************************
bool VulkanExample1::recordCommandBuffer (u32 imageIndex, VkCommandBuffer &in_out_commandBuffer)
{
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0; // Optional
    beginInfo.pInheritanceInfo = nullptr; // Optional

    VkResult result = vkBeginCommandBuffer(in_out_commandBuffer, &beginInfo);
    if (VK_SUCCESS != result)
    {
        gos::logger::err ("recordCommandBuffer() => vkBeginCommandBuffer() => %s\n", string_VkResult(result));
        return false;
    }


    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = pipe1.renderPass;
    renderPassInfo.framebuffer = vulkan.swapChainInfo.frameBuffers[imageIndex];    
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = vulkan.swapChainInfo.imageExtent;

    VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;    

    vkCmdBeginRenderPass(in_out_commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(in_out_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->pipe1.pipe);


    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(vulkan.swapChainInfo.imageExtent.width);
    viewport.height = static_cast<float>(vulkan.swapChainInfo.imageExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(in_out_commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = vulkan.swapChainInfo.imageExtent;
    vkCmdSetScissor(in_out_commandBuffer, 0, 1, &scissor);


    vkCmdDraw(in_out_commandBuffer, 3, 1, 0, 0);

    vkCmdEndRenderPass (in_out_commandBuffer);

    result = vkEndCommandBuffer (in_out_commandBuffer);
    if (VK_SUCCESS != result)
    {
        gos::logger::err ("recordCommandBuffer() => vkEndCommandBuffer() => %s\n", string_VkResult(result));
        return false;
    }    
    
    return true;
}

/************************************
 * renderizza inviando command buffer a GPU e poi aspettando che questa
 * abbia finito il suo lavoro
 */
void VulkanExample1::mainLoop_waitEveryFrame()
{
    VkCommandBuffer     vkCommandBuffer;
    VkSemaphore         imageAvailableSemaphore;
    VkSemaphore         renderFinishedSemaphore;
    VkFence             inFlightFence;
    vulkanCreateCommandBuffer (vulkan, &vkCommandBuffer);
    vulkanCreateSemaphore (vulkan, &imageAvailableSemaphore);
    vulkanCreateSemaphore (vulkan, &renderFinishedSemaphore);
    vulkanCreateFence (vulkan, true, &inFlightFence);

    VkResult            result;
    gos::TimerFPS       fpsTimer;
    gos::Timer          cpuWaitTimer;
    gos::Timer          frameTimer;
    gos::Timer          acquireImageTimer;
    while (!glfwWindowShouldClose (window)) 
    {
//printf ("frame begin\n");
        frameTimer.start();
        fpsTimer.onFrameBegin();

        glfwPollEvents();

        //draw frames
        cpuWaitTimer.start();
            vkWaitForFences (vulkan.dev, 1, &inFlightFence, VK_TRUE, UINT64_MAX);
            vkResetFences (vulkan.dev, 1, &inFlightFence);
//printf ("  CPU waited GPU fence for %ld us\n", cpuWaitTimer.elapsed_usec());

        //recupero una immagine dalla swap chain, attendo per sempre e indico [imageAvailableSemaphore] come
        //semafro che GPU deve segnalare quando questa operazione e' ok
        acquireImageTimer.start();
            u32 imageIndex;
            result = vkAcquireNextImageKHR (vulkan.dev, vulkan.swapChainInfo.vkSwapChain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);        
            if (VK_SUCCESS != result)
            {
                gos::logger::err ("vkAcquireNextImageKHR() => %s\n", string_VkResult(result));
                continue;
            }
//printf ("  CPU waited vkAcquireNextImageKHR %ld us\n", acquireImageTimer.elapsed_usec());
        
        //command buffer che opera su [imageIndex]
        recordCommandBuffer(imageIndex, vkCommandBuffer);

        //submit
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore semaphoresToBeWaitedBeforeStarting[] = { imageAvailableSemaphore };
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = semaphoresToBeWaitedBeforeStarting;
        submitInfo.pWaitDstStageMask = waitStages;

        submitInfo.commandBufferCount = 1;        
        submitInfo.pCommandBuffers = &vkCommandBuffer;

        //semaforo che GPU segnalera' al termine dell'esecuzione del command buffer
        VkSemaphore signalSemaphores[] = {renderFinishedSemaphore};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        //submitto il batch a GPU e indico che deve segnalare [inFlightFence] quando ha finito 
        result = vkQueueSubmit (vulkan.gfxQ, 1, &submitInfo, inFlightFence);
        if (VK_SUCCESS != result)
        {
            gos::logger::err ("vkQueueSubmit() => %s\n", string_VkResult(result));
        }


        //presentazione
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores; //prima di presentare, aspetta che GPU segnali [signalSemaphores]
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &vulkan.swapChainInfo.vkSwapChain;
        presentInfo.pImageIndices = &imageIndex;
        
        vkQueuePresentKHR (vulkan.gfxQ, &presentInfo);

//printf ("  total frame time: %ldus\n", frameTimer.elapsed_usec());


        if (fpsTimer.onFrameEnd())
        {
            const float usec = fpsTimer.getAvgFrameTime_usec();
            const float msec = usec/ 1000.0f;
            printf ("Avg frame time: %.2fms [%.2fus] [fps: %.01f]\n", msec, usec, fpsTimer.getAvgFPS());
        }
    }

    //aspetto che GPU abbia finito tutto cio' che ha in coda
    vkDeviceWaitIdle(vulkan.dev);

    vulkanDestroySemaphore (vulkan, imageAvailableSemaphore);
    vulkanDestroySemaphore (vulkan, renderFinishedSemaphore);
    vulkanDestroyFence (vulkan, inFlightFence);
}

/************************************
 * renderizza inviando command buffer a GPU e poi, mentre GPU renderizza,
 * prepara il frame successivo in parallelo
 * 
 * Mah... non mi convince molto questa implementazione
 */
void VulkanExample1::mainLoop_multiFrame()
{
    static constexpr u8 MAX_FRAMES_IN_FLIGHT = 2;

    VkCommandBuffer     vkCommandBuffer[MAX_FRAMES_IN_FLIGHT];
    VkSemaphore         imageAvailableSemaphore[MAX_FRAMES_IN_FLIGHT];
    VkSemaphore         renderFinishedSemaphore[MAX_FRAMES_IN_FLIGHT];
    VkFence             inFlightFence[MAX_FRAMES_IN_FLIGHT];
    for (u8 i=0; i<MAX_FRAMES_IN_FLIGHT; i++)
    {
        vulkanCreateCommandBuffer (vulkan, &vkCommandBuffer[i]);
        vulkanCreateSemaphore (vulkan, &imageAvailableSemaphore[i]);
        vulkanCreateSemaphore (vulkan, &renderFinishedSemaphore[i]);
        vulkanCreateFence (vulkan, true, &inFlightFence[i]);
    }

    VkResult            result;
    gos::TimerFPS       fpsTimer;
    gos::Timer          cpuWaitTimer;
    gos::Timer          frameTimer;
    gos::Timer          acquireImageTimer;
    u32                 currentFrame = 0;


u32 exit=10;
    while (!glfwWindowShouldClose (window) && exit--) 
    {
        printf ("frame begin [%d]\n", currentFrame);
        frameTimer.start();
        fpsTimer.onFrameBegin();

        glfwPollEvents();

        //draw frames
        printf ("  waiting for fence\n");
        cpuWaitTimer.start();
            vkWaitForFences (vulkan.dev, 1, &inFlightFence[currentFrame], VK_TRUE, UINT64_MAX);
            vkResetFences (vulkan.dev, 1, &inFlightFence[currentFrame]);
        printf ("  CPU waited GPU fence for %ld us\n", cpuWaitTimer.elapsed_usec());

        //recupero una immagine dalla swap chain, attendo per sempre e indico [imageAvailableSemaphore] come
        //semafro che GPU deve segnalare quando questa operazione e' ok
        acquireImageTimer.start();
        u32 imageIndex;
        do
        {
            result = vkAcquireNextImageKHR (vulkan.dev, vulkan.swapChainInfo.vkSwapChain, 1000, imageAvailableSemaphore[currentFrame], VK_NULL_HANDLE, &imageIndex);        
            /*if (VK_SUCCESS != result)
            {
                gos::logger::err ("vkAcquireNextImageKHR() => %s\n", string_VkResult(result));
            }*/
        } while (VK_SUCCESS != result);

        printf ("  CPU waited vkAcquireNextImageKHR %ld us, image index is %d\n", acquireImageTimer.elapsed_usec(), imageIndex);
        
        //command buffer che opera su [imageIndex]
        recordCommandBuffer(imageIndex, vkCommandBuffer[currentFrame]);

        //submit
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore semaphoresToBeWaitedBeforeStarting[] = { imageAvailableSemaphore[currentFrame] };
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = semaphoresToBeWaitedBeforeStarting;
        submitInfo.pWaitDstStageMask = waitStages;

        submitInfo.commandBufferCount = 1;        
        submitInfo.pCommandBuffers = &vkCommandBuffer[currentFrame];

        //semaforo che GPU segnalera' al termine dell'esecuzione del command buffer
        VkSemaphore signalSemaphores[] = {renderFinishedSemaphore[currentFrame]};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        //submitto il batch a GPU e indico che deve segnalare [inFlightFence] quando ha finito 
        result = vkQueueSubmit (vulkan.gfxQ, 1, &submitInfo, inFlightFence[currentFrame]);
        if (VK_SUCCESS != result)
        {
            gos::logger::err ("vkQueueSubmit() => %s\n", string_VkResult(result));
        }


        //presentazione
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores; //prima di presentare, aspetta che GPU segnali [signalSemaphores]
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &vulkan.swapChainInfo.vkSwapChain;
        presentInfo.pImageIndices = &imageIndex;
        
        printf ("  present\n");
        vkQueuePresentKHR (vulkan.gfxQ, &presentInfo);

        printf ("  total frame time: %ldus\n", frameTimer.elapsed_usec());

        //next frame
        currentFrame++;
        if (currentFrame >= MAX_FRAMES_IN_FLIGHT)
            currentFrame = 0;

        if (fpsTimer.onFrameEnd())
        {
            const float usec = fpsTimer.getAvgFrameTime_usec();
            const float msec = usec/ 1000.0f;
            printf ("Avg frame time: %.2fms [%.2fus] [fps: %.01f]\n", msec, usec, fpsTimer.getAvgFPS());
        }

    }

    //aspetto che GPU abbia finito tutto cio' che ha in coda
    vkDeviceWaitIdle(vulkan.dev);

    for (u8 i=0; i<MAX_FRAMES_IN_FLIGHT; i++)
    {
        vulkanDestroySemaphore (vulkan, imageAvailableSemaphore[i]);
        vulkanDestroySemaphore (vulkan, renderFinishedSemaphore[i]);
        vulkanDestroyFence (vulkan, inFlightFence[i]);
    }
}

//************************************
void VulkanExample1::mainLoop()
{
    mainLoop_waitEveryFrame();
    //mainLoop_multiFrame();
}
