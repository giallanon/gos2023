#include "gosGPU.h"
#include "../gos/gosUtils.h"

using namespace gos;


typedef gos::GPU::RenderPassBuilder                   RTLB_INFO;   //di comodo
typedef gos::GPU::RenderPassBuilder::SubPassInfo      SUBPASS_INFO;   //di comodo

/**********************************************************************************************************************
 * Subpass info
 * 
 * 
 * 
 ***********************************************************************************************************************/
SUBPASS_INFO& GPU::RenderPassBuilder::SubPassInfo::writeToRenderTarget (u8 index)
{
    if (index < GOSGPU__NUM_MAX_ATTACHMENT)
        renderTargetIndexList[nRenderTarget++] = index;
    else
    {
        gos::logger::err ("RenderTaskLayout::SubPass::writeToRenderTarget(%d) => invalid render target index!\n", index);
        DBGBREAK;
    }
    return *this;
}




/**********************************************************************************************************************
 * RenderPassBuilder
 * 
 * 
 * 
 ***********************************************************************************************************************/
GPU::RenderPassBuilder::RenderPassBuilder (GPU *gpuIN, GPURenderPassHandle *out_handleIN) : GPU::TempBuilder (gpuIN)
{
    out_handle = out_handleIN;
    vkRenderPassHandle = VK_NULL_HANDLE;

    bAnyError = false;
    numRenderTargetInfo = 0;
    numSubpassInfo = 0;
    depthBuffer.reset();    
}

//***********************************************************
GPU::RenderPassBuilder::~RenderPassBuilder()
{ 
}



//***********************************************************
RTLB_INFO& GPU::RenderPassBuilder::requireRendertarget (const eImageFormat imageFormat, const eImageLayout initialLayout, const eImageLayout finalLayout, eAttachmentLoadOp loadOp, eAttachmentStoreOp storeOp)
{
    if (numRenderTargetInfo < GOSGPU__NUM_MAX_ATTACHMENT)
    {
        rtInfoList[numRenderTargetInfo].imageFormat = gpu::toVulkan (imageFormat);
        rtInfoList[numRenderTargetInfo].initialLayout = gpu::toVulkan(initialLayout);
        rtInfoList[numRenderTargetInfo].finalLayout = gpu::toVulkan(finalLayout);
        rtInfoList[numRenderTargetInfo].loadOp = gpu::toVulkan(loadOp);
        rtInfoList[numRenderTargetInfo].storeOp = gpu::toVulkan(storeOp);
        numRenderTargetInfo++;
    }
    else
    {
        bAnyError = true;
        gos::logger::err ("RenderTaskLayout::requireRendertarget2() => too many!\n");
        DBGBREAK;
    }
    return *this;
}


//***********************************************************
RTLB_INFO& GPU::RenderPassBuilder::requireZBuffer (const eImageFormat imageFormat, const eImageLayout initialLayout, const eImageLayout finalLayout, eAttachmentLoadOp loadOp, eAttachmentStoreOp storeOp)
{
    if (!utils::isFormatWithDepth(imageFormat))
    {
        bAnyError = true;
        gos::logger::err ("RenderTaskLayout::requireZBuffer(%s) => invalid format, not a DEPTH format\n", utils::enumToString(imageFormat));
        return *this;
    }
    
    depthBuffer.isRequired = true;
    depthBuffer.imageFormat = imageFormat;
    depthBuffer.initialLayout = initialLayout;
    depthBuffer.finalLayout = finalLayout;
    depthBuffer.loadOp = loadOp;
    depthBuffer.storeOp = storeOp;

    return *this;
}



//***********************************************************
SUBPASS_INFO& GPU::RenderPassBuilder::addSubpass_GFX ()
{
    if (numSubpassInfo < NUM_MAX_SUBPASS)
    {
        subpassInfoList[numSubpassInfo].priv_begin (this, SubPassInfo::eMode::gfx);
        return subpassInfoList[numSubpassInfo++];
    }
    else
    {
        bAnyError = true;
        gos::logger::err ("RenderTaskLayout::subpass_beginGFX() => too many!\n");
        DBGBREAK;
    }
    return subpassInfoList[0];
}

//***********************************************************
SUBPASS_INFO& GPU::RenderPassBuilder::addSubpass_COMPUTE ()
{
    if (numSubpassInfo < NUM_MAX_SUBPASS)
    {
        subpassInfoList[numSubpassInfo].priv_begin (this, SubPassInfo::eMode::compute);
        return subpassInfoList[numSubpassInfo++];
    }
    else
    {
        bAnyError = true;
        gos::logger::err ("RenderTaskLayout::subpass_beginCOMPUTE() => too many!\n");
        DBGBREAK;
    }
    return subpassInfoList[0];
}

//***********************************************************
bool GPU::RenderPassBuilder::end()
{
    if (!bAnyError)
    {
        if (!priv_buildVulkan())
            bAnyError = true;
    }

    return gpu->priv_renderLayout_onBuilderEnds (this);
}

//***********************************************************
bool GPU::RenderPassBuilder::priv_buildVulkan()
{
    //elenco degli attachment
    u8 numAttachment = 0;
    VkAttachmentDescription attachmentList[GOSGPU__NUM_MAX_ATTACHMENT];


    //elenco dei color buffer attachment
    for (u8 i=0; i<numRenderTargetInfo; i++)
    {
        memset (&attachmentList[numAttachment], 0, sizeof(VkAttachmentDescription));
        
        attachmentList[numAttachment].format = rtInfoList[i].imageFormat;
        attachmentList[numAttachment].samples = VK_SAMPLE_COUNT_1_BIT;
        attachmentList[numAttachment].initialLayout = rtInfoList[i].initialLayout;
        attachmentList[numAttachment].finalLayout = rtInfoList[i].finalLayout;
        attachmentList[numAttachment].loadOp = rtInfoList[i].loadOp;
        attachmentList[numAttachment].storeOp = rtInfoList[i].storeOp;

        //stencil: don't care visto che e' un color buffer
        attachmentList[numAttachment].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachmentList[numAttachment].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

        numAttachment++;
    }


    //depthstencil attachment se necessario
    depthBuffer.indexOfDepthStencilAttachment = 0xFF;
    if (depthBuffer.isRequired)
    {
        depthBuffer.indexOfDepthStencilAttachment = numAttachment;
        memset (&attachmentList[numAttachment], 0, sizeof(VkAttachmentDescription));

        attachmentList[numAttachment].format = gpu::toVulkan(depthBuffer.imageFormat);
        attachmentList[numAttachment].samples = VK_SAMPLE_COUNT_1_BIT;
        attachmentList[numAttachment].initialLayout = gpu::toVulkan(depthBuffer.initialLayout, depthBuffer.imageFormat);
        attachmentList[numAttachment].finalLayout = gpu::toVulkan(depthBuffer.finalLayout, depthBuffer.imageFormat);
        
        attachmentList[numAttachment].loadOp = gpu::toVulkan(depthBuffer.loadOp);
        attachmentList[numAttachment].storeOp = gpu::toVulkan(depthBuffer.storeOp);

        if (utils::isFormatWithStencil(depthBuffer.imageFormat))
        {
            attachmentList[numAttachment].stencilLoadOp = gpu::toVulkan(depthBuffer.loadOp);
            attachmentList[numAttachment].stencilStoreOp = gpu::toVulkan(depthBuffer.storeOp);
        }
        else
        {
            attachmentList[numAttachment].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachmentList[numAttachment].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        }
        numAttachment++;
    }



    //subpass
    static constexpr u32 NUM_MAX_ATTACHMENT_REF = 32;
    VkAttachmentReference attachmentRef[NUM_MAX_ATTACHMENT_REF];
    u32 nRef = 0;

    VkSubpassDescription subpassList[NUM_MAX_SUBPASS];
    for (u8 i=0; i<numSubpassInfo; i++)
    {
        memset (&subpassList[i], 0, sizeof(VkSubpassDescription));
    
        switch (subpassInfoList[i].mode)
        {
        default:
            gos::logger::err ("RenderTaskLayout::end() => invalid [mode] for Subpass %d\n", i);
            return false;

        case SubPassInfo::eMode::gfx:
            subpassList[i].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            break;

        case SubPassInfo::eMode::compute:
            subpassList[i].pipelineBindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;
            break;
        }

        //color attachment
        //Il subpass ha dichiarato di volere un certo numero di RenderTarget.
        const u8 nRT = subpassInfoList[i].nRenderTarget;
        if (0 == nRT)
        {
            subpassList[i].colorAttachmentCount = 0;
            subpassList[i].pColorAttachments = NULL;
        }
        else
        {
            subpassList[i].colorAttachmentCount = nRT;
            subpassList[i].pColorAttachments = &attachmentRef[nRef];
            
            for (u8 i2=0; i2<nRT; i2++)
            {
                const u8 rtIndex = subpassInfoList[i].renderTargetIndexList[i2];
                if (rtIndex >= this->numRenderTargetInfo)
                {
                    gos::logger::err ("RenderTaskLayout::end() => subpass %d want to use renderTarge at index %d but the num of RT for this layout is %d\n", i, rtIndex, numRenderTargetInfo);
                    return false;                    
                }

                if (nRef >= GOSGPU__NUM_MAX_ATTACHMENT)
                {
                    gos::logger::err ("RenderTaskLayout::end() => too many color attachment REF\n");
                    return false;                    
                }                
                
                attachmentRef[nRef].attachment = rtIndex;
                attachmentRef[nRef].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                nRef++;
            }
        }

        //depth stencil
        if (subpassInfoList[i].bUseDepthStencil)
        {
            subpassList[i].pDepthStencilAttachment = &attachmentRef[nRef];
            
            attachmentRef[nRef].attachment = depthBuffer.indexOfDepthStencilAttachment;
            attachmentRef[nRef].layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            nRef++;
        }
        else
            subpassList[i].pDepthStencilAttachment = NULL;

        //TODO
        subpassList[i].inputAttachmentCount = 0;
        subpassList[i].pInputAttachments = NULL;
        subpassList[i].preserveAttachmentCount = 0;
        subpassList[i].pPreserveAttachments = NULL;
    }



    //dipendenza di questo subpass da altri subpass (in questo caso non ce ne sono)
    VkSubpassDependency dependency{};

    if (subpassInfoList[0].bUseDepthStencil)
    {
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;

        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.srcAccessMask = 0;

        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    }
    else
    {
        //indico che il mio [subpass_0] dipende da un qualunque precedente e già esistente RenderPass sia in questo momento in esecuzione
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;

        //quando [srcSubpass] ha terminato [STAGE_COLOR_ATTACHMENT_OUTPUT], allora segnala che la dipendenza è risolta
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;

        //[subpass_0] può partire in ogni momento ma, primia di entrare in [STAGE_COLOR_ATTACHMENT] deve attendere che [srcSubpass] abbia segnalato la dipendenza
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;    //specifica in dettaglio quali tipi di accesso sono necessari. In questo caso, devo scrivere su un RenderTarget
    }


    //creazione del render pass
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = numAttachment;
    renderPassInfo.pAttachments = attachmentList;
    renderPassInfo.subpassCount = numSubpassInfo;
    renderPassInfo.pSubpasses = subpassList;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;    

    VkResult result = vkCreateRenderPass (gpu->REMOVE_getVkDevice(), &renderPassInfo, nullptr, &vkRenderPassHandle);
    if (VK_SUCCESS == result)
        return true;

    gos::logger::err ("RenderTaskLayout::end() => vkCreateRenderPass() failed, err=%s\n", string_VkResult(result)); 
    return false;                    
}


