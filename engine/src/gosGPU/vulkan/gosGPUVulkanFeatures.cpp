#include "gosGPUVukanHelpers.h"
#include "../../../gos/gos.h"

using namespace gos;

/*****************************************+
 * Qui ci sono tutte le feature del device fisico che voglio attivare
 * 
 */
bool VkPhyDeviceFeatures::checkPhysicalDeviceFeatures (VkPhysicalDevice &vkDev, eVulkanVersion vulkanVersion)
{
    //recupero tutte le feature del device fisico
    VkPhyDeviceFeatures allFeatures;
    allFeatures.priv_getAllPhysicalDeviceFeatures (vkDev, vulkanVersion);

    bool ret = true;
    priv_reset(vulkanVersion);
    

#define CHECK(propName) if (!allFeatures.propName) { gos::logger::warn ("feature not supported: " #propName "\n"); ret = false; } else { this->propName = true; }

    if (vulkanVersion >= eVulkanVersion::v1_3)
    {
        CHECK(features13.synchronization2);
        CHECK(features13.dynamicRendering);
    }

    CHECK(features12.separateDepthStencilLayouts);
    CHECK(features12.runtimeDescriptorArray);
    CHECK(features12.descriptorBindingPartiallyBound);
    CHECK(features12.descriptorBindingVariableDescriptorCount);
    CHECK(features12.descriptorBindingUpdateUnusedWhilePending);
    CHECK(features12.descriptorBindingUniformBufferUpdateAfterBind);
    CHECK(features12.descriptorBindingSampledImageUpdateAfterBind);
    CHECK(features12.descriptorBindingStorageBufferUpdateAfterBind);
    CHECK(features12.shaderSampledImageArrayNonUniformIndexing);
    CHECK(features12.shaderStorageBufferArrayNonUniformIndexing);
    //CHECK(features12.shaderUniformBufferArrayNonUniformIndexing);

    //CHECK(features11.storageBuffer16BitAccess);
    //CHECK(features11.shaderDrawParameters);

    CHECK(features.features.imageCubeArray);
    CHECK(features.features.geometryShader);
    CHECK(features.features.tessellationShader);
    CHECK(features.features.depthClamp);
    CHECK(features.features.fillModeNonSolid);
    CHECK(features.features.samplerAnisotropy);

#undef CHECK

    return ret;
}   

//*****************************************+
void VkPhyDeviceFeatures::priv_getAllPhysicalDeviceFeatures (VkPhysicalDevice &vkDev, eVulkanVersion vulkanVersion)
{
    priv_reset (vulkanVersion);
    vkGetPhysicalDeviceFeatures2 (vkDev, &features);
}

//*******************************************
void VkPhyDeviceFeatures::priv_reset (eVulkanVersion vulkanVersion)
{

#define ADD(obj,struct_type)\
    {\
        memset (&obj, 0, sizeof(obj));\
        obj.sType = struct_type;\
        *next=&obj;\
        next=&obj.pNext;\
    }\


    memset (&features, 0, sizeof(features));
    features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;

    void **next = &features.pNext;
    if (vulkanVersion >= eVulkanVersion::v1_1)  ADD(features11,VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES);
    if (vulkanVersion >= eVulkanVersion::v1_2)  ADD(features12,VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES);
    if (vulkanVersion >= eVulkanVersion::v1_3)  ADD(features13,VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES);
}
