#include "gosGPUVukanHelpers.h"
#include "../gosGPU.h"
#include "../gosGPUUtils.h"
#include "../../gos/gos.h"

using namespace gos;




/*************************************************************************************+
 * 
 * VkInstanceValidationLayersList
 * 
 * 
 ************************************/
void VkInstanceValidationLayersList::build (gos::Allocator *allocatorIN)
{
    this->free();
    u32 nElem;
    vkEnumerateInstanceLayerProperties(&nElem, nullptr);
    if (nElem)
    {
        this->alloc (allocatorIN, nElem);
        vkEnumerateInstanceLayerProperties(&nElem, this->_getRawBuffer());
    }            
}
        
void VkInstanceValidationLayersList::printInfo () const
{
    gos::logger::log_3 ("available validation layers (%d):\n", getCount());
    gos::logger::inc_indent();
    for (u32 i=0; i<getCount(); i++)
        gos::logger::log_3 ("[%s]  ", get(i)->layerName);
    gos::logger::log_3 ("\n");
    gos::logger::dec_indent();    
}

u32 VkInstanceValidationLayersList::find (const char *layerName) const
{
    for (u32 i=0; i<getCount(); i++)
    {
        if (strcasecmp(layerName, get(i)->layerName) == 0)
            return i;
    }
    return u32MAX;
}


/*************************************************************************************+
 * 
 * VkInstanceExtensionList
 * 
 * 
 ************************************/
void VkInstanceExtensionList::build (gos::Allocator *allocatorIN)
{
    this->free();
    u32 nElem;
    vkEnumerateInstanceExtensionProperties(nullptr, &nElem, nullptr);
    if (nElem)
    {
        this->alloc (allocatorIN, nElem);
        vkEnumerateInstanceExtensionProperties(nullptr, &nElem, this->_getRawBuffer());
    }            
}

void VkInstanceExtensionList::printInfo () const
{
    gos::logger::log_3 ("available extensions (%d):\n", getCount());
    gos::logger::inc_indent();
    for (u32 i=0; i<getCount(); i++)
        gos::logger::log_3 ("[%s]  ", get(i)->extensionName);
    gos::logger::log_3 ("\n");
    gos::logger::dec_indent();
}            

u32 VkInstanceExtensionList::find (const char *extensionName) const
{
    for (u32 i=0; i<getCount(); i++)
    {
        if (strcasecmp(extensionName, get(i)->extensionName) == 0)
            return i;
    }
    return u32MAX;
}



/*************************************************************************************+
 * 
 * VkPhyDeviceExtensionList
 * 
 * 
 ************************************/
void VkPhyDeviceExtensionList::build (gos::Allocator *allocatorIN, VkPhysicalDevice &phyDevice)
{
    this->free();
    u32 nElem;
    vkEnumerateDeviceExtensionProperties(phyDevice, nullptr, &nElem, nullptr);
    if (nElem)
    {
        this->alloc (allocatorIN, nElem);
        vkEnumerateDeviceExtensionProperties(phyDevice, nullptr, &nElem, this->_getRawBuffer());
    }            
}

void VkPhyDeviceExtensionList::printInfo () const
{
    gos::logger::log_3 ("supported extensions:\n");
    gos::logger::inc_indent();
    for (u32 i2=0; i2<getCount(); i2++)
        gos::logger::log_3 ("[%s]  ", get(i2)->extensionName);
    gos::logger::log_3 ("\n");
    gos::logger::dec_indent();
}            

u32 VkPhyDeviceExtensionList::find (const char *extensionName) const
{
    for (u32 i=0; i<getCount(); i++)
    {
        if (strcasecmp(extensionName, get(i)->extensionName) == 0)
            return i;
    }
    return u32MAX;
}



/*************************************************************************************+
 * 
 * VkPhyDeviceQueueList
 * 
 * 
 ************************************/
void VkPhyDeviceQueueList::build (gos::Allocator *allocatorIN, VkPhysicalDevice &phyDevice)
{
    this->free();
    u32 nElem;
    vkGetPhysicalDeviceQueueFamilyProperties(phyDevice, &nElem, nullptr);
    if (nElem)
    {
        this->alloc (allocatorIN, nElem);
        vkGetPhysicalDeviceQueueFamilyProperties(phyDevice, &nElem, this->_getRawBuffer());
    }            
}

void VkPhyDeviceQueueList::printQueueFamilyInfo (u32 i) const
{
    gos::logger::log_3 ("count = %d\n", get(i)->queueCount);
    gos::logger::log_3 ("flags = ");
    if (support_VK_QUEUE_GRAPHICS_BIT(i)) gos::logger::log_3 ("[VK_QUEUE_GRAPHICS_BIT]  ");
    if (support_VK_QUEUE_COMPUTE_BIT(i)) gos::logger::log_3 ("[VK_QUEUE_COMPUTE_BIT]  ");
    if (support_VK_QVK_QUEUE_TRANSFER_BIT(i)) gos::logger::log_3 ("[VK_QUEUE_TRANSFER_BIT]  ");
    if (support_VK_QUEUE_SPARSE_BINDING_BIT(i)) gos::logger::log_3 ("[VK_QUEUE_SPARSE_BINDING_BIT]  ");
    if ((get(i)->queueFlags & VK_QUEUE_VIDEO_DECODE_BIT_KHR)) gos::logger::log_3 ("[VK_QUEUE_VIDEO_DECODE_BIT_KHR]  ");
    if ((get(i)->queueFlags & VK_QUEUE_VIDEO_ENCODE_BIT_KHR)) gos::logger::log_3 ("[VK_QUEUE_VIDEO_ENCODE_BIT_KHR]  ");
    if ((get(i)->queueFlags & VK_QUEUE_OPTICAL_FLOW_BIT_NV)) gos::logger::log_3 ("[VK_QUEUE_OPTICAL_FLOW_BIT_NV]  ");
    if ((get(i)->queueFlags & VK_QUEUE_PROTECTED_BIT)) gos::logger::log_3 ("[VK_QUEUE_PROTECTED_BIT]  ");
    if ((get(i)->queueFlags & VK_DEVICE_QUEUE_CREATE_PROTECTED_BIT)) gos::logger::log_3 ("[VK_DEVICE_QUEUE_CREATE_PROTECTED_BIT]  ");
    gos::logger::log_3 ("\n");
}



/*************************************************************************************+
 * 
 * VPhyDevicekSurfaceFormatKHRList
 * 
 * 
 ************************************/
void VPhyDevicekSurfaceFormatKHRList::build (gos::Allocator *allocatorIN, VkPhysicalDevice &phyDevice, const VkSurfaceKHR &vkSurface)
{
    this->free();
    u32 nElem;
    vkGetPhysicalDeviceSurfaceFormatsKHR(phyDevice, vkSurface, &nElem, nullptr);
    if (nElem)
    {
        this->alloc (allocatorIN, nElem);
        vkGetPhysicalDeviceSurfaceFormatsKHR(phyDevice, vkSurface, &nElem, this->_getRawBuffer());
    }            
}


bool VPhyDevicekSurfaceFormatKHRList::isSupportedFormat (VkFormat fmt) const
{
    for (u32 i2=0; i2<getCount(); i2++)
    {
        if (fmt == get(i2)->format)
            return true;
    }

    return false;
}


void VPhyDevicekSurfaceFormatKHRList::printInfo() const
{
    gos::logger::log_3 ("supported format:\n");
    gos::logger::inc_indent();
    for (u32 i2=0; i2<getCount(); i2++)
        gos::logger::log_3 ("%s, %s\n", string_VkFormat(get(i2)->format), string_VkColorSpaceKHR (get(i2)->colorSpace));
    gos::logger::dec_indent();
}




/*************************************************************************************+
 * 
 * VPhyDevicekSurfacePresentModesKHRList
 * 
 * 
 ************************************/
void VPhyDevicekSurfacePresentModesKHRList::build (gos::Allocator *allocatorIN, VkPhysicalDevice &phyDevice, const VkSurfaceKHR &vkSurface)
{
    this->free();
    u32 nElem;
    vkGetPhysicalDeviceSurfacePresentModesKHR(phyDevice, vkSurface, &nElem, nullptr);
    if (nElem)
    {
        this->alloc (allocatorIN, nElem);
        vkGetPhysicalDeviceSurfacePresentModesKHR(phyDevice, vkSurface, &nElem, this->_getRawBuffer());
    }            
}

void VPhyDevicekSurfacePresentModesKHRList::printInfo() const
{
    gos::logger::log_3 ("present mode:\n");
    gos::logger::inc_indent();
    for (u32 i2=0; i2<getCount(); i2++)
        gos::logger::log_3 ("%s\n", string_VkPresentModeKHR (*get(i2)));
    gos::logger::dec_indent();
}

bool VPhyDevicekSurfacePresentModesKHRList::exists (VkPresentModeKHR mode) const
{
    for (u32 i2=0; i2<getCount(); i2++)
    {
        if ( *get(i2) == mode)
            return true;
    }
    return false;
}


