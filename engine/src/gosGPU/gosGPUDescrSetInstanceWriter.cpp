#include "gosGPUDescrSetInstanceWriter.h"
#include "gosGPU.h"


using namespace gos;


//***************************** 
gpu::DescrSetInstanceWriter::DescrSetInstanceWriter()
{
    gpu = NULL;
}

//***************************** 
gpu::DescrSetInstanceWriter& gpu::DescrSetInstanceWriter::begin (gos::GPU *gpuIN, const GPUDescrSetInstanceHandle &descrSetInstanceHandle)
{
    bAnyError = false;
    gpu = gpuIN;
    bufferList.num = 0;
    imageList.num = 0;
    numWriteDescr = 0;

    if (!gpu->toVulkan (descrSetInstanceHandle, &vkDescrSetHandle))
    {
        bAnyError = true;
        gos::logger::err ("DescrSetInstanceWriter::begin() => invalid DescrSetInstance handle\n");
        DBGBREAK;
    }

    return *this;
}

//***************************** 
gpu::DescrSetInstanceWriter& gpu::DescrSetInstanceWriter::bindUniformBuffer (u32 binding, const GPUUniformBufferHandle &handle)
{
    if (bAnyError)
        return *this;

    if (bufferList.num >= GOSGPU__NUM_MAX_DESCRIPTOR_PER_SET)
    {
        gos::logger::err ("DescrSetInstanceWriter::bindUniformBuffer() => too many descr buffer info\n");
        return *this;
    }
    
    VkBuffer vkBufferHandle;
    u32 bufferSize;
    if (!gpu->toVulkan (handle, &vkBufferHandle, &bufferSize))
    {
        gos::logger::err ("DescrSetInstanceWriter::bindUniformBuffer() => invalid uniform buffer handle\n");
        return *this;
    }        


    //buffer descr
    VkDescriptorBufferInfo *p = &bufferList.list[bufferList.num++];
    memset (p, 0, sizeof(VkDescriptorBufferInfo));
    p->buffer = vkBufferHandle;
    p->offset = 0;
    p->range = bufferSize;


    //write descr
    VkWriteDescriptorSet *d = &writeDescrList[numWriteDescr++];
    memset (d, 0, sizeof(VkWriteDescriptorSet));
    d->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    d->dstSet = vkDescrSetHandle;
    d->dstBinding = binding;
    d->dstArrayElement = 0;

    d->descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    d->descriptorCount = 1;    
    d->pBufferInfo = p;

    return *this;    
}

//***************************** 
gpu::DescrSetInstanceWriter& gpu::DescrSetInstanceWriter::bindTextureAndSampler (u32 binding, const GPUTextureHandle &texHandle, const GPUSamplerHandle &samplerHandle)
{
    if (bAnyError)
        return *this;

    if (imageList.num >= GOSGPU__NUM_MAX_DESCRIPTOR_PER_SET)
    {
        gos::logger::err ("DescrSetInstanceWriter::bindTextureAndSampler() => too many image buffer info\n");
        return *this;
    }
    
    VkImageView vkImgView;
    if (!gpu->toVulkan (texHandle, &vkImgView))
    {
        gos::logger::err ("DescrSetInstanceWriter::bindTextureAndSampler() => invalid texture handle\n");
        return *this;
    }        

    VkSampler vkSampler;
    if (!gpu->toVulkan (samplerHandle, &vkSampler))
    {
        gos::logger::err ("DescrSetInstanceWriter::bindTextureAndSampler() => invalid sampler handle\n");
        return *this;
    }        

    //image descr
    VkDescriptorImageInfo *p = &imageList.list[imageList.num++];
    memset (p, 0, sizeof(VkDescriptorImageInfo));
    p->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    p->imageView = vkImgView;
    p->sampler = vkSampler;


    //write descr
    VkWriteDescriptorSet *d = &writeDescrList[numWriteDescr++];
    memset (d, 0, sizeof(VkWriteDescriptorSet));
    d->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    d->dstSet = vkDescrSetHandle;
    d->dstBinding = binding;
    d->dstArrayElement = 0;

    d->descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    d->descriptorCount = 1;    
    d->pImageInfo = p;

    return *this;    
}

//***************************** 
bool gpu::DescrSetInstanceWriter::end()
{
    if (0 == numWriteDescr)
        bAnyError = true;

    if (bAnyError)
        return false;


    vkUpdateDescriptorSets (gpu->REMOVE_getVkDevice(), numWriteDescr, writeDescrList, 0, nullptr);
    return true;
}

