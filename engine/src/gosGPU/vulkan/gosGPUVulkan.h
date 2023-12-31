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
    bool    vulkanCreateInstance (VkInstance *out, const gos::StringList &requiredValidationLayerList, const gos::StringList &requiredExtensionList);

    bool    vulkanScanAndSelectAPhysicalDevices (VkInstance &vkInstance, const VkSurfaceKHR &vkSurface, const gos::StringList &requiredExtensionList, sPhyDeviceInfo *out);

    /*********************************************
     * Dato il [vkPhyDevice] e una lista di estensioni richieste [requiredExtensionList], crea il device logico
     * create le queue e filla out_vulkan con queste informazioni
     */
    bool    vulkanCreateDevice (sPhyDeviceInfo &vkPhyDevInfo, const gos::StringList &requiredExtensionList, sVkDevice *out_vulkan);

    bool    vulkanCreateSwapChain (sVkDevice &vulkan, const VkSurfaceKHR &vkSurface, sSwapChainInfo *out);

} //namespace gos

#endif //_gosGPUVulkan_h_