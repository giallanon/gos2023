#ifndef _gosGPUVulkan_h_
#define _gosGPUVulkan_h_
#include "gosGPUVulkanDevice.h"
#include "../gosGPUEnumAndDefine.h"
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

} //namespace gos

#endif //_gosGPUVulkan_h_