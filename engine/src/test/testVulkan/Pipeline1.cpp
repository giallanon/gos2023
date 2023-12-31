#include "Pipeline1.h"
#include "vulkan/gosGPUVukanHelpers.h"

//************************************* 
void Pipeline1::priv_reset()                         
{
    _layoutHandle = VK_NULL_HANDLE; 
    renderPassHandle = VK_NULL_HANDLE; 
    pipeHandle = VK_NULL_HANDLE; 
    for (u8 i=0;i<SWAPCHAIN_NUM_MAX_IMAGES;i++)
        frameBufferHandleList[i] = VK_NULL_HANDLE;
}

//************************************* 
void Pipeline1::destroy (gos::GPU *gpu)
{
    if (VK_NULL_HANDLE != pipeHandle)
        vkDestroyPipeline (gpu->REMOVE_getVkDevice(), pipeHandle, nullptr);

    if (VK_NULL_HANDLE != _layoutHandle)
        vkDestroyPipelineLayout (gpu->REMOVE_getVkDevice(), _layoutHandle, nullptr);

    if (VK_NULL_HANDLE != renderPassHandle)
        vkDestroyRenderPass (gpu->REMOVE_getVkDevice(), renderPassHandle, nullptr);

    priv_destroyFrameBuffers(gpu);

    priv_reset();
}

//************************************* 
void Pipeline1::priv_destroyFrameBuffers (gos::GPU *gpu)
{
    for (u8 i=0;i<SWAPCHAIN_NUM_MAX_IMAGES;i++)
    {
        if (VK_NULL_HANDLE != frameBufferHandleList[i])
        {
            vkDestroyFramebuffer(gpu->REMOVE_getVkDevice(), frameBufferHandleList[i], nullptr);
            frameBufferHandleList[i] = VK_NULL_HANDLE;
        }
    }
}

//************************************* 
bool Pipeline1::create (gos::GPU *gpu, const GPUVtxDeclHandle vtxDeclHandle, eDrawPrimitive drawPrimitive)
{
    gos::logger::log ("Pipeline1::create()\n");
    gos::logger::incIndent();

    bool ret = false;
    GPUShaderHandle vtxShader;
    GPUShaderHandle fragShader;
    
    //carico gli shader
    while (1)
    {
        u8  s[1024];
        VkResult result;

        //shaders
        gos::string::utf8::spf (s, sizeof(s), "%s/testVulkan/shader/vert1.spv", gos::getAppPathNoSlash());
        if (!gpu->vtxshader_createFromFile (s, "main", &vtxShader))
        {
            gos::logger::err ("can't load shader %s\n", s);
            break;
        }    

        gos::string::utf8::spf (s, sizeof(s), "%s/testVulkan/shader/frag1.spv", gos::getAppPathNoSlash());
        if (!gpu->vtxshader_createFromFile (s, "main", &fragShader))
        {
            gos::logger::err ("can't load shader %s\n", s);
            break;
        }    

        VkPipelineShaderStageCreateInfo shadersCreateInfoArray[2] = {};
        {
            shadersCreateInfoArray[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            shadersCreateInfoArray[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
            shadersCreateInfoArray[0].module = gpu->shader_getVkHandle(vtxShader);
            shadersCreateInfoArray[0].pName = gpu->shader_getMainFnName(vtxShader);

            shadersCreateInfoArray[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            shadersCreateInfoArray[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
            shadersCreateInfoArray[1].module = gpu->shader_getVkHandle(fragShader);
            shadersCreateInfoArray[1].pName = gpu->shader_getMainFnName(fragShader);
        }

        //vertex shader input
        //In base a [vtxDecl], creo le info necessarie per bindare gli stream dei vxt
        gos::VkPipelineVertexInputStage vertexInputInfo;
        if (!vertexInputInfo.build (gpu, vtxDeclHandle))
        {
            gos::logger::err ("vertexInputInfo!!\n");
            break;
        }    



        //input assemply (aka draw primitive)
        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = gos::gpu::drawPrimitive_to_vulkan(drawPrimitive);
        inputAssembly.primitiveRestartEnable = VK_FALSE;


        //pipeline dynamic state
        //Si indicano qui quali stati delle pipeline sono dinamici e che quindi possono essere settati
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
        //viewportState.pViewports = &viewport; //non serve visto che ho definto la pipeline con viewport dinamica


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

        result = vkCreatePipelineLayout (gpu->REMOVE_getVkDevice(), &pipelineLayoutInfo, nullptr, &_layoutHandle);
        if (VK_SUCCESS != result)
        {
            gos::logger::err ("vkCreatePipelineLayout() error: %s\n", string_VkResult(result)); 
            break;
        }

        //creo renderPass1 
        if (!priv_createRenderPass(gpu))
            break;


        //finalmente, creazione della pipe
        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shadersCreateInfoArray;
        pipelineInfo.pVertexInputState = &vertexInputInfo.vkVertexInputState;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = nullptr; // Optional
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = _layoutHandle;
        pipelineInfo.renderPass = renderPassHandle;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
        pipelineInfo.basePipelineIndex = -1; // Optional

        result = vkCreateGraphicsPipelines(gpu->REMOVE_getVkDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeHandle);
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
    gpu->shader_delete (vtxShader);
    gpu->shader_delete (fragShader);
    
    //creo i frame buffer
    recreateFrameBuffers(gpu);

    if (ret)
        gos::logger::log (eTextColor::green, "OK\n");
    gos::logger::decIndent();
    return ret;
}

//************************************* 
bool Pipeline1::priv_createRenderPass (gos::GPU *gpu)
{
    gos::logger::log ("Pipeline1::priv_createRenderPass: ");
    bool ret = false;

    //color buffer attachment
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = gpu->swapChain_getImageFormat();
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
    subpass.pDepthStencilAttachment = NULL;

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

    VkResult result = vkCreateRenderPass (gpu->REMOVE_getVkDevice(), &renderPassInfo, nullptr, &renderPassHandle);
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

//************************************* 
bool Pipeline1::recreateFrameBuffers(gos::GPU *gpu)
{
    bool ret = true;
    gos::logger::log ("Pipeline1::priv_createFrameBuffers\n");
    gos::logger::incIndent();

    priv_destroyFrameBuffers(gpu);

    for (u8 i = 0; i < gpu->swapChain_getImageCount(); i++) 
    {
        VkImageView imageViewList[2] = { gpu->swapChain_getImageViewHandle(i) , 0};

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPassHandle;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = imageViewList;
        framebufferInfo.width = gpu->swapChain_getWidth();
        framebufferInfo.height = gpu->swapChain_getHeight();
        framebufferInfo.layers = 1;

        const VkResult result = vkCreateFramebuffer(gpu->REMOVE_getVkDevice(), &framebufferInfo, nullptr, &frameBufferHandleList[i]);
        if (VK_SUCCESS != result)
        {
            gos::logger::err ("vkCreateFramebuffer() => %s\n", string_VkResult(result));
            ret = false;
            break;
        }
    }

    gos::logger::decIndent();
    return ret;
}
