#include "gosGPUVukanHelpers.h"
#include "../gosGPU.h"
#include "../gosGPUVtxDecl.h"
#include "../gosGPUUtils.h"
#include "../../gos/gos.h"

using namespace gos;


/*************************************************************************************+
 * 
 * VkPipelineVertexInputStage
 * 
 * 
 ************************************/
bool VkPipelineVertexInputStage::build (const GPU *gpu, const GPUVtxDeclHandle handle)
{
    const u32 numInputBindingDesc = sizeof(vxtBindingDescrList) / sizeof(VkVertexInputBindingDescription);
    const u32 numAttributeDesc = sizeof(vtxAttributeDescrList) / sizeof(VkVertexInputAttributeDescription);

    memset (&vkVertexInputState, 0, sizeof(vkVertexInputState));
    vkVertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    if (handle.isInvalid())
    {
        vkVertexInputState.vertexBindingDescriptionCount = 0;
        vkVertexInputState.pVertexBindingDescriptions = nullptr; // Optional
        vkVertexInputState.vertexAttributeDescriptionCount = 0;
        vkVertexInputState.pVertexAttributeDescriptions = nullptr; // Optional
        return true;
    }

    //recupera la descrizione della vtxDecl
    gpu::VtxDecl vtxDecl;
    if (!gpu->vtxDecl_query (handle, &vtxDecl))
    {
        DBGBREAK;
        return false;
    }

    if (vtxDecl.stream_getNum() > numInputBindingDesc)
        return false;
    if (vtxDecl.attr_getNum() > numAttributeDesc)
        return false;

    u32 totNumAttributeDescr=0;
    for (u32 i=0; i<vtxDecl.stream_getNum(); i++)
    {        
        vxtBindingDescrList[i].binding = i;

        if (eVtxStreamInputRate::perInstance == vtxDecl.stream_getInputRate(i))
            vxtBindingDescrList[i].inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;
        else
            vxtBindingDescrList[i].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        vxtBindingDescrList[i].stride = vtxDecl.stream_getStrideInByte(i);

        u8 firstIndex;
        u8 numDescrInStream;
        vtxDecl.attr_getList (i, &firstIndex, &numDescrInStream);
        for (u8 i2=0; i2<numDescrInStream; i2++)
        {
            vtxAttributeDescrList[totNumAttributeDescr].binding =  vtxDecl.attr_getStreamIndex(firstIndex);
            vtxAttributeDescrList[totNumAttributeDescr].location = vtxDecl.attr_getBindingLocation(firstIndex);
            vtxAttributeDescrList[totNumAttributeDescr].offset = vtxDecl.attr_getOffset(firstIndex);
            vtxAttributeDescrList[totNumAttributeDescr].format = gos::gpu::dataFormat_to_vulkan (vtxDecl.attr_getDataFormat(firstIndex));
            
            totNumAttributeDescr++;
            firstIndex++;
        }
    }

    assert (totNumAttributeDescr == vtxDecl.attr_getNum());

    vkVertexInputState.vertexBindingDescriptionCount = vtxDecl.stream_getNum();
    vkVertexInputState.pVertexBindingDescriptions = vxtBindingDescrList;
    vkVertexInputState.vertexAttributeDescriptionCount = vtxDecl.attr_getNum();
    vkVertexInputState.pVertexAttributeDescriptions = vtxAttributeDescrList;
    return true;
}



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
    gos::logger::log ("available validation layers (%d):\n", getCount());
    gos::logger::incIndent();
    for (u32 i=0; i<getCount(); i++)
        gos::logger::log (" %s", get(i)->layerName);
    gos::logger::log ("\n");
    gos::logger::decIndent();    
}

u32 VkInstanceValidationLayersList::find (const char *layerName) const
{
    for (u32 i=0; i<getCount(); i++)
    {
        if (strcasecmp(layerName, get(i)->layerName) == 0)
            return i;
    }
    return i32MAX;
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
    gos::logger::log ("available extensions (%d):\n", getCount());
    gos::logger::incIndent();
    for (u32 i=0; i<getCount(); i++)
        gos::logger::log (" %s ", get(i)->extensionName);
    gos::logger::log ("\n");
    gos::logger::decIndent();
}            

u32 VkInstanceExtensionList::find (const char *extensionName) const
{
    for (u32 i=0; i<getCount(); i++)
    {
        if (strcasecmp(extensionName, get(i)->extensionName) == 0)
            return i;
    }
    return i32MAX;
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
    gos::logger::log ("supported extensions:\n");
    gos::logger::incIndent();
    for (u32 i2=0; i2<getCount(); i2++)
        gos::logger::log ("%s\n", get(i2)->extensionName);
    gos::logger::decIndent();
}            

u32 VkPhyDeviceExtensionList::find (const char *extensionName) const
{
    for (u32 i=0; i<getCount(); i++)
    {
        if (strcasecmp(extensionName, get(i)->extensionName) == 0)
            return i;
    }
    return i32MAX;
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
    gos::logger::log ("count=%d\n", get(i)->queueCount);
    gos::logger::log ("flags=");
    if (support_VK_QUEUE_GRAPHICS_BIT(i)) gos::logger::log ("VK_QUEUE_GRAPHICS_BIT ");
    if (support_VK_QUEUE_COMPUTE_BIT(i)) gos::logger::log ("VK_QUEUE_COMPUTE_BIT ");
    if (support_VK_QVK_QUEUE_TRANSFER_BIT(i)) gos::logger::log ("VK_QUEUE_TRANSFER_BIT ");
    if (support_VK_QUEUE_SPARSE_BINDING_BIT(i)) gos::logger::log ("VK_QUEUE_SPARSE_BINDING_BIT ");
    if ((get(i)->queueFlags & VK_QUEUE_VIDEO_DECODE_BIT_KHR)) gos::logger::log ("VK_QUEUE_VIDEO_DECODE_BIT_KHR ");
    if ((get(i)->queueFlags & VK_QUEUE_VIDEO_ENCODE_BIT_KHR)) gos::logger::log ("VK_QUEUE_VIDEO_ENCODE_BIT_KHR ");
    if ((get(i)->queueFlags & VK_QUEUE_OPTICAL_FLOW_BIT_NV)) gos::logger::log ("VK_QUEUE_OPTICAL_FLOW_BIT_NV ");
    if ((get(i)->queueFlags & VK_QUEUE_PROTECTED_BIT)) gos::logger::log ("VK_QUEUE_PROTECTED_BIT ");
    if ((get(i)->queueFlags & VK_DEVICE_QUEUE_CREATE_PROTECTED_BIT)) gos::logger::log ("VK_DEVICE_QUEUE_CREATE_PROTECTED_BIT ");
    gos::logger::log ("\n");
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

void VPhyDevicekSurfaceFormatKHRList::printInfo() const
{
    gos::logger::log ("supported format:\n");
    gos::logger::incIndent();
    for (u32 i2=0; i2<getCount(); i2++)
        gos::logger::log ("%s, %s\n", string_VkFormat(get(i2)->format), string_VkColorSpaceKHR (get(i2)->colorSpace));
    gos::logger::decIndent();
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
    gos::logger::log ("present mode:\n");
    gos::logger::incIndent();
    for (u32 i2=0; i2<getCount(); i2++)
        gos::logger::log ("%s\n", string_VkPresentModeKHR (*get(i2)));
    gos::logger::decIndent();
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


