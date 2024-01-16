#include "gosGPUDescrSetInstanceWriter.h"
#include "gosGPU.h"


using namespace gos;


//***************************** 
gpu::DescrSetInstanceWriter::DescrSetInstanceWriter()
{
    gpu = NULL;
}

//***************************** 
gpu::DescrSetInstanceWriter& gpu::DescrSetInstanceWriter::begin (gos::GPU *gpuIN, const GPUDescrSetInstancerHandle &descrSetInstanceHandle)
{
    bAnyError = false;
    gpu = gpuIN;
    numDescrBufferInfo = 0;
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
gpu::DescrSetInstanceWriter& gpu::DescrSetInstanceWriter::updateUniformBuffer (u32 binding, const GPUUniformBufferHandle &handle)
{
    if (bAnyError)
        return *this;

    if (numDescrBufferInfo >= GOSGPU__NUM_MAX_DESCRIPTOR_PER_SET)
    {
        gos::logger::err ("DescrSetInstanceWriter::updateUniformBuffer() => too many descr buffer info\n");
        return *this;
    }
    
    VkBuffer vkBufferHandle;
    u32 bufferSize;
    if (!gpu->toVulkan (handle, &vkBufferHandle, &bufferSize))
    {
        gos::logger::err ("DescrSetInstanceWriter::updateUniformBuffer() => invalid uniform buffer handle\n");
        return *this;
    }        


    //buffer descr
    memset (&descrBufferInfoList[numDescrBufferInfo], 0, sizeof(VkDescriptorBufferInfo));
    descrBufferInfoList[numDescrBufferInfo].buffer = vkBufferHandle;
    descrBufferInfoList[numDescrBufferInfo].offset = 0;
    descrBufferInfoList[numDescrBufferInfo].range = bufferSize;


    //write descr
    memset (&writeDescrList[numWriteDescr], 0, sizeof(VkWriteDescriptorSet));
    writeDescrList[numWriteDescr].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescrList[numWriteDescr].dstSet = vkDescrSetHandle;
    writeDescrList[numWriteDescr].dstBinding = binding;
    writeDescrList[numWriteDescr].dstArrayElement = 0;

    writeDescrList[numWriteDescr].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writeDescrList[numWriteDescr].descriptorCount = 1;    
    writeDescrList[numWriteDescr].pBufferInfo = &descrBufferInfoList[numDescrBufferInfo];

    ++numDescrBufferInfo;
    ++numWriteDescr;

    return *this;    
}

//***************************** 
bool gpu::DescrSetInstanceWriter::end()
{
    if (0 == numDescrBufferInfo)
        bAnyError = true;

    if (bAnyError)
        return false;


    vkUpdateDescriptorSets (gpu->REMOVE_getVkDevice(), numWriteDescr, writeDescrList, 0, nullptr);
    return true;
}

