#include "gosGPUMappedBuffer.h"
#include "gosGPU.h"

using namespace gos;
using namespace gos::gpu;

//***********************************************
void MappedBufW::priv_reset()
{
	buffer = NULL;
	min_offset = u32MAX;
	max_offset = 0;
}

//***********************************************
bool MappedBufW::bind (GPU *gpuIN, const gpu::Buffer *bufferIN)
{
	assert (NULL != gpuIN);
	assert (NULL != bufferIN->mapped_host_pt);

	if (NULL != buffer)
	{
		DBGBREAK;
		return false;
	}

	priv_reset();
	gpu = gpuIN;
	buffer = bufferIN;
	return true;
}

//***********************************************
void MappedBufW::write (const void *src, u32 howManyBytes, u32 dst_offset)
{
	assert (NULL != buffer);
	assert (NULL != src);
	assert (howManyBytes > 0);

	const u32 o = dst_offset + howManyBytes -1;
	assert (o < buffer->bufferSize);
	if (o > max_offset) 			max_offset = o;
	if (dst_offset < min_offset)	min_offset = dst_offset;

	assert (o < buffer->bufferSize);
	memcpy (&buffer->mapped_host_pt[dst_offset], src, howManyBytes);
}

//***********************************************
void MappedBufW::end()
{
	if (u32MAX != min_offset)
	{
		const u32 size = (max_offset - min_offset) +1;
		gpu->_internal__buffer_end_write (buffer, min_offset, size);
	}
	
	priv_reset();
}