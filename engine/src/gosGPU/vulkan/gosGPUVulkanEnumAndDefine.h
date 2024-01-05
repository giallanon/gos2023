#ifndef _gosGPUVulkanEnumAndDefine_h_
#define _gosGPUVulkanEnumAndDefine_h_
#define _GLFW_X11
#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define VK_ENABLE_BETA_EXTENSIONS
#include "GLFW/glfw3.h"
#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>
#include "../../gos/gosEnumAndDefine.h"
//#include "../../gos/gos.h"


static constexpr u8 SWAPCHAIN_NUM_MAX_IMAGES = 8;

namespace gos
{
    enum class eVulkanQueueType : u8
    {
        gfx = 0,
        compute = 1,
        transfer = 2
    };

    struct sSwapChainInfo
    {
                sSwapChainInfo()            { reset(); }
        
        void    reset()
                {
                    imageCount=0; 
                    vkSwapChain=VK_NULL_HANDLE; 
                    for (u8 i=0;i<SWAPCHAIN_NUM_MAX_IMAGES;i++)
                    { 
                        vkImageList[i]=VK_NULL_HANDLE; 
                        vkImageListView[i]=VK_NULL_HANDLE;
                        //frameBuffers[i]=VK_NULL_HANDLE;
                    } 
                }

        void    destroy(VkDevice &vkDevice)
                {
                    for (u8 i=0;i<imageCount;i++)
                    {
                        if (VK_NULL_HANDLE != vkImageListView[i])
                            vkDestroyImageView(vkDevice, vkImageListView[i], nullptr);
                    }
                    if (VK_NULL_HANDLE != vkSwapChain)
                        vkDestroySwapchainKHR(vkDevice, vkSwapChain, nullptr);
                    reset();
                }
        
        VkSwapchainKHR      vkSwapChain;
        VkFormat            imageFormat;
        VkExtent2D          imageExtent;
        VkColorSpaceKHR     colorSpace;
        u32                 imageCount;
        VkImage             vkImageList[SWAPCHAIN_NUM_MAX_IMAGES];
        VkImageView         vkImageListView[SWAPCHAIN_NUM_MAX_IMAGES];
    };

    struct sPhyDeviceInfo
    {
    public:
                sPhyDeviceInfo()            { reset(); }
        void    reset()                     { vkDev=VK_NULL_HANDLE; devIndex=u32MAX; queue_gfx.reset(); queue_compute.reset(); queue_transfer.reset(); }
        bool    isValid() const             { return (vkDev!=VK_NULL_HANDLE); }

    public:
        struct sQueueInfo
        {
            sQueueInfo()        { reset(); }
            void reset()        { familyIndex=u32MAX; count=0; }
            
            u32 familyIndex;
            u32 count;
        };

    public:
        VkPhysicalDevice    vkDev;
        u32                 devIndex;
        sQueueInfo          queue_gfx;
        sQueueInfo          queue_compute;
        sQueueInfo          queue_transfer;
        VkPhysicalDeviceMemoryProperties vkMemoryProperties;
    };

    struct sVkDevice
    {
    public:
        struct sQueueInfo
        {
        public:
            void reset()        { vkQueueHandle = VK_NULL_HANDLE; vkPoolHandle=VK_NULL_HANDLE; bIsUnique=false; familyIndex=u32MAX; }

        public:
            VkQueue         vkQueueHandle;
            VkCommandPool   vkPoolHandle;
            u32             familyIndex;
            bool            bIsUnique;
        };

    public:
                    sVkDevice()         { reset(); }

        void        reset()             
                    {
                        phyDevInfo.reset(); 
                        dev=VK_NULL_HANDLE;
                        swapChainInfo.reset();
                        for (u8 i=0; i<NUM_Q; i++)
                            _queueList[i].reset();
                    }

        void        destroy()
                    {
                        if (VK_NULL_HANDLE == dev)
                            return;

                        for (u8 i=0; i<NUM_Q; i++)
                        {
                            if (_queueList[i].bIsUnique)
                            {
                                if (VK_NULL_HANDLE != _queueList[i].vkPoolHandle)
                                {
                                    vkDestroyCommandPool (dev, _queueList[i].vkPoolHandle, nullptr);
                                    _queueList[i].vkPoolHandle = VK_NULL_HANDLE;
                                }
                            }
                        }

                        swapChainInfo.destroy(dev);

                        vkDestroyDevice(dev, nullptr);
                        reset();
                    }

        sQueueInfo* getQueueInfo (eVulkanQueueType type)
        {
            switch (type)
            {
            default:
                DBGBREAK;
                return &_queueList[0];
            case eVulkanQueueType::gfx:         return &_queueList[0];
            case eVulkanQueueType::compute:     return &_queueList[1];
            case eVulkanQueueType::transfer:    return &_queueList[2];
            }
        }

    private:
        static const u8 NUM_Q = 3;

    public:
        sPhyDeviceInfo      phyDevInfo;
        sSwapChainInfo      swapChainInfo;
        VkDevice            dev;
        sQueueInfo          _queueList[NUM_Q];
        
    };

}  //namespace gos

#endif //_gosGPUVulkanEnumAndDefine_h_