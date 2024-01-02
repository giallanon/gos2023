#include "gosGPUPipeline.h"
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
    vkPipeLayoutHandle = VK_NULL_HANDLE;

    shaderList.setup (allocator, 8);
}

//******************************** 
Pipeline::~Pipeline()
{
    cleanUp();
    shaderList.unsetup ();
}

//******************************** 
void Pipeline::cleanUp()
{
    if (VK_NULL_HANDLE != vkPipeLayoutHandle)
    {
        vkDestroyPipelineLayout (gpu->REMOVE_getVkDevice(), vkPipeLayoutHandle, nullptr);
        vkPipeLayoutHandle = VK_NULL_HANDLE;
    }

    if (VK_NULL_HANDLE != vkPipelineHandle)
    {
        vkDestroyPipeline (gpu->REMOVE_getVkDevice(), vkPipelineHandle, nullptr);
        vkPipelineHandle = VK_NULL_HANDLE;
    }
}    

//******************************** 
Pipeline& Pipeline::begin(GPU *gpuIN)
{
    cleanUp();
    gpu = gpuIN;

    shaderList.reset();
    setDrawPrimitive (eDrawPrimitive::trisList);
    vtxDeclHandle.setInvalid();

     return *this;
}

//******************************** 
bool Pipeline::end (VkRenderPass &vkRenderPassHandle)
{
    assert (VK_NULL_HANDLE == vkPipelineHandle);
    assert (VK_NULL_HANDLE == vkPipeLayoutHandle);
    assert (VK_NULL_HANDLE != vkRenderPassHandle);

    VkResult result;

    
    //Pipeline layout
    //Serve ad indicare il numero di "const push" disponibili alla pipe
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0; // Optional
    pipelineLayoutInfo.pSetLayouts = nullptr; // Optional
    pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
    pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

    result = vkCreatePipelineLayout (gpu->REMOVE_getVkDevice(), &pipelineLayoutInfo, nullptr, &vkPipeLayoutHandle);
    if (VK_SUCCESS != result)
    {
        gos::logger::err ("GPU::Pipeline::end() => vkCreatePipelineLayout() => %s\n", string_VkResult(result)); 
        return false;
    }



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
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;      //fill o wirefram
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;        //questa e la successiva in sostanza indicano
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;     //il cull mode = ccq
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
    pipelineInfo.layout = vkPipeLayoutHandle;
    pipelineInfo.renderPass = vkRenderPassHandle;
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

