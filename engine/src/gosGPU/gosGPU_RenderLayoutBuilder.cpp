#include "gosGPU.h"

using namespace gos;


typedef gos::GPU::RenderTaskLayoutBuilder                   RTLB_INFO;   //di comodo
typedef gos::GPU::RenderTaskLayoutBuilder::SubPassInfo      SUBPASS_INFO;   //di comodo

/**********************************************************************************************************************
 * Subpass info
 * 
 * 
 * 
 ***********************************************************************************************************************/
SUBPASS_INFO& GPU::RenderTaskLayoutBuilder::SubPassInfo::useRenderTarget (u8 index)
{
    if (index < GOSGPU__NUM_MAX_ATTACHMENT)
        renderTargetIndexList[nRenderTarget++] = index;
    else
    {
        gos::logger::err ("RenderTaskLayout::SubPass::useRenderTarget(%d) => invalid render target index!\n", index);
        DBGBREAK;
    }
    return *this;
}




/**********************************************************************************************************************
 * RenderTaskLayoutBuilder
 * 
 * 
 * 
 ***********************************************************************************************************************/
GPU::RenderTaskLayoutBuilder::RenderTaskLayoutBuilder (GPU *gpuIN, GPURenderLayoutHandle *out_handleIN) : GPU::TempBuilder (gpuIN)
{
    out_handle = out_handleIN;
    vkRenderPassHandle = VK_NULL_HANDLE;

    bAnyError = false;
    numRenderTargetInfo = 0;
    numSubpassInfo = 0;
    depthBuffer.reset();    
}

//***********************************************************
GPU::RenderTaskLayoutBuilder::~RenderTaskLayoutBuilder()
{ 
}

//***********************************************************
RTLB_INFO& GPU::RenderTaskLayoutBuilder::requireRendertarget (eRenderTargetUsage usage, VkFormat imageFormat, bool bClear, const gos::ColorHDR &clearColor)
{
    if (numRenderTargetInfo < GOSGPU__NUM_MAX_ATTACHMENT)
    {
        rtInfoList[numRenderTargetInfo].usage = usage;
        rtInfoList[numRenderTargetInfo].bClear = bClear;
        rtInfoList[numRenderTargetInfo].imageFormat = imageFormat;
        rtInfoList[numRenderTargetInfo].clearColor = clearColor;
        numRenderTargetInfo++;
    }
    else
    {
        bAnyError = true;
        gos::logger::err ("RenderTaskLayout::requireRendertarget() => too many!\n");
        DBGBREAK;
    }
    return *this;
}

//***********************************************************
RTLB_INFO& GPU::RenderTaskLayoutBuilder::requireDepthStencil (VkFormat imageFormat, bool bWithStencil, bool bClear, f32 clearValue)
{
    depthBuffer.isRequired = true;
    depthBuffer.bWithStencil = bWithStencil;
    depthBuffer.bClear = bClear;
    depthBuffer.clearValue = clearValue;
    depthBuffer.imageFormat = imageFormat;
    return *this;
}

//***********************************************************
SUBPASS_INFO& GPU::RenderTaskLayoutBuilder::addSubpass_GFX ()
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
SUBPASS_INFO& GPU::RenderTaskLayoutBuilder::addSubpass_COMPUTE ()
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
bool GPU::RenderTaskLayoutBuilder::end()
{
    if (!bAnyError)
    {
        if (!priv_buildVulkan())
            bAnyError = true;
    }

    return gpu->priv_renderLayout_onBuilderEnds (this);
}

//***********************************************************
bool GPU::RenderTaskLayoutBuilder::priv_buildVulkan()
{
    //color buffer attachment
    u8 numAttachment = 0;
    VkAttachmentDescription attachmentList[GOSGPU__NUM_MAX_ATTACHMENT];


    //elenco dei color buffer attachment
    for (u8 i=0; i<numRenderTargetInfo; i++)
    {
        memset (&attachmentList[numAttachment], 0, sizeof(VkAttachmentDescription));
        
        attachmentList[numAttachment].format = rtInfoList[i].imageFormat;
        attachmentList[numAttachment].samples = VK_SAMPLE_COUNT_1_BIT;
        attachmentList[numAttachment].loadOp = rtInfoList[i].bClear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        
        attachmentList[numAttachment].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;              //non mi interessa lo stato del buffer all'inizio, tanto lo pulisco

        switch (rtInfoList[i].usage)
        {
        default:
            gos::logger::err ("RenderTaskLayout::end() => invalid [usage] for RenderTarget %d\n", i);
            return false;

        case eRenderTargetUsage::presentation:
            //Lo uso per essere presentato a video
            attachmentList[numAttachment].storeOp = VK_ATTACHMENT_STORE_OP_STORE;                  //mantiene le info scritte in questo buffer
            attachmentList[numAttachment].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;          //il formato finale deve essere prensentabile
            break;

        case eRenderTargetUsage::storage:
            //Lo uso durante il rendering e, alla fine, devo conservarne i risultati
            attachmentList[numAttachment].storeOp = VK_ATTACHMENT_STORE_OP_STORE;                  //mantiene le info scritte in questo buffer
            attachmentList[numAttachment].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;  //leggibile dagli shader
            break;

        case eRenderTargetUsage::storage_discard:
            //Lo uso durante il rendering, ma alla fine lo posso buttare via
            attachmentList[numAttachment].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;              //alla fine del rendering, puoi buttare via il contenuto
            attachmentList[numAttachment].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;  //leggibile dagli shader
            break;
        }

        //stencil: don't care visto che e' un color buffer
        attachmentList[numAttachment].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachmentList[numAttachment].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

        numAttachment++;
    }


    //depthstencil attachment se necessario
    u32 indexOfDepthStencilAttachment = u32MAX;
    if (depthBuffer.isRequired)
    {
        indexOfDepthStencilAttachment = numAttachment;
        memset (&attachmentList[numAttachment], 0, sizeof(VkAttachmentDescription));

        attachmentList[numAttachment].format = depthBuffer.imageFormat;
        attachmentList[numAttachment].samples = VK_SAMPLE_COUNT_1_BIT;
        attachmentList[numAttachment].loadOp = depthBuffer.bClear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachmentList[numAttachment].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachmentList[numAttachment].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachmentList[numAttachment].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachmentList[numAttachment].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachmentList[numAttachment].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
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
            
            attachmentRef[nRef].attachment = indexOfDepthStencilAttachment;
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


