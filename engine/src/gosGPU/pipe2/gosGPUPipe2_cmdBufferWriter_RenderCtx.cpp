#include "gosGPUPipe2_cmdBufferWriter.h"
#include "../gosGPU.h"

using namespace gos;
using namespace gos::gpu;


//*********************************
RenderCtx::RenderCtx()
{
    gpu = NULL;
    vkCommandBuffer = VK_NULL_HANDLE;
    flag.zero();
}

//*********************************
RenderCtx::~RenderCtx()
{
}

//*********************************
bool RenderCtx::internal__reset (GPU *gpuIN, VkCommandBuffer vkCommandBufferIN, const RenderCtx::Props &propsIN)
{
    gpu = gpuIN;
    vkCommandBuffer = vkCommandBufferIN;
    memcpy (&props, &propsIN,sizeof(props));

    flag.zero();

    cache_curPipeline = NULL;
    cache_vxtBuffer_handle[0].setInvalid();
    cache_vxtBuffer_handle[1].setInvalid();
    cache_vxtBuffer_offset[0] = 0;
    cache_vxtBuffer_offset[1] = 0;
    cache_idxBuffer_handle.setInvalid();
    cache_idxBuffer_offset = 0;


    //verifica che le props siano valide
    if (0 == props.renderAreaW || 0 == props.renderAreaH)
    {
        logger::err ("RenderCtx::reset() =>  renderArea not set!\n");
        priv_setError();
        return false;
    }


    VkRenderingInfo renderingInfo;
    memset (&renderingInfo, 0, sizeof(renderingInfo));
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea.extent.width = props.renderAreaW;
    renderingInfo.renderArea.extent.height = props.renderAreaH;
    renderingInfo.layerCount = 1;

    if (props.numColorAttachments)
    {
        renderingInfo.colorAttachmentCount = props.numColorAttachments;
        renderingInfo.pColorAttachments = props.colorAttachList;
    }

    if (props.haveZB)
        renderingInfo.pDepthAttachment = &props.zBufferAttach;    

    vkCmdBeginRendering (vkCommandBuffer, &renderingInfo);
    flag.set (FLAG__BEGIN_RENDER_HAS_BEEN_CALLED);
    return true;
}


//*********************************
void RenderCtx::priv_setError()
{ 
    flag.set(FLAG__ANY_ERROR); 
    DBGBREAK;
}

//***********************************************
RenderCtx& RenderCtx::setDepthTestEnable (bool b)
{
    if (!anyError())
    {
        vkCmdSetDepthTestEnable (vkCommandBuffer, b == true ? 1 : 0);
    }
    return *this;
}

//***********************************************
RenderCtx& RenderCtx::setDepthWriteEnable (bool b)
{
    if (!anyError())
    {
        vkCmdSetDepthWriteEnable (vkCommandBuffer, b == true ? 1 : 0);
    }
    return *this;
}

//*********************************
RenderCtx& RenderCtx::bindPipeline (const GPUPipelineHandle pipelineHandle)
{
    if (anyError())
        return *this;

    const gpu::Pipeline2 *pipe = gpu->getInfo (pipelineHandle);
    assert (NULL != pipe);

    if (cache_curPipeline != pipe)
    {
        cache_curPipeline = pipe;
        vkCmdBindPipeline (vkCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, cache_curPipeline->vkPipelineHandle);
    }
    return *this;        
}

//*********************************
RenderCtx& RenderCtx::bindDescriptorSet (const GPUDescrSetInstanceHandle handle, u8 set, u32 dynamicOffset)
{
    if (anyError())
        return *this;

    if (NULL == cache_curPipeline)
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
        vkCmdBindDescriptorSets (vkCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, cache_curPipeline->vkPipelineLayoutHandle, set, 1, &ds->vkHandle, 0, nullptr);
    else
        vkCmdBindDescriptorSets (vkCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, cache_curPipeline->vkPipelineLayoutHandle, set, 1, &ds->vkHandle, 1, &dynamicOffset);
            
    return *this;
}

//*********************************
RenderCtx& RenderCtx::bindVtxIdxBuffer (const GPUVtxBufferHandle vtxbuffer_handle, u32 vtxbuffer_offset, const GPUIdxBufferHandle idxbuffer_handle, u32 idxbuffer_offset)
{
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
RenderCtx& RenderCtx::bindVtxBuffer (const GPUVtxBufferHandle handle, u32 offsetIN)
{
    if (handle == cache_vxtBuffer_handle[0] && offsetIN == cache_vxtBuffer_offset[0])
        return *this;
    cache_vxtBuffer_handle[0] = handle;
    cache_vxtBuffer_offset[0] = offsetIN;

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
RenderCtx& RenderCtx::bindVtxBuffers (const GPUVtxBufferHandle handleStream0, const GPUVtxBufferHandle handleStream1)
{
    if (handleStream0 == cache_vxtBuffer_handle[0] && handleStream1 == cache_vxtBuffer_handle[1])
        return *this;
    cache_vxtBuffer_handle[0] = handleStream0;
    cache_vxtBuffer_handle[1] = handleStream1;

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
RenderCtx& RenderCtx::bindIdxBufferU16 (const GPUIdxBufferHandle handle, u32 offsetIN)
{
    if (handle == cache_idxBuffer_handle && offsetIN == cache_idxBuffer_offset)
        return *this;
    cache_idxBuffer_handle = handle;
    cache_idxBuffer_offset = offsetIN;

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
RenderCtx& RenderCtx::pushConstant (u8 whichOne, const void *data, u32 sizeof_data)
{
    assert (NULL != cache_curPipeline);
    assert (whichOne < GOSGPU__NUM_MAX_PUSH_CONSTANT_PER_PIPELINE);
    assert (cache_curPipeline->pcList[whichOne].size == sizeof_data);

    //copio il valore della push const nel buffer interno della pipe
    //e pusho l'intero rnge alla GPU solo prima di una draw...
    const u32 offset = cache_curPipeline->pcList[whichOne].offset;
    memcpy (&pushConstBuffer[offset], data, sizeof_data);

    return *this;
}

//*********************************
void RenderCtx::priv_flushPushConst()
{
    for (u32 i=0; i<cache_curPipeline->pcRange_num; i++)
    {
        const u32 offset = cache_curPipeline->pcRange_list[i].offset;
        vkCmdPushConstants (vkCommandBuffer, cache_curPipeline->vkPipelineLayoutHandle, 
                                cache_curPipeline->pcRange_list[i].stageFlags,
                                offset,
                                cache_curPipeline->pcRange_list[i].size,
                                &pushConstBuffer[offset]);
    }
}

//*********************************
RenderCtx& RenderCtx::drawIndexed (u32 indexCount, u32 instanceCount, u32 firstIndex, u32 vertexOffset, u32 firstInstance)
{
    assert (NULL != cache_curPipeline);
    priv_flushPushConst();
    vkCmdDrawIndexed(vkCommandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
    return *this;
}

//*********************************
RenderCtx& RenderCtx::draw (u32 vtxCount, u32 instanceCount, u32 firstVtx, u32 firstInstance)
{
    assert (NULL != cache_curPipeline);

    priv_flushPushConst();
    vkCmdDraw (vkCommandBuffer, vtxCount, instanceCount, firstVtx, firstInstance);        
    return *this;
}

//*********************************
bool RenderCtx::end_render_ctx()
{
    if (flag.isBitSet (FLAG__BEGIN_RENDER_HAS_BEEN_CALLED))
    {
        vkCmdEndRendering (vkCommandBuffer);
        flag.clear(FLAG__BEGIN_RENDER_HAS_BEEN_CALLED);
    }

    return anyError();
}