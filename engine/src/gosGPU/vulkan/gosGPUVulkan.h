#ifndef _gosGPUVulkan_h_
#define _gosGPUVulkan_h_
#include "gosGPUVulkanEnumAndDefine.h"
#include "../../gos/string/gosStringList.h"

namespace gos
{
    /*********************************************
     * [requiredValidationLayerList] e' una lista di stringhe separate da virgola che contiene un elenco di validation layer da attivare
     *                              es: VK_LAYER_KHRONOS_validation,VK_LAYER_LUNARG_monitor
     * [requiredExtensionList] come sopra, ma per le estensioni
     */
    bool    vulkanCreateInstance (VkInstance *out, const gos::StringList &requiredValidationLayerList, const gos::StringList &requiredExtensionList, eVulkanVersion vulkanVersion);

    bool    vulkanScanAndSelectAPhysicalDevices (const VkInstance &vkInstance, const VkSurfaceKHR &vkSurfaceKHR, const gos::StringList &requiredExtensionList, eVulkanVersion vulkanVersion, sPhyDeviceInfo *out);

    /*********************************************
     * Dato il [vkPhyDevice] e una lista di estensioni richieste [requiredExtensionList], crea il device logico
     * create le queue e filla out_vulkan con queste informazioni
     */
    bool    vulkanCreateSwapChain (VulkanDevice &vulkan, const VkSurfaceKHR &vkSurfaceKHR, bool bVSync, sSwapChainInfo *out);

    bool    vulkanFindBestDepthOnlyFormat (const sPhyDeviceInfo &vkPhyDevInfo, VkFormat *out_depthFormat);

    bool    vulkanFindBestDepthStencilFormat (const sPhyDeviceInfo &vkPhyDevInfo, VkFormat* out_depthStencilFormat);

    bool    vulkanCreateBuffer (VulkanDevice &vulkan, u32 sizeInByte, 
                                VkBufferUsageFlags usage, 
                                VkMemoryPropertyFlags memProperties,
                                bool bCanBeUsedBy_gfxQ, bool bCanBeUsedBy_computeQ, bool bCanBeUsedBy_transferQ,
                                VkBuffer *out_vkBufferHandle, VkDeviceMemory *out_vkMemHandle, u64 *out_realMemAllocated);

    bool    vulkanCreateCommandBuffer (VulkanDevice &vulkan, eGPUQueueFamily whichQ, u32 threadID, VkCommandPool *out_pool, VkCommandBuffer *out_handle);
    bool    vulkanDeleteCommandBuffer (VulkanDevice &vulkan, eGPUQueueFamily whichQ, VkCommandPool vkPool, VkCommandBuffer &vkHandle);

    bool    vulkanCreateImage2D (VulkanDevice &vulkan, u32 dimx, u32 dimy, u8 numMipMap, VkFormat fmt, eMemAccessMode memAccessMode, 
                                VkImageUsageFlags usage, VkImage *out_imagehandle, VkDeviceMemory *out_vkMemHandle, u32 *out_sizeInByte);


} //namespace gos

#endif //_gosGPUVulkan_h_