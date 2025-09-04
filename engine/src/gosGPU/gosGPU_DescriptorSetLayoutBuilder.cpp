#include "gosGPU.h"

using namespace gos;


typedef GPU::DescriptorSetLayoutBuilder  GPUDSLB;   //di comodo

//*********************************************** 
GPU::DescriptorSetLayoutBuilder::DescriptorSetLayoutBuilder (GPU *gpuIN, VkDescriptorSetLayoutCreateFlags createFlagIN, GPUDescrSetLayoutHandle *out_handleIN) :
    GPU::TempBuilder(gpuIN)
{
    out_handle = out_handleIN;
    bAnyError = false;
    nextBindingNumber = 0;
    numDescriptor = 0;
    createFlag = createFlagIN;
}

//*********************************************** 
GPU::DescriptorSetLayoutBuilder::~DescriptorSetLayoutBuilder()
{
}

//*********************************************** 
GPUDSLB& GPU::DescriptorSetLayoutBuilder::add (eGPUDescriptrorType descrType, u32 usageFlags, u32 count)
{
    switch (descrType)
    {
    default:                                                break;
    case eGPUDescriptrorType::UNIFORM_BUFFER:               return priv_add (VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, usageFlags, count);
    case eGPUDescriptrorType::DYNAMIC_UNIFORM_BUFFER:       return priv_add (VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, usageFlags, count);
    case eGPUDescriptrorType::STORAGE_BUFFER:               return priv_add (VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, usageFlags, count);
    case eGPUDescriptrorType::DYNAMIC_STORAGE_BUFFER:       return priv_add (VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, usageFlags, count);
    case eGPUDescriptrorType::COMBINED_IMAGE_SAMPLER:       return priv_add (VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, usageFlags, count);
    case eGPUDescriptrorType::SAMPLER:                      return priv_add (VK_DESCRIPTOR_TYPE_SAMPLER, usageFlags, count);
    case eGPUDescriptrorType::TEXTURE2D:                    return priv_add (VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, usageFlags, count);
    }

    DBGBREAK;
    return *this;
}

//*********************************************** 
GPUDSLB& GPU::DescriptorSetLayoutBuilder::priv_add (VkDescriptorType descrType, VkShaderStageFlags stageFlags, u32 count)
{
    if (numDescriptor >= GOSGPU__NUM_MAX_DESCRIPTOR_PER_SET)
    {
        bAnyError = true;
        gos::logger::err ("GPU::DescriptorSetLayoutBuilder::add() => too many descriptor. Max is %d\n", GOSGPU__NUM_MAX_DESCRIPTOR_PER_SET);
    }
    else
    {
        list[numDescriptor].binding = nextBindingNumber;
        list[numDescriptor].descriptorType = descrType;
        list[numDescriptor].descriptorCount = count;
        list[numDescriptor].stageFlags = stageFlags;
        list[numDescriptor].pImmutableSamplers = nullptr; // Optional

        ++nextBindingNumber;
        ++numDescriptor;
    }

    return *this;
}

//*********************************************** 
bool GPU::DescriptorSetLayoutBuilder::end()
{
    if (numDescriptor == 0)
    {
        bAnyError = true;
        gos::logger::err ("GPU::DescriptorSetLayoutBuilder::end() => num numDescriptor can't be 0\n");
    }

    return gpu->priv_descrSetLayout_onBuilderEnds (this);
}

