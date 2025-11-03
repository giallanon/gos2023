#ifndef _gosGPUVulkanQFamily_h_
#define _gosGPUVulkanQFamily_h_
#include "gosGPUVulkanEnumAndDefine.h"

namespace gos
{
    /*************************************
    * @brief    VulkanQFamily
    * 
    */
    class VulkanQFamily
    {
    public:
                            VulkanQFamily();
                            ~VulkanQFamily()                                                  { unsetup(); }

        void                setup (VkDevice vkDev, eGPUQueueFamily familyType, u32 familyIndex);
        void                unsetup();

        void                waitIdle ();
        VkResult            submit(u32 submitCount, const VkSubmitInfo *submitInfo, VkFence fence);

        VkCommandPool       getOrCreateCommandPool (u32 threadID);
        void                deleteCommandPool (VkCommandPool vkPool);

        bool                commandBuffer_create (u32 threadID, VkCommandPool *out_pool, VkCommandBuffer *out_handle);
        void                commandBuffer_delete (VkCommandPool vkPool, VkCommandBuffer vkHandle);

        u8                  getNativeFamilyIndex() const                                        { return familyIndex; }
        eGPUQueueFamily     getNativeFamilyType() const                                         { return familyType; }
        VkQueue             getHandle() const                                                   { return vkQueueHandle; }

    private:
        static constexpr u8 NUM_MAX_THREAD  = 16;

    private:
        struct sPool
        {
            u32             threadID;
            VkCommandPool   vkPoolHandle;
        };

    private:
        VkQueue             vkQueueHandle;
        VkDevice            vkDev;
        gos::Mutex          mutex;
        sPool               poolList[NUM_MAX_THREAD];      //un cmdPool per ogni possibile thread
        u8                  familyIndex;
        eGPUQueueFamily     familyType;
    };
} //namespace gos

#endif //_gosGPUVulkanQFamily_h_