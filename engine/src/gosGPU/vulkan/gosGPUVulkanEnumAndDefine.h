#ifndef _gosGPUVulkanEnumAndDefine_h_
#define _gosGPUVulkanEnumAndDefine_h_
#define _GLFW_X11
#define GLFW_INCLUDE_VULKAN
#define VK_ENABLE_BETA_EXTENSIONS
#include "GLFW/glfw3.h"
#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>
#include "../../gos/gosEnumAndDefine.h"
//#include "../../gos/gos.h"


static constexpr u8 SWAPCHAIN_NUM_MAX_IMAGES = 8;

namespace gos
{
    struct sSwapChainInfo
    {
                sSwapChainInfo()            { reset(); }
        
        void    reset()
                {
                    imageCount=0; 
                    vkSwapChain=VK_NULL_HANDLE; 
                    for (u8 i=0;i<SWAPCHAIN_NUM_MAX_IMAGES;i++)
                    { 
                        vkImage[i]=VK_NULL_HANDLE; 
                        vkImageView[i]=VK_NULL_HANDLE;
                        //frameBuffers[i]=VK_NULL_HANDLE;
                    } 
                }

        void    destroy(VkDevice &vkDevice)
                {
                    for (u8 i=0;i<imageCount;i++)
                    {
                        //if (VK_NULL_HANDLE != frameBuffers[i]) vkDestroyFramebuffer(vkDevice, frameBuffers[i], nullptr);
                        
                        if (VK_NULL_HANDLE != vkImageView[i])
                            vkDestroyImageView(vkDevice, vkImageView[i], nullptr);
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
        VkImage             vkImage[SWAPCHAIN_NUM_MAX_IMAGES];
        VkImageView         vkImageView[SWAPCHAIN_NUM_MAX_IMAGES];
        //VkFramebuffer       frameBuffers[N_MAX_IMAGES];
    };

    struct sPhyDeviceInfo
    {
                sPhyDeviceInfo()            { reset(); }
        void    reset()                     { vkDev=VK_NULL_HANDLE; devIndex=gfxQIndex=computeQIndex=u32MAX; }
        bool    isValid() const             { return (vkDev!=VK_NULL_HANDLE); }

        VkPhysicalDevice    vkDev;
        u32                 devIndex;
        u32                 gfxQIndex;
        u32                 computeQIndex;
    };

    struct sVkDevice
    {
    public:
                    sVkDevice()         { reset(); }
        void        reset()             { phyDevInfo.reset(); dev=VK_NULL_HANDLE; gfxQ=computeQ=VK_NULL_HANDLE; commandPool=VK_NULL_HANDLE; swapChainInfo.reset();}
        void        destroy()
                    {
                        if (VK_NULL_HANDLE == dev)
                            return;

                        if (VK_NULL_HANDLE != commandPool)
                            vkDestroyCommandPool (dev, commandPool, nullptr);

                        swapChainInfo.destroy(dev);

                        vkDestroyDevice(dev, nullptr);
                        reset();
                    }

    public:
        sPhyDeviceInfo      phyDevInfo;
        sSwapChainInfo      swapChainInfo;
        VkDevice            dev;
        VkQueue             gfxQ;
        VkQueue             computeQ;
        VkCommandPool       commandPool;
    };

}  //namespace gos

#endif //_gosGPUVulkanEnumAndDefine_h_