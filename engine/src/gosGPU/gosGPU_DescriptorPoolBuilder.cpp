#include "gosGPU.h"

using namespace gos;


typedef GPU::DescriptorPoolBuilder  GPUDPOOLB;   //di comodo

//*********************************************** 
GPU::DescriptorPoolBuilder::DescriptorPoolBuilder (GPU *gpuIN, GPUDescrPoolHandle *out_handleIN) :
    GPU::TempBuilder(gpuIN)
{
    out_handle = out_handleIN;
    bAnyError = false;
    numMaxDescriptorSets = 0;
    numPool = 0;
    vkPoolFlags = 0;
}

//*********************************************** 
GPU::DescriptorPoolBuilder::~DescriptorPoolBuilder()
{
}

//*********************************************** 
VkDescriptorPoolSize* GPU::DescriptorPoolBuilder::priv_findOrAddByDescrType (VkDescriptorType descrType, u32 howMany)
{
    for (u32 i=0; i<numPool; i++)
    {
        if (list[i].type == descrType)
            return &list[i];
    }

    if (numPool >= GOSGPU__NUM_MAX_DESCRIPTOR_POOL_SIZE_PER_POOL)
    {
        bAnyError = true;
        gos::logger::err ("GPU::DescriptorPoolBuilder::priv_findOrAddByDescrType() => too many descriptorPool. Max is %d\n", GOSGPU__NUM_MAX_DESCRIPTOR_POOL_SIZE_PER_POOL);
        return NULL;
    }

    list[numPool].type = descrType;
    list[numPool].descriptorCount = howMany;
    return &list[numPool++];
}

//*********************************************** 
GPUDPOOLB& GPU::DescriptorPoolBuilder::addPool_uniformBuffer(u32 howMany)
{
    VkDescriptorPoolSize *p = priv_findOrAddByDescrType (VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, howMany);
    if (p)
    {
        p->descriptorCount++;
    }

    return *this;
}


//*********************************************** 
GPUDPOOLB& GPU::DescriptorPoolBuilder::addPool_storageBuffer(u32 howMany)
{
    VkDescriptorPoolSize *p = priv_findOrAddByDescrType (VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, howMany);
    if (p)
    {
        p->descriptorCount++;
    }

    return *this;
}

//*********************************************** 
GPUDPOOLB& GPU::DescriptorPoolBuilder::addPool_textureSampler(u32 howMany)
{
{
    VkDescriptorPoolSize *p = priv_findOrAddByDescrType (VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, howMany);
    if (p)
    {
        p->descriptorCount++;
    }

    return *this;
}
}

//*********************************************** 
bool GPU::DescriptorPoolBuilder::end()
{
    if (0 == numPool)
    {
        bAnyError = true;
        gos::logger::err ("GPU::DescriptorPoolBuilder::end() => numPool can't be 0\n");
    }

    if (0 == numMaxDescriptorSets)
    {
        bAnyError = true;
        gos::logger::err ("GPU::DescriptorPoolBuilder::end() => numMaxDescriptorSets can't be 0\n");
    }

    return gpu->priv_descrPool_onBuilderEnds (this);
}

