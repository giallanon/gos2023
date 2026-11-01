#include "gosGPUPipe2_cmdBufferWriter.h"
#include "../gosGPU.h"

using namespace gos;
using namespace gos::gpu;


//*********************************
ComputeCtx::ComputeCtx()
{
    gpu = NULL;
    vkCommandBuffer = VK_NULL_HANDLE;
    flag.zero();
}

//*********************************
ComputeCtx::~ComputeCtx()
{
}

//*********************************
bool ComputeCtx::priv_setup (CmdBufferWriter2 *ownerIN, GPU *gpuIN, VkCommandBuffer vkCommandBufferIN)
{
    gpu = gpuIN;
	owner = ownerIN;
    vkCommandBuffer = vkCommandBufferIN;
    flag.zero();
    cache_curPipeline = NULL;
    return true;
}


//*********************************
void ComputeCtx::priv_setError()
{ 
    flag.set(FLAG__ANY_ERROR); 
    DBGBREAK;
}

//*********************************
ComputeCtx& ComputeCtx::bindPipeline (const GPUPipelineHandle pipelineHandle)
{
    if (anyError())
        return *this;

    const gpu::Pipeline2 *pipe = gpu->get_info (pipelineHandle);
    assert (NULL != pipe);

    if (cache_curPipeline != pipe)
    {
        cache_curPipeline = pipe;
        vkCmdBindPipeline (vkCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, cache_curPipeline->vkPipelineHandle);
		flag.clear(FLAG__PC_UPDATED);
    }
    return *this;        
}

//*********************************
ComputeCtx& ComputeCtx::bindDescriptorSet (const GPUDescrSetInstanceHandle handle, u8 set, u32 dynamicOffset)
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
    const gpu::DescrSetInstance *ds = gpu->get_info(handle);
    if (NULL == ds)
    {
        gos::logger::err ("gpu::CmdBufferWriter::bindDescriptorSet() => invalid descrSetInstace handle\n");
        priv_setError();
        return *this;
    }           

    if (u32MAX == dynamicOffset)
        vkCmdBindDescriptorSets (vkCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, cache_curPipeline->vkPipelineLayoutHandle, set, 1, &ds->vkHandle, 0, nullptr);
    else
        vkCmdBindDescriptorSets (vkCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, cache_curPipeline->vkPipelineLayoutHandle, set, 1, &ds->vkHandle, 1, &dynamicOffset);
            
    return *this;
}

//*********************************
ComputeCtx& ComputeCtx::pushConstant (u8 whichOne, const void *data, u32 sizeof_data)
{
    assert (NULL != cache_curPipeline);
    assert (whichOne < GOSGPU__NUM_MAX_PUSH_CONSTANT_PER_PIPELINE);
    assert (cache_curPipeline->pcList[whichOne].size == sizeof_data);

    //copio il valore della push const nel buffer interno della pipe
    //e pusho l'intero rnge alla GPU solo prima di una draw...
    const u32 offset = cache_curPipeline->pcList[whichOne].offset;
    memcpy (&pushConstBuffer[offset], data, sizeof_data);
	flag.set (FLAG__PC_UPDATED);

    return *this;
}

//*********************************
void ComputeCtx::priv_flushPushConst()
{
	if (flag.isBitSet(FLAG__PC_UPDATED))
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
		flag.clear(FLAG__PC_UPDATED);
	}
}

//*********************************
ComputeCtx& ComputeCtx::dispatch(u32 group_count_x, u32 group_count_y, u32 group_count_z)
{
	priv_flushPushConst();
	vkCmdDispatch (vkCommandBuffer, group_count_x, group_count_y, group_count_z);
	return *this;
}

//*********************************
CmdBufferWriter2& ComputeCtx::compute_end()
{
    return *owner;
}