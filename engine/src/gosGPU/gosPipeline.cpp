#include "gosPipeline.h"
#include "../gos/gos.h"
#include "gosGPU.h"
#include "vulkan/gosGPUVukanHelpers.h"

using namespace gos;
using namespace gos::gpu;


//******************************** 
Pipeline::Pipeline()
{
    allocator= gos::getSysHeapAllocator();

    gpu = NULL;            
    vkPipelineHandle = VK_NULL_HANDLE;
    vkLayoutHandle = VK_NULL_HANDLE;
    REMOVE_vkRenderPassHandle = VK_NULL_HANDLE;

    shaderList.setup (allocator, 8);
}

//******************************** 
Pipeline::~Pipeline()
{
    priv_deleteVkHandle();
    shaderList.unsetup ();
}

//******************************** 
void Pipeline::priv_deleteVkHandle()
{
    if (NULL == gpu)
        return;

    if (VK_NULL_HANDLE != vkLayoutHandle)
    {
        vkDestroyPipelineLayout (gpu->REMOVE_getVkDevice(), vkLayoutHandle, nullptr);
        vkLayoutHandle = VK_NULL_HANDLE;
    }

    if (VK_NULL_HANDLE != vkPipelineHandle)
    {
        vkDestroyPipeline (gpu->REMOVE_getVkDevice(), vkPipelineHandle, nullptr);
        vkPipelineHandle = VK_NULL_HANDLE;
    }

    if (VK_NULL_HANDLE != REMOVE_vkRenderPassHandle)
    {
        vkDestroyRenderPass (gpu->REMOVE_getVkDevice(), REMOVE_vkRenderPassHandle, nullptr);
        REMOVE_vkRenderPassHandle = VK_NULL_HANDLE;
    }

    gpu = NULL;
}    

//******************************** 
void Pipeline::begin(GPU *gpuIN)
{
    priv_deleteVkHandle();
    gpu = gpuIN;

    shaderList.reset();
    setDrawPrimitive (eDrawPrimitive::trisList);
    vtxDeclHandle.setInvalid();
}

//******************************** 
bool Pipeline::end ()
{
    assert (VK_NULL_HANDLE == vkPipelineHandle);
    assert (VK_NULL_HANDLE == vkLayoutHandle);
    assert (VK_NULL_HANDLE == REMOVE_vkRenderPassHandle);

    VkResult result;


    //shader
    static const u8     NUM_MAX_SHADER = 32;
    VkPipelineShaderStageCreateInfo shadersCreateInfoArray[NUM_MAX_SHADER] = {};
    u8 numShader = 0;
    {
        for (u32 i=0; i<shaderList.getNElem(); i++)
        {
            GPUShaderHandle handle = shaderList(i);
            
            shadersCreateInfoArray[numShader].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;

            switch (gpu->shader_getType(handle))
            {
            default:
                gos::logger::err ("GPU::Pipeline::end() => unsupported shader type\n");
                DBGBREAK;
                return false;
                break;

            case eShaderType::vertexShader:
                shadersCreateInfoArray[numShader].stage = VK_SHADER_STAGE_VERTEX_BIT;
                break;

            case eShaderType::fragmentShader:
                shadersCreateInfoArray[numShader].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
                break;                
            }
            
            shadersCreateInfoArray[numShader].module = gpu->shader_getVkHandle(handle);
            shadersCreateInfoArray[numShader].pName = gpu->shader_getMainFnName(handle);
            numShader++;
        }
    }



    //VtxDeclaration
    gos::VkPipelineVertexInputStage vertexInputInfo;
    if (!vertexInputInfo.build (gpu, vtxDeclHandle))
    {
        gos::logger::err ("GPU::Pipeline::end() => cannot use the current VtxDeclaration\n");
        DBGBREAK;
        return false;
    }
    

    //input assemply (aka draw primitive)
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = gos::gpu::drawPrimitive_to_vulkan(drawPrimitive);
    inputAssembly.primitiveRestartEnable = VK_FALSE;



    //pipeline dynamic state
    //Si indicano qui quali stati delle pipeline sono dinamici e che quindi possono essere settati dinamicamente di volta in volta.
    //Gli stati non dinamici (cioe' quasi tutti), sono definiti nella pipeline e non potranno mai cambiare (alpha blending, culling, etc)
    const VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = sizeof(dynamicStates) / sizeof(VkDynamicState);
        dynamicState.pDynamicStates = dynamicStates;


    //Viewport (dinamica, va settata ogni colta con i comandi del renderBuffer
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

    result = vkCreatePipelineLayout (gpu->REMOVE_getVkDevice(), &pipelineLayoutInfo, nullptr, &vkLayoutHandle);
    if (VK_SUCCESS != result)
    {
        gos::logger::err ("GPU::Pipeline::end() => vkCreatePipelineLayout() => %s\n", string_VkResult(result)); 
        return false;
    }



    //render pass
    //TODO: non deve stare qui dentro, deve esserci una classe esterna per definire il renderpass
    REMOVE_priv_createRenderPass();



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
    pipelineInfo.layout = vkLayoutHandle;
    pipelineInfo.renderPass = REMOVE_vkRenderPassHandle;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
    pipelineInfo.basePipelineIndex = -1; // Optional

    result = vkCreateGraphicsPipelines(gpu->REMOVE_getVkDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &vkPipelineHandle);
    if (VK_SUCCESS != result)
    {
        gos::logger::err ("GPU::Pipeline::end() => vkCreateGraphicsPipelines() error: %s\n", string_VkResult(result)); 
        return false;
    }

    return true;
}



//************************************* 
bool Pipeline::REMOVE_priv_createRenderPass ()
{
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

    VkResult result = vkCreateRenderPass (gpu->REMOVE_getVkDevice(), &renderPassInfo, nullptr, &REMOVE_vkRenderPassHandle);
    if (VK_SUCCESS == result)
        ret = true;
        
    return ret;
}