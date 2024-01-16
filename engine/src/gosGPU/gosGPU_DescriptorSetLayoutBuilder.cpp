#include "gosGPU.h"

using namespace gos;


typedef GPU::DescriptorSetLayoutBuilder  GPUDSLB;   //di comodo

//*********************************************** 
GPU::DescriptorSetLayoutBuilder::DescriptorSetLayoutBuilder (GPU *gpuIN, GPUDescrSetLayoutHandle *out_handleIN) :
    GPU::TempBuilder(gpuIN)
{
    out_handle = out_handleIN;
    bAnyError = false;
    nextBindingNumber = 0;
    numDescriptor = 0;
}

//*********************************************** 
GPU::DescriptorSetLayoutBuilder::~DescriptorSetLayoutBuilder()
{
}


//*********************************************** 
GPUDSLB& GPU::DescriptorSetLayoutBuilder::add (VkDescriptorType descrType, VkShaderStageFlagBits stageFlags, u32 count)
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

