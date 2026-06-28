#ifndef _gosGPUVulkanDevice_h_
#define _gosGPUVulkanDevice_h_
#include "gosGPUVulkanEnumAndDefine.h"
#include "gosGPUVulkanQFamily.h"
#include "../gosGPUEnumAndDefine.h"

namespace gos
{
   /*************************************
    * @brief    VulkanDevice
    * 
    */
    class VulkanDevice
    {

    public:
                                VulkanDevice()                                          { priv_reset(); }
                                ~VulkanDevice()                                         { unsetup(); }

        bool                    setup (const sPhyDeviceInfo &phyInfo, const gos::StringList &requiredExtensionList, eVulkanVersion vulkanVersion);
        void                    unsetup();
        
        //======================= query
        bool                    findBestDepthOnlyFormat (VkFormat *out_depthFormat) const;
        bool                    findBestDepthStencilFormat (VkFormat* out_depthStencilFormat) const;
        bool                    isImage2DFmtSupported (eImageFormat fmt, eImageTiling tiling) const;

        
        u32                     limits_get_maxSamplerAnisotropy() const             { return static_cast<u32>(phyDevInfo.deviceProperties.limits.maxSamplerAnisotropy); }
        
        u32                     limits_get_maxDescriptorSetSampledImages() const    { return static_cast<u32>(phyDevInfo.deviceProperties.limits.maxDescriptorSetSampledImages); }
        
        u32                     limits_get_minUniformBufferOffsetAlignment() const  { return static_cast<u32>(phyDevInfo.deviceProperties.limits.minUniformBufferOffsetAlignment); }
        u32                     limits_get_maxUniformBufferRange() const            { return static_cast<u32>(phyDevInfo.deviceProperties.limits.maxUniformBufferRange); }

        u32                     limits_get_minStorageBufferOffsetAlignment() const  { return static_cast<u32>(phyDevInfo.deviceProperties.limits.minStorageBufferOffsetAlignment); }
        u32                     limits_get_maxStorageBufferRange() const            { return static_cast<u32>(phyDevInfo.deviceProperties.limits.maxStorageBufferRange); }
        u32                     limits_get_nonCoherentAtomSize() const              { return static_cast<u32>(phyDevInfo.deviceProperties.limits.nonCoherentAtomSize); }

        //===================== queue
        void                    waitIdle()                                                                                                      { vkDeviceWaitIdle(vkDev); }
        void                    queue_waitIdle (eGPUQueueFamily whichOne)                                                                       { getQFamily(whichOne)->waitIdle(); }
        VkResult                queue_submit (eGPUQueueFamily whichOne, u32 submitCount, const VkSubmitInfo *submitInfo, VkFence fence)         { return getQFamily(whichOne)->submit(submitCount, submitInfo, fence); }
        VulkanQFamily*          getQFamily (eGPUQueueFamily type)                       { return &qfamilyList[priv_from_family_to_index(type)]; }
        const VulkanQFamily*    getQFamily (eGPUQueueFamily type) const                 { return &qfamilyList[priv_from_family_to_index(type)]; }

        //==================== sync object
        bool                    semaphore_create  (VkSemaphore *out);
        void                    semaphore_destroy  (VkSemaphore &in);
        
        bool                    fence_create  (bool bStartAsSignaled, VkFence *out);
        void                    fence_destroy  (VkFence &in);

        //ritorna true se il [fence] e' segnalato, false se timeout
        bool                    fence_wait (const VkFence fenceHandle, u64 timeout_ns = UINT64_MAX);
        bool                    fence_waitMany (const VkFence *fenceHandleList, bool bWaitForAll, u32 fenceCount, u64 timeout_ns = UINT64_MAX);

        //riporta [fence] in stato non segnalato
        void                    fence_reset (const VkFence fenceHandle);
        void                    fence_resetMany (const VkFence *enceHandleList, u32 fenceCount);

        bool                    fence_isSignaled  (const VkFence fenceHandle);

        //========================== swapchain
        bool                    swapchain_create (const VkSurfaceKHR &vkSurfaceKHR, bool bVSync, sSwapChainInfo *out);
        void                    swapchain_delete (sSwapChainInfo &s);
        VkResult                swapChain_acquireImage (sSwapChainInfo &swapchain, u64 timeout_ns, VkSemaphore semaphore, VkFence fence, u32 *out_imageIndex);
        VkResult                swapChain_present (sSwapChainInfo &swapchain, const VkSemaphore *semaphoreHandleList, u32 semaphoreCount, u32 imageIndex);


        //========================== memory
        VkResult                memory_map (VkDeviceMemory vkMemHandle, u32 offset, u32 sizeInByte, VkMemoryMapFlags flags, void** out_p) const;
        void                    memory_unmap (VkDeviceMemory vkMemHandle);
        void                    memory_invalidateRanges (u32 numRanges, const VkMappedMemoryRange *rangeList);
        void                    memory_flushRanges (u32 numRanges, const VkMappedMemoryRange *rangeList);

        //=========================== command buffer
        bool                    commandBuffer_create (eGPUQueueFamily whichQ, u32 threadID, VkCommandPool *out_pool, VkCommandBuffer *out_handle);
        void                    commandBuffer_delete (eGPUQueueFamily whichQ, VkCommandPool vkPool, VkCommandBuffer vkHandle);

        //=========================== shader
        VkResult                shader_create (const void *bufferIN, u32 bufferSize, VkShaderModule *out);
        void                    shader_delete (VkShaderModule vkHandle);

        //============================= images
        bool                    image_create2D (u32 dimx, u32 dimy, u8 numMipMap, VkFormat fmt, eMemAccessMode memAccessMode, VkImageUsageFlags usage, VkImage *out_imagehandle, VkDeviceMemory *out_vkMemHandle, u32 *out_sizeInByte);
        void                    image_delete (VkImage vkHandle, VkDeviceMemory vkMemHandle, u32 memoryAllocated);
        void                    image_getSubresourceLayout (VkImage image, const VkImageSubresource *pSubresource, VkSubresourceLayout *pLayout) const;


        VkResult                imageView_create (const VkImageViewCreateInfo &createInfo, VkImageView *out_view);
        void                    imageView_delete (VkImageView vkHandle);

        //=============================  buffer
        bool                    buffer_create (u32 sizeInByte, VkBufferUsageFlags usage, VkMemoryPropertyFlags memProperties,
                                                bool bCanBeUsedBy_gfxQ, bool bCanBeUsedBy_computeQ, bool bCanBeUsedBy_transferQ,
                                                VkBuffer *out_vkBufferHandle, VkDeviceMemory *out_vkMemHandle, u32 *out_realMemAllocated);
        void                    buffer_delete (VkBuffer vkBufferHandle, VkDeviceMemory vkMemHandle, u32 realMemAllocated);

        //=============================  descriptor
        VkResult                descPool_create (const VkDescriptorPoolCreateInfo &creat, VkDescriptorPool *out);
        void                    descPool_delete (VkDescriptorPool vkHandle);

        VkResult                descSetLayout_create (const VkDescriptorSetLayoutCreateInfo &creat, VkDescriptorSetLayout *out);
        void                    descSetLayout_delete (VkDescriptorSetLayout vkHandle);

        VkResult                descriptorSet_create (VkDescriptorPool vkPoolHandle, VkDescriptorSetLayout vkDescSetLayoutHandle, VkDescriptorSet *out);
        void                    descriptorSet_delete (VkDescriptorPool vkPoolHandle, VkDescriptorSet vkHandle);
        void                    descriptorSet_update (u32 descriptorWriteCount, const VkWriteDescriptorSet *pDescriptorWrites, u32 descriptorCopyCount, const VkCopyDescriptorSet *pDescriptorCopies) const;

        //=============================  sampler
        VkResult                sampler_create (const VkSamplerCreateInfo &creat, VkSampler *out);
        void                    sampler_delete (VkSampler vkHandle);

        //=============================  pipeline
        VkResult                pipelineLayout_create (const VkPipelineLayoutCreateInfo &creat, VkPipelineLayout *out);
        void                    pipelineLayout_delete (VkPipelineLayout vkHandle);

        VkResult                pipeline_create (const VkGraphicsPipelineCreateInfo &creat, VkPipeline *out);
        void                    pipeline_delete (VkPipeline vkHandle);

    private:
        static constexpr u8 NUM_MAX_QFAMILY = static_cast<u8>(eGPUQueueFamily::_NUM);

    private:
        void                priv_reset();
        u8                  priv_from_family_to_index (eGPUQueueFamily type) const          { return map_qfamily_to_q[static_cast<u8>(type)]; }
        void                priv_addNativeQFamily (eGPUQueueFamily familyType, u32 familyIndex);
        bool                priv_getMemoryType (uint32_t typeBits, VkMemoryPropertyFlags properties, u32 *out_index);
        bool                priv_allocMemory (const VkMemoryAllocateInfo *pAllocateInfo, VkDeviceMemory *pMemory, const char *debug_from_who);
		bool                priv__do_allocMemory (const VkMemoryAllocateInfo *pAllocateInfo, VkDeviceMemory *pMemory, const char *debug_from_who);
        void                priv_freeMemory (VkDeviceMemory memory, u64 memSize);
		void                priv__do_freeMemory (VkDeviceMemory memory, u64 memSize);

    private:
        sPhyDeviceInfo      phyDevInfo;
        VkDevice            vkDev;
        u64                 memory_maxAllocated;
        u64                 memory_curAllocated;
        VulkanQFamily       qfamilyList[NUM_MAX_QFAMILY];
        u8                  map_qfamily_to_q[NUM_MAX_QFAMILY];
        u8                  numQFamily;
		gos::Mutex			mutex_memAlloc;
    };    
} //namespace gos

#endif //_gosGPUVulkanDevice_h_

