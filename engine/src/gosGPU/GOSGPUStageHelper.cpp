#include "GOSGPUStageHelper.h"
#include "gosGPU.h"

using namespace gos;
using namespace gos::gpu;


//**********************************
StageHelper::StageHelper()
{
	gpu = NULL;
}

//**********************************
void StageHelper::setup (GPU *gpuIN, u32 sizeof_stagingBufferIN)
{
	gpu = gpuIN;
	sizeof_stagingBuffer = sizeof_stagingBufferIN;

	if (!gpu->cmdBuffer_create (eGPUQueueFamily::transfer, &handle_cmdBuffer))
	{
		logger::err ("StageHelper::setup => can't create cmd buffer\n");
		return;
	}

	if (!gpu->stagingBuffer_create (sizeof_stagingBuffer, &handle_stgBuffer))
	{
		logger::err ("StageHelper::setup => can't create stage buffer\n");
		return;
	}

	job.setup (gpu);
}

//**********************************
void StageHelper::unsetup ()
{
	if (NULL == gpu)
		return;
	gpu->deleteResource (handle_stgBuffer);
	gpu->deleteResource (handle_cmdBuffer);
	job.unsetup();
	gpu = NULL;
}

//**********************************
StageHelper& StageHelper::begin()
{
	cw.begin (gpu, handle_cmdBuffer);
	ct = 0;
	return *this;
}

//**********************************
StageHelper& StageHelper::imageTransition (const VkImage &image, const eImageLayout currentLayout, const eImageLayout newLayout)						{ cw.imageTransition (image, currentLayout, newLayout); return *this;}
StageHelper& StageHelper::imageTransition (const GPURenderTargetHandle &rtHandle, const eImageLayout currentLayout, const eImageLayout newLayout)		{ cw.imageTransition (rtHandle, currentLayout, newLayout); return *this;}
StageHelper& StageHelper::imageTransition (const GPUZBufferHandle &zbHandle, const eImageLayout currentLayout, const eImageLayout newLayout)			{ cw.imageTransition (zbHandle, currentLayout, newLayout); return *this;}

StageHelper& StageHelper::copyImageToImage (const VkImage &source, const VkImage &destination, const VkExtent2D &srcSize, const VkExtent2D &dstSize)						{ cw.copyImageToImage (source, destination, srcSize, dstSize); return *this;}
StageHelper& StageHelper::copyImageToImage (const GPURenderTargetHandle &rtHandle, const VkImage &destination, const VkExtent2D &srcSize, const VkExtent2D &dstSize)		{ cw.copyImageToImage (rtHandle, destination, srcSize, dstSize); return *this;}
StageHelper& StageHelper::copyImageToImage (const GPURenderTargetHandle &rtSRC, const GPURenderTargetHandle &rtDST, const VkExtent2D &srcSize, const VkExtent2D &dstSize)	{ cw.copyImageToImage (rtSRC, rtDST, srcSize, dstSize); return *this;}

void StageHelper::priv_mem_to_stgBuffer (const void *src, u32 sizeof_src, u32 stgBuffer_offset)
{
	assert (sizeof_stagingBuffer >= stgBuffer_offset + sizeof_src);

    const gpu::Buffer *s = gpu->getInfo (handle_stgBuffer);

    //memcpy di dataSRC nello stagin buffer
    assert (s->mapped_size >= sizeof_src);
    memcpy (&s->mapped_host_pt[stgBuffer_offset], src, sizeof_src);	
}

//**********************************
void StageHelper::priv_stgBuffer_to_buffer (u32 stgBuffer_offset, GPUVtxBufferHandle dstBufferHandle, u32 offsetDST, u32 howManyByteToCopy)	{ cw.copyBuffer (handle_stgBuffer, dstBufferHandle, stgBuffer_offset, offsetDST, howManyByteToCopy); }
void StageHelper::priv_stgBuffer_to_buffer (u32 stgBuffer_offset, GPUIdxBufferHandle dstBufferHandle, u32 offsetDST, u32 howManyByteToCopy)	{ cw.copyBuffer (handle_stgBuffer, dstBufferHandle, stgBuffer_offset, offsetDST, howManyByteToCopy); }

//**********************************
StageHelper& StageHelper::mem_to_buffer (const void *src, u32 sizeof_src, GPUVtxBufferHandle dstBufferHandle, u32 offsetDST)
{
	assert (ct + sizeof_src <= sizeof_stagingBuffer);
	priv_mem_to_stgBuffer (src, sizeof_src, ct);
	priv_stgBuffer_to_buffer (ct, dstBufferHandle, offsetDST, sizeof_src);
	ct += sizeof_src;
	return *this;
}

//**********************************
StageHelper& StageHelper::mem_to_buffer (const void *src, u32 sizeof_src, GPUIdxBufferHandle dstBufferHandle, u32 offsetDST)
{
	assert (ct + sizeof_src <= sizeof_stagingBuffer);
	priv_mem_to_stgBuffer (src, sizeof_src, ct);
	priv_stgBuffer_to_buffer (ct, dstBufferHandle, offsetDST, sizeof_src);
	ct += sizeof_src;
	return *this;
}

//**********************************
StageHelper& StageHelper::mem_to_stgBuffer (const void *src, u32 sizeof_src, u32 *out_stgBuffer_offset)
{
	assert (ct + sizeof_src <= sizeof_stagingBuffer);
	
	if (NULL != out_stgBuffer_offset)
		*out_stgBuffer_offset = ct;
	priv_mem_to_stgBuffer (src, sizeof_src, ct);
	ct += sizeof_src;
	return *this;
}



//**********************************
void StageHelper::submit()
{
	cw.end();
	assert (false == cw.anyError());

	job.submit (handle_cmdBuffer);

    while (!job.hasFinished())
    {
    }	
}


