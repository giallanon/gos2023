#include "gosGPUPipe2_cmdBufferWriter.h"
#include "../gosGPU.h"

using namespace gos;
using namespace gos::gpu;

typedef CmdBufferWriter2::BeginRend CMDBUFV_BGREND_CLASS;


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

    cache_vxtBuffer_handle[0].setInvalid();
    cache_vxtBuffer_handle[1].setInvalid();
    cache_vxtBuffer_offset[0] = 0;
    cache_vxtBuffer_offset[1] = 0;

    cache_idxBuffer_handle.setInvalid();
    cache_idxBuffer_offset = 0;
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
CMDBUFV_BGREND_CLASS& CmdBufferWriter2::BeginRend::withZB (const GPUZBufferHandle &zbHandle, eAttachmentLoadOp loadOp, eAttachmentStoreOp storeOp, f32 clearValue_depth, u32 clearValue_stencil)
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

//***********************************************
CMDBUFV_BGREND_CLASS& CmdBufferWriter2::BeginRend::setDepthTestEnable (bool b)
{
    if (!anyError())
    {
        vkCmdSetDepthTestEnable (vkCommandBuffer, b == true ? 1 : 0);
    }
    return *this;
}

//***********************************************
CMDBUFV_BGREND_CLASS& CmdBufferWriter2::BeginRend::setDepthWriteEnable (bool b)
{
    if (!anyError())
    {
        vkCmdSetDepthWriteEnable (vkCommandBuffer, b == true ? 1 : 0);
    }
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

        
    curPipeline = gpu->getInfo (pipelineHandle);
    assert (NULL != curPipeline);
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
        gos::logger::err ("gpu::CmdBufferWriter::bindDescriptorSet() => no pipeline bound\n");
        priv_setError();
        return *this;
    }        

    //recupero il descrSetInstance
    const gpu::DescrSetInstance *ds = gpu->getInfo(handle);
    if (NULL == ds)
    {
        gos::logger::err ("gpu::CmdBufferWriter::bindDescriptorSet() => invalid descrSetInstace handle\n");
        priv_setError();
        return *this;
    }           

    if (u32MAX == dynamicOffset)
        vkCmdBindDescriptorSets (vkCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, curPipeline->vkPipelineLayoutHandle, set, 1, &ds->vkHandle, 0, nullptr);
    else
        vkCmdBindDescriptorSets (vkCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, curPipeline->vkPipelineLayoutHandle, set, 1, &ds->vkHandle, 1, &dynamicOffset);
            
    return *this;
}

CMDBUFV_BGREND_CLASS& CmdBufferWriter2::BeginRend::bindVtxIdxBuffer (const GPUVtxBufferHandle vtxbuffer_handle, u32 vtxbuffer_offset, const GPUIdxBufferHandle idxbuffer_handle, u32 idxbuffer_offset)
{
    assert (flag.isBitSet(FLAG__vkBeginRender_HAS_BEEN_ISSUED));

    if (vtxbuffer_handle != cache_vxtBuffer_handle[0] || vtxbuffer_offset != cache_vxtBuffer_offset[0])    
    {
        cache_vxtBuffer_handle[0] = vtxbuffer_handle;
        cache_vxtBuffer_offset[0] = vtxbuffer_offset;
    
        const gpu::Buffer *vtxBuffer = gpu->getInfo(vtxbuffer_handle);
        assert (NULL != vtxBuffer);

        static const u8 VTXBUFFER__FIRST_VTX_STREAM_INDEX = 0;
        static const u8 VTXBUFFER__NUM_STREAM = 1;
        VkBuffer        vtxBufferList[VTXBUFFER__NUM_STREAM] = { vtxBuffer->vkHandle };
        VkDeviceSize    vtxBufferOffsetsList[VTXBUFFER__NUM_STREAM] = { vtxbuffer_offset };    
        vkCmdBindVertexBuffers (vkCommandBuffer, VTXBUFFER__FIRST_VTX_STREAM_INDEX, VTXBUFFER__NUM_STREAM, vtxBufferList, vtxBufferOffsetsList);        
    }

    if (idxbuffer_handle != cache_idxBuffer_handle || idxbuffer_offset != cache_idxBuffer_offset)
    {
        cache_idxBuffer_handle = idxbuffer_handle;
        cache_idxBuffer_offset = idxbuffer_offset;

        const gpu::Buffer *idxBuffer = gpu->getInfo (idxbuffer_handle);
        assert (NULL != idxBuffer);
        vkCmdBindIndexBuffer (vkCommandBuffer, idxBuffer->vkHandle, idxbuffer_offset, VK_INDEX_TYPE_UINT16);
    }

    return *this;    
}

//*********************************
CMDBUFV_BGREND_CLASS& CmdBufferWriter2::BeginRend::bindVtxBuffer (const GPUVtxBufferHandle handle, u32 offsetIN)
{
    if (handle == cache_vxtBuffer_handle[0] && offsetIN == cache_vxtBuffer_offset[0])
        return *this;
    cache_vxtBuffer_handle[0] = handle;
    cache_vxtBuffer_offset[0] = offsetIN;

    assert (flag.isBitSet(FLAG__vkBeginRender_HAS_BEEN_ISSUED));
    //priv_recordBeginRenderingIfNeeded();
    //if (anyError())
    //    return *this;

    const gpu::Buffer *vtxBuffer = gpu->getInfo(handle);
    if (NULL == vtxBuffer)
    {
        gos::logger::err ("gpu::CmdBufferWriter::bindVtxBuffer() => invalid vtxBufferHandle\n");
        priv_setError();
        return *this;
    }            

    //bindo il vtx buffer a partire dal layout=0
    static const u8 VTXBUFFER__FIRST_VTX_STREAM_INDEX = 0;
    static const u8 VTXBUFFER__NUM_STREAM = 1;
    VkBuffer        vtxBufferList[VTXBUFFER__NUM_STREAM] = { vtxBuffer->vkHandle };
    VkDeviceSize    vtxBufferOffsetsList[VTXBUFFER__NUM_STREAM] = { offsetIN };    
    vkCmdBindVertexBuffers (vkCommandBuffer, VTXBUFFER__FIRST_VTX_STREAM_INDEX, VTXBUFFER__NUM_STREAM, vtxBufferList, vtxBufferOffsetsList);


    return *this;
}

//*********************************
CMDBUFV_BGREND_CLASS& CmdBufferWriter2::BeginRend::bindVtxBuffers (const GPUVtxBufferHandle handleStream0, const GPUVtxBufferHandle handleStream1)
{
    if (handleStream0 == cache_vxtBuffer_handle[0] && handleStream1 == cache_vxtBuffer_handle[1])
        return *this;
    cache_vxtBuffer_handle[0] = handleStream0;
    cache_vxtBuffer_handle[1] = handleStream1;

    assert (flag.isBitSet(FLAG__vkBeginRender_HAS_BEEN_ISSUED));
    //priv_recordBeginRenderingIfNeeded();
    //if (anyError())
    //    return *this;

    const gpu::Buffer *vtxBuffer0 = gpu->getInfo (handleStream0);
    if (NULL == vtxBuffer0)
    {
        gos::logger::err ("gpu::CmdBufferWriter::bindVtxBuffer() => invalid vtxBufferHandle\n");
        priv_setError();
        return *this;
    }            

    const gpu::Buffer *vtxBuffer1 = gpu->getInfo (handleStream1);
    if (NULL == vtxBuffer1)
    {
        gos::logger::err ("gpu::CmdBufferWriter::bindVtxBuffer() => invalid vtxBufferHandle\n");
        priv_setError();
        return *this;
    }            

    //bindo il vtx buffer a partire dal layout=0
    static const u8 VTXBUFFER__FIRST_VTX_STREAM_INDEX = 0;
    static const u8 VTXBUFFER__NUM_STREAM = 2;
    VkBuffer        vtxBufferList[VTXBUFFER__NUM_STREAM] = { vtxBuffer0->vkHandle, vtxBuffer1->vkHandle };
    VkDeviceSize    vtxBufferOffsetsList[VTXBUFFER__NUM_STREAM] = {0};    
    vkCmdBindVertexBuffers (vkCommandBuffer, VTXBUFFER__FIRST_VTX_STREAM_INDEX, VTXBUFFER__NUM_STREAM, vtxBufferList, vtxBufferOffsetsList);


    return *this;
}

//*********************************
CMDBUFV_BGREND_CLASS& CmdBufferWriter2::BeginRend::bindIdxBufferU16 (const GPUIdxBufferHandle handle, u32 offsetIN)
{
    if (handle == cache_idxBuffer_handle && offsetIN == cache_idxBuffer_offset)
        return *this;
    cache_idxBuffer_handle = handle;
    cache_idxBuffer_offset = offsetIN;


    assert (flag.isBitSet(FLAG__vkBeginRender_HAS_BEEN_ISSUED));
    //priv_recordBeginRenderingIfNeeded();
    //if (anyError())
    //    return *this;
        

    const gpu::Buffer *idxBuffer = gpu->getInfo (handle);
    if (NULL == idxBuffer)
    {
        gos::logger::err ("gpu::CmdBufferWriter::bindIdxBufferU16() => invalid idxBufferHandle\n");
        priv_setError();
        return *this;
    }            

    vkCmdBindIndexBuffer (vkCommandBuffer, idxBuffer->vkHandle, offsetIN, VK_INDEX_TYPE_UINT16);
    return *this;
}

//*********************************
CMDBUFV_BGREND_CLASS& CmdBufferWriter2::BeginRend::pushConstant (u8 whichOne, const void *data, u32 sizeof_data)
{
    assert (flag.isBitSet(FLAG__vkBeginRender_HAS_BEEN_ISSUED));
    //priv_recordBeginRenderingIfNeeded();
    //if (anyError())
    //    return *this;

    assert (NULL != curPipeline);
    // if (NULL == curPipeline)
    // {
    //     gos::logger::err ("gpu::CmdBufferWriter::pushConstant(%d) => no pipeline bound\n", whichOne);
    //     priv_setError();
    //     return *this;
    // }

    assert (whichOne < GOSGPU__NUM_MAX_PUSH_CONSTANT_PER_PIPELINE);
    // if (whichOne >= GOSGPU__NUM_MAX_PUSH_CONSTANT_PER_PIPELINE)
    // {
    //     gos::logger::err ("gpu::CmdBufferWriter::pushConstant(%d) => invalid index\n", whichOne);
    //     priv_setError();
    //     return *this;
    // }

    assert (curPipeline->pcList[whichOne].size == sizeof_data);
    // if (curPipeline->pcList[whichOne].size != sizeof_data)
    // {
    //     gos::logger::err ("gpu::CmdBufferWriter::pushConstant(%d) => size does not match\n", whichOne);
    //     priv_setError();
    //     return *this;
    // }

    //copio il valore della push const nel buffer interno della pipe
    //e pusho l'intero rnge alla GPU solo prima di una draw...
    const u32 offset = curPipeline->pcList[whichOne].offset;
    memcpy (&pushConstBuffer[offset], data, sizeof_data);

    return *this;
}

//*********************************
void CmdBufferWriter2::BeginRend::priv_flushPushConst()
{
    for (u32 i=0; i<curPipeline->pcRange_num; i++)
    {
        const u32 offset = curPipeline->pcRange_list[i].offset;
        vkCmdPushConstants (vkCommandBuffer, curPipeline->vkPipelineLayoutHandle, 
                                curPipeline->pcRange_list[i].stageFlags,
                                offset,
                                curPipeline->pcRange_list[i].size,
                                &pushConstBuffer[offset]);
    }
}

//*********************************
CMDBUFV_BGREND_CLASS& CmdBufferWriter2::BeginRend::drawIndexed (u32 indexCount, u32 instanceCount, u32 firstIndex, u32 vertexOffset, u32 firstInstance)
{
    assert (flag.isBitSet(FLAG__vkBeginRender_HAS_BEEN_ISSUED));
    //priv_recordBeginRenderingIfNeeded();
    //if (anyError())
    //    return *this;

    assert (NULL != curPipeline);
    // if (NULL == curPipeline)
    // {
    //     gos::logger::err ("gpu::CmdBufferWriter::drawIndexed => no pipeline bound\n");
    //     priv_setError();
    //     return *this;
    // }
       
    priv_flushPushConst();
    vkCmdDrawIndexed(vkCommandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
    return *this;
}

//*********************************
CMDBUFV_BGREND_CLASS& CmdBufferWriter2::BeginRend::draw (u32 vtxCount, u32 instanceCount, u32 firstVtx, u32 firstInstance)
{
    assert (flag.isBitSet(FLAG__vkBeginRender_HAS_BEEN_ISSUED));
    //priv_recordBeginRenderingIfNeeded();
    //if (anyError())
    //    return *this;

    assert (NULL != curPipeline);
    // if (NULL == curPipeline)
    // {
    //     gos::logger::err ("gpu::CmdBufferWriter::drawIndexed => no pipeline bound\n");
    //     priv_setError();
    //     return *this;
    // }

    priv_flushPushConst();
    vkCmdDraw (vkCommandBuffer, vtxCount, instanceCount, firstVtx, firstInstance);        
    return *this;
}