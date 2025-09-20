#include "gosGPUPipe2_cmdBufferWriter.h"
#include "../gosGPU.h"

using namespace gos;
using namespace gos::gpu;
using namespace gos::gpu::pipe2;

typedef pipe2::CmdBufferWriter2::BeginRend CMDBUFV_BGREND_CLASS;


//*********************************
void CmdBufferWriter2::BeginRend::priv_setup (GPU *gpuIN, CmdBufferWriter2 *cmdBufferWriterIN, VkCommandBuffer vkCommandBufferIN)
{
    gpu = gpuIN;
    cmdBufferWriter = cmdBufferWriterIN;
    vkCommandBuffer = vkCommandBufferIN;
    flag.zero();
    numColorAttachments = 0;
    haveZB = 0;
    renderAreaW = renderAreaH = 0;
    curPipeline = NULL;
}

//*********************************
CmdBufferWriter2& CmdBufferWriter2::BeginRend::endRender()
{
    if (!anyError())
    {
        if (flag.isBitSet(FLAG__vkBeginRender_HAS_BEEN_ISSUED))
            vkCmdEndRendering (vkCommandBuffer);
    }

    return *cmdBufferWriter;
}

//*********************************
void CmdBufferWriter2::BeginRend::priv_setError()
{ 
    flag.set(FLAG__ANY_ERROR); 
    DBGBREAK;
}

//*********************************
CMDBUFV_BGREND_CLASS& CmdBufferWriter2::BeginRend::withRenderArea (u32 w, u32 h)
{
    if (anyError())
        return *this;

    //withRT() / withZB() /withRenderArea() li devo usare per forza all'inizio.
    //Appena chiamo una funzione != da //withRT() / withZB() /withRenderArea() allora setto FLAG__vkBeginRender_HAS_BEEN_ISSUED e da li
    //in poi diventa invalido chiamare //withRT() / withZB() /withRenderArea()
    if (flag.isBitSet(FLAG__vkBeginRender_HAS_BEEN_ISSUED))
    {
        priv_setError();
        return *this;
    }
    renderAreaW = w;
    renderAreaH = h;
    return *this;
}

//*********************************
CMDBUFV_BGREND_CLASS& CmdBufferWriter2::BeginRend::withRenderArea (const GPURenderTargetHandle &rtHandle)
{
    const gpu::RenderTarget *rt_info = gpu->getInfo (rtHandle);
    assert (NULL != rt_info);
    return withRenderArea (rt_info->resolvedW, rt_info->resolvedH);
}

//*********************************
CMDBUFV_BGREND_CLASS& CmdBufferWriter2::BeginRend::withRT (const VkImageView &sourceIMG, eAttachmentLoadOp loadOp, eAttachmentStoreOp storeOp, const gos::ColorHDR &color)
{
    if (anyError())
        return *this;

    //withRT() / withZB() /withRenderArea() li devo usare per forza all'inizio.
    //Appena chiamo una funzione != da //withRT() / withZB() /withRenderArea() allora setto FLAG__vkBeginRender_HAS_BEEN_ISSUED e da li
    //in poi diventa invalido chiamare //withRT() / withZB() /withRenderArea()
    if (flag.isBitSet(FLAG__vkBeginRender_HAS_BEEN_ISSUED))
    {
        priv_setError();
        return *this;
    }

    assert (numColorAttachments < GOSGPU__NUM_MAX_ATTACHMENT);
    VkRenderingAttachmentInfo *p = &colorAttachList[numColorAttachments++];

    memset (p, 0, sizeof(VkRenderingAttachmentInfo));
    p->sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    p->imageView = sourceIMG;
    p->imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
    p->loadOp = gpu::toVulkan(loadOp);
    p->storeOp = gpu::toVulkan(storeOp);
    p->clearValue.color.float32[0] = color.col.r;
    p->clearValue.color.float32[1] = color.col.g;
    p->clearValue.color.float32[2] = color.col.b;
    p->clearValue.color.float32[3] = color.col.a;

    return *this;
}

CMDBUFV_BGREND_CLASS& CmdBufferWriter2::BeginRend::withRT (const GPURenderTargetHandle &rtHandle, eAttachmentLoadOp loadOp, eAttachmentStoreOp storeOp, const gos::ColorHDR &color)
{
    const gpu::RenderTarget *rt_info = gpu->getInfo (rtHandle);
    assert (NULL != rt_info);
    return withRT (rt_info->view, loadOp, storeOp, color);
}

//*********************************
CMDBUFV_BGREND_CLASS& CmdBufferWriter2::BeginRend::withZB (const GPUDepthStencilHandle &zbHandle, eAttachmentLoadOp loadOp, eAttachmentStoreOp storeOp, f32 clearValue_depth, u32 clearValue_stencil)
{
    if (anyError())
        return *this;

    //withRT() / withZB() /withRenderArea() li devo usare per forza all'inizio.
    //Appena chiamo una funzione != da //withRT() / withZB() /withRenderArea() allora setto FLAG__vkBeginRender_HAS_BEEN_ISSUED e da li
    //in poi diventa invalido chiamare //withRT() / withZB() /withRenderArea()
    if (flag.isBitSet(FLAG__vkBeginRender_HAS_BEEN_ISSUED))
    {
        priv_setError();
        return *this;
    }

    assert (0 == haveZB);

    haveZB = 1;
    const gpu::DepthStencil *zBuffer_info = gpu->getInfo (zbHandle);
    assert (NULL != zBuffer_info);


    memset (&zBufferAttach, 0, sizeof(zBufferAttach));
    zBufferAttach.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    zBufferAttach.imageView = zBuffer_info->view;
    zBufferAttach.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL_KHR;
    zBufferAttach.loadOp = gpu::toVulkan(loadOp);
    zBufferAttach.storeOp = gpu::toVulkan(storeOp);
    zBufferAttach.clearValue.depthStencil.depth = clearValue_depth;
    zBufferAttach.clearValue.depthStencil.stencil = clearValue_stencil;

    return *this;
}


//*********************************
void CmdBufferWriter2::BeginRend::priv_recordBeginRenderingIfNeeded()
{
    if (flag.isBitSet(FLAG__vkBeginRender_HAS_BEEN_ISSUED))
        return;

    if (0 == renderAreaW || 0 == renderAreaH)
    {
        DBGBREAK;
        priv_setError();
        return;
    }


    flag.set(FLAG__vkBeginRender_HAS_BEEN_ISSUED);

    VkRenderingInfo renderingInfo;
    memset (&renderingInfo, 0, sizeof(renderingInfo));
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea.extent.width = renderAreaW;
    renderingInfo.renderArea.extent.height = renderAreaH;
    renderingInfo.layerCount = 1;

    if (numColorAttachments)
    {
        renderingInfo.colorAttachmentCount = numColorAttachments;
        renderingInfo.pColorAttachments = colorAttachList;
    }

    if (haveZB)
        renderingInfo.pDepthAttachment = &zBufferAttach;    

    vkCmdBeginRendering (vkCommandBuffer, &renderingInfo);
}

//*********************************
CMDBUFV_BGREND_CLASS& CmdBufferWriter2::BeginRend::bindPipeline (const GPUPipelineHandle pipelineHandle)
{
    priv_recordBeginRenderingIfNeeded();
    if (anyError())
        return *this;

        
    gpu->toVulkan (pipelineHandle, &curPipeline);
    assert (NULL != pipe);
    vkCmdBindPipeline (vkCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, curPipeline->vkPipelineHandle);
    
    return *this;        
}

//*********************************
CMDBUFV_BGREND_CLASS& CmdBufferWriter2::BeginRend::bindDescriptorSet (const GPUDescrSetInstanceHandle handle, u8 set, u32 dynamicOffset)
{
    priv_recordBeginRenderingIfNeeded();
    if (anyError())
        return *this;

    if (NULL == curPipeline)
    {
        gos::logger::err ("gpu::pipe2::CmdBufferWriter::bindDescriptorSet() => no pipeline bound\n");
        priv_setError();
        return *this;
    }        

    //recupero il descrSetInstance
    VkDescriptorSet vkDescrSetHandle;
    if (!gpu->toVulkan (handle, &vkDescrSetHandle))
    {
        gos::logger::err ("gpu::pipe2::CmdBufferWriter::bindDescriptorSet() => invalid descrSetInstace handle\n");
        priv_setError();
        return *this;
    }           

    if (u32MAX == dynamicOffset)
        vkCmdBindDescriptorSets (vkCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, curPipeline->vkPipelineLayoutHandle, set, 1, &vkDescrSetHandle, 0, nullptr);
    else
        vkCmdBindDescriptorSets (vkCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, curPipeline->vkPipelineLayoutHandle, set, 1, &vkDescrSetHandle, 1, &dynamicOffset);
            
    return *this;
}

//*********************************
CMDBUFV_BGREND_CLASS& CmdBufferWriter2::BeginRend::bindVtxBuffer (const GPUVtxBufferHandle handle, u32 offsetIN)
{
    priv_recordBeginRenderingIfNeeded();
    if (anyError())
        return *this;

    VkBuffer vkVtxBuffer;
    if (!gpu->toVulkan (handle, &vkVtxBuffer))
    {
        gos::logger::err ("gpu::pipe2::CmdBufferWriter::bindVtxBuffer() => invalid vtxBufferHandle\n");
        priv_setError();
        return *this;
    }            

    //bindo il vtx buffer a partire dal layout=0
    static const u8 VTXBUFFER__FIRST_VTX_STREAM_INDEX = 0;
    static const u8 VTXBUFFER__NUM_STREAM = 1;
    VkBuffer        vtxBufferList[VTXBUFFER__NUM_STREAM] = { vkVtxBuffer };
    VkDeviceSize    vtxBufferOffsetsList[VTXBUFFER__NUM_STREAM] = { offsetIN };    
    vkCmdBindVertexBuffers (vkCommandBuffer, VTXBUFFER__FIRST_VTX_STREAM_INDEX, VTXBUFFER__NUM_STREAM, vtxBufferList, vtxBufferOffsetsList);


    return *this;
}

//*********************************
CMDBUFV_BGREND_CLASS& CmdBufferWriter2::BeginRend::bindVtxBuffers (const GPUVtxBufferHandle handleStream0, const GPUVtxBufferHandle handleStream1)
{
    priv_recordBeginRenderingIfNeeded();
    if (anyError())
        return *this;

    VkBuffer vkVtxBuffer0;
    if (!gpu->toVulkan (handleStream0, &vkVtxBuffer0))
    {
        gos::logger::err ("gpu::pipe2::CmdBufferWriter::bindVtxBuffer() => invalid vtxBufferHandle\n");
        priv_setError();
        return *this;
    }            

    VkBuffer vkVtxBuffer1;
    if (!gpu->toVulkan (handleStream1, &vkVtxBuffer1))
    {
        gos::logger::err ("gpu::pipe2::CmdBufferWriter::bindVtxBuffer() => invalid vtxBufferHandle\n");
        priv_setError();
        return *this;
    }            

    //bindo il vtx buffer a partire dal layout=0
    static const u8 VTXBUFFER__FIRST_VTX_STREAM_INDEX = 0;
    static const u8 VTXBUFFER__NUM_STREAM = 2;
    VkBuffer        vtxBufferList[VTXBUFFER__NUM_STREAM] = { vkVtxBuffer0, vkVtxBuffer1 };
    VkDeviceSize    vtxBufferOffsetsList[VTXBUFFER__NUM_STREAM] = {0};    
    vkCmdBindVertexBuffers (vkCommandBuffer, VTXBUFFER__FIRST_VTX_STREAM_INDEX, VTXBUFFER__NUM_STREAM, vtxBufferList, vtxBufferOffsetsList);


    return *this;
}

//*********************************
CMDBUFV_BGREND_CLASS& CmdBufferWriter2::BeginRend::bindIdxBufferU16 (const GPUIdxBufferHandle handle, u32 offsetIN)
{
    priv_recordBeginRenderingIfNeeded();
    if (anyError())
        return *this;

    VkBuffer vkIdxBuffer;
    if (!gpu->toVulkan (handle, &vkIdxBuffer))
    {
        gos::logger::err ("gpu::pipe2::CmdBufferWriter::bindIdxBufferU16() => invalid idxBufferHandle\n");
        priv_setError();
        return *this;
    }            

    vkCmdBindIndexBuffer (vkCommandBuffer, vkIdxBuffer, offsetIN, VK_INDEX_TYPE_UINT16);
    return *this;
}

//*********************************
CMDBUFV_BGREND_CLASS& CmdBufferWriter2::BeginRend::pushConstant (u8 whichOne, const void *data, u32 sizeof_data)
{
    priv_recordBeginRenderingIfNeeded();
    if (anyError())
        return *this;

    if (NULL == curPipeline)
    {
        gos::logger::err ("gpu::pipe2::CmdBufferWriter::pushConstant(%d) => no pipeline bound\n", whichOne);
        priv_setError();
        return *this;
    }

    if (whichOne >= GOSGPU__NUM_MAX_PUSH_CONSTANT_PER_PIPELINE)
    {
        gos::logger::err ("gpu::pipe2::CmdBufferWriter::pushConstant(%d) => invalid index\n", whichOne);
        priv_setError();
        return *this;
    }

    if (curPipeline->pushContantList[whichOne].size != sizeof_data)
    {
        gos::logger::err ("gpu::pipe2::CmdBufferWriter::pushConstant(%d) => size does not match\n", whichOne);
        priv_setError();
        return *this;
    }

    vkCmdPushConstants (vkCommandBuffer, curPipeline->vkPipelineLayoutHandle, 
                        curPipeline->pushContantList[whichOne].stageFlags,
                        curPipeline->pushContantList[whichOne].offset,
                        curPipeline->pushContantList[whichOne].size,
                        data);

    return *this;
}

//*********************************
CMDBUFV_BGREND_CLASS& CmdBufferWriter2::BeginRend::drawIndexed (u32 indexCount, u32 instanceCount, u32 firstIndex, u32 vertexOffset, u32 firstInstance)
{
    priv_recordBeginRenderingIfNeeded();
    if (anyError())
        return *this;

    if (NULL == curPipeline)
    {
        gos::logger::err ("gpu::pipe2::CmdBufferWriter::drawIndexed => no pipeline bound\n");
        priv_setError();
        return *this;
    }
       
    vkCmdDrawIndexed(vkCommandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
    return *this;
}

//*********************************
CMDBUFV_BGREND_CLASS& CmdBufferWriter2::BeginRend::draw (u32 vtxCount, u32 instanceCount, u32 firstVtx, u32 firstInstance)
{
    priv_recordBeginRenderingIfNeeded();
    if (anyError())
        return *this;

    if (NULL == curPipeline)
    {
        gos::logger::err ("gpu::pipe2::CmdBufferWriter::drawIndexed => no pipeline bound\n");
        priv_setError();
        return *this;
    }

    vkCmdDraw (vkCommandBuffer, vtxCount, instanceCount, firstVtx, firstInstance);        
    return *this;
}