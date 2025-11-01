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

    descrSetInstHandle = gpu->getInfo (descrSetInstanceHandle);
    if (NULL == descrSetInstHandle)
    {
        bAnyError = true;
        gos::logger::err ("DescrSetInstanceWriter::begin() => invalid DescrSetInstance handle\n");
        DBGBREAK;
    }

    return *this;
}

//***************************** 
gpu::DescrSetInstanceWriter& gpu::DescrSetInstanceWriter::bindUniformBuffer (u32 binding, const GPUUniformBufferHandle &handle, u32 dstArrayElem)
{
    if (bAnyError)
        return *this;

    if (bufferList.num >= GOSGPU__NUM_MAX_DESCRIPTOR_PER_SET)
    {
        gos::logger::err ("DescrSetInstanceWriter::bindUniformBuffer() => too many descr buffer info\n");
        return *this;
    }
    
    const gpu::Buffer *buffer = gpu->getInfo(handle);
    if (NULL == buffer)
    {
        gos::logger::err ("DescrSetInstanceWriter::bindUniformBuffer() => invalid uniform buffer handle\n");
        return *this;
    }        
 
    //buffer descr
    VkDescriptorBufferInfo *p = &bufferList.list[bufferList.num++];
    memset (p, 0, sizeof(VkDescriptorBufferInfo));
    p->buffer = buffer->vkHandle;
    p->offset = 0;
    p->range = buffer->bufferSize;


    //write descr
    VkWriteDescriptorSet *d = &writeDescrList[numWriteDescr++];
    memset (d, 0, sizeof(VkWriteDescriptorSet));
    d->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    d->dstSet = descrSetInstHandle->vkHandle;
    d->dstBinding = binding;
    d->dstArrayElement = dstArrayElem;

    d->descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    d->descriptorCount = 1;    
    d->pBufferInfo = p;

    return *this;    
}

//***************************** 
gpu::DescrSetInstanceWriter& gpu::DescrSetInstanceWriter::bindStorageBuffer (u32 binding, const GPUStorageBufferHandle &handle, u32 dstArrayElem)
{
    if (bAnyError)
        return *this;
    
    const gpu::Buffer *buffer = gpu->getInfo(handle);
    if (NULL == buffer)
    {
        gos::logger::err ("DescrSetInstanceWriter::bindStorageBuffer() => invalid uniform buffer handle\n");
        return *this;
    }        

    return priv_bindBuffer (binding, buffer->vkHandle, buffer->bufferSize, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, dstArrayElem);
}

//***************************** 
gpu::DescrSetInstanceWriter& gpu::DescrSetInstanceWriter::bindDynamicStorageBuffer (u32 binding, const GPUStorageBufferHandle &handle, u32 sizeOfOneElement)
{
    if (bAnyError)
        return *this;
    
    const gpu::Buffer *buffer = gpu->getInfo(handle);
    if (NULL == buffer)
    {
        gos::logger::err ("DescrSetInstanceWriter::bindDynamicStorageBuffer() => invalid uniform buffer handle\n");
        return *this;
    }        

    return priv_bindBuffer (binding, buffer->vkHandle, sizeOfOneElement, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 0);
}

//***************************** 
gpu::DescrSetInstanceWriter& gpu::DescrSetInstanceWriter::priv_bindBuffer (u32 binding, const VkBuffer &vkBufferHandle, u32 bufferSize, VkDescriptorType descriptorType, u32 dstArrayElem)
{
    if (bAnyError)
        return *this;

    if (bufferList.num >= GOSGPU__NUM_MAX_DESCRIPTOR_PER_SET)
    {
        gos::logger::err ("DescrSetInstanceWriter::priv_bindBuffer() => too many descr buffer info\n");
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
    d->dstSet = descrSetInstHandle->vkHandle;
    d->dstBinding = binding;
    d->dstArrayElement = dstArrayElem;

    d->descriptorType = descriptorType;
    d->descriptorCount = 1;    
    d->pBufferInfo = p;

    return *this;    
}

//***************************** 
gpu::DescrSetInstanceWriter& gpu::DescrSetInstanceWriter::bindCombinedTextureAndSampler (u32 binding, const GPUTextureHandle &texHandle, const GPUSamplerHandle &samplerHandle, u32 dstArrayElem)
{
    if (bAnyError)
        return *this;

    if (imageList.num >= GOSGPU__NUM_MAX_DESCRIPTOR_PER_SET)
    {
        gos::logger::err ("DescrSetInstanceWriter::bindCombinedTextureAndSampler() => too many image buffer info\n");
        return *this;
    }
    
    VkImageView vkImgView;
    if (!gpu->toVulkan (texHandle, &vkImgView))
    {
        gos::logger::err ("DescrSetInstanceWriter::bindCombinedTextureAndSampler() => invalid texture handle\n");
        return *this;
    }        

    const gpu::Sampler *sampler = gpu->getInfo (samplerHandle);
    if (NULL == sampler)
    {
        gos::logger::err ("DescrSetInstanceWriter::bindTextureAndSampler() => invalid sampler handle\n");
        return *this;
    }        

    //image descr
    VkDescriptorImageInfo *p = &imageList.list[imageList.num++];
    memset (p, 0, sizeof(VkDescriptorImageInfo));
    p->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    p->imageView = vkImgView;
    p->sampler = sampler->vkHandle;


    //write descr
    VkWriteDescriptorSet *d = &writeDescrList[numWriteDescr++];
    memset (d, 0, sizeof(VkWriteDescriptorSet));
    d->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    d->dstSet = descrSetInstHandle->vkHandle;
    d->dstBinding = binding;
    d->dstArrayElement = dstArrayElem;

    d->descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    d->descriptorCount = 1;    
    d->pImageInfo = p;

    return *this;    
}


//***************************** 
gpu::DescrSetInstanceWriter& gpu::DescrSetInstanceWriter::bindSamplerInArray  (u32 binding, const GPUSamplerHandle &handle, u32 dstArrayElem)
{
    if (bAnyError)
        return *this;
    
    if (imageList.num >= GOSGPU__NUM_MAX_DESCRIPTOR_PER_SET)
    {
        gos::logger::err ("DescrSetInstanceWriter::bindSamplerInArray() => too many image buffer info\n");
        return *this;
    }

    const gpu::Sampler *sampler = gpu->getInfo(handle);
    if (NULL == sampler)
    {
        gos::logger::err ("DescrSetInstanceWriter::bindSamplerInArray() => invalid sampler handle\n");
        return *this;
    }
    
    //image descr
    VkDescriptorImageInfo *p = &imageList.list[imageList.num++];
    memset (p, 0, sizeof(VkDescriptorImageInfo));
    p->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    p->sampler = sampler->vkHandle;    

    //write descr
    VkWriteDescriptorSet *d = &writeDescrList[numWriteDescr++];
    memset (d, 0, sizeof(VkWriteDescriptorSet));
    d->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    d->dstSet = descrSetInstHandle->vkHandle;
    d->dstBinding = binding;
    d->dstArrayElement = dstArrayElem;

    d->descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    d->descriptorCount = 1;    
    d->pImageInfo = p;
    return *this;    
}

//***************************** 
gpu::DescrSetInstanceWriter& gpu::DescrSetInstanceWriter::bindTextureInArray (u32 binding, const GPUTextureHandle &handle, u32 dstArrayElem)
{
    if (bAnyError)
        return *this;

    if (imageList.num >= GOSGPU__NUM_MAX_DESCRIPTOR_PER_SET)
    {
        gos::logger::err ("DescrSetInstanceWriter::bindTextureInArray() => too many image buffer info\n");
        return *this;
    }
    
    VkImageView vkImgView;
    if (!gpu->toVulkan (handle, &vkImgView))
    {
        gos::logger::err ("DescrSetInstanceWriter::bindTextureInArray() => invalid texture handle\n");
        return *this;
    }        

    //image descr
    VkDescriptorImageInfo *p = &imageList.list[imageList.num++];
    memset (p, 0, sizeof(VkDescriptorImageInfo));
    p->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    p->imageView = vkImgView;


    //write descr
    VkWriteDescriptorSet *d = &writeDescrList[numWriteDescr++];
    memset (d, 0, sizeof(VkWriteDescriptorSet));
    d->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    d->dstSet = descrSetInstHandle->vkHandle;
    d->dstBinding = binding;
    d->dstArrayElement = dstArrayElem;

    d->descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
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


    gpu->_getVulkanDevice()->descriptorSet_update (numWriteDescr, writeDescrList, 0, nullptr);
    return true;
}

