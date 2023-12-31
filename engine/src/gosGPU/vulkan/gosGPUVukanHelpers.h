#ifndef _gosGPUVukanHelpers_h_
#define _gosGPUVukanHelpers_h_
#include "gosGPUVulkanEnumAndDefine.h"
#include "../gosGPUEnumAndDefine.h"
#include "../../gos/memory/gosMemory.h"

namespace gos
{
    class GPU; //fwd decl
    
    namespace gpu
    {
        class VtxDecl; //fwd decl
    }

    /*************************************************************
     * VulkanList
     * 
     * Generico contenitore di struct
    */
    template<typename T>
    class VulkanList
    {
    public:
                    VulkanList()                                    { allocator=NULL; buffer=NULL; count=0; }
                    ~VulkanList()                                   { free(); }

        void        alloc (gos::Allocator *allocIN, u32 num)        { free(); allocator=allocIN; buffer = GOSALLOCT(T*, allocator, sizeof(T) * num); count=num; }
        void        free()                                          { if (allocator) GOSFREE(allocator, buffer); buffer=NULL; count=0; allocator=NULL; }
        u32         getCount() const                                { return count; }
        const T*    get(u32 i) const                                { if (i>=count) return NULL; return &buffer[i]; }
        const T*    operator()(u32 i) const                         { return get(i); }

        T*          _getRawBuffer()                                 { return buffer; }
    
    private:
        gos::Allocator  *allocator;
        T               *buffer;
        u32             count;
    };

    /*=======================================================
     * crea e mantiene una lista dei "validation layer" diposibili per la vkIntance
     */
    class VkInstanceValidationLayersList : public VulkanList<VkLayerProperties>
    {
    public:
                VkInstanceValidationLayersList()         { }
        virtual ~VkInstanceValidationLayersList()        { }

        void    build (gos::Allocator *allocatorIN);
        void    printInfo () const;
        u32     find (const char *layerName) const;
    };
    
    /*=======================================================
     * crea e mantiene una lista delle "extensionr" diposibili per la vkIntance
     */
    class VkInstanceExtensionList : public VulkanList<VkExtensionProperties>
    {
    public:
                VkInstanceExtensionList()         { }
        virtual ~VkInstanceExtensionList()        { }

        void    build (gos::Allocator *allocatorIN);
        void    printInfo () const;
        u32     find (const char *extensionName) const;
    };

    /*=======================================================
     * crea e mantiene una lista delle "extensionr" diposibili per il device fisico
     */
    class VkPhyDeviceExtensionList : public VulkanList<VkExtensionProperties>
    {
    public:
                VkPhyDeviceExtensionList()         { }
        virtual ~VkPhyDeviceExtensionList()        { }

        void    build (gos::Allocator *allocatorIN, VkPhysicalDevice &phyDevice);
        void    printInfo () const;
        u32     find (const char *extensionName) const;
    };


    /*=======================================================
     * crea e mantiene una lista delle "queue" diposibili per il device fisico
     */
    class VkPhyDeviceQueueList : public VulkanList<VkQueueFamilyProperties>
    {
    public:
                VkPhyDeviceQueueList()         { }
        virtual ~VkPhyDeviceQueueList()        { }

        void    build (gos::Allocator *allocatorIN, VkPhysicalDevice &phyDevice);
        void    printQueueFamilyInfo (u32 i) const;

        bool    support_VK_QUEUE_GRAPHICS_BIT(u32 i) const              { return (get(i)->queueFlags & VK_QUEUE_GRAPHICS_BIT); }
        bool    support_VK_QUEUE_COMPUTE_BIT(u32 i) const               { return (get(i)->queueFlags & VK_QUEUE_COMPUTE_BIT); }
        bool    support_VK_QVK_QUEUE_TRANSFER_BIT(u32 i) const          { return (get(i)->queueFlags & VK_QUEUE_TRANSFER_BIT); }
        bool    support_VK_QUEUE_SPARSE_BINDING_BIT(u32 i) const        { return (get(i)->queueFlags & VK_QUEUE_SPARSE_BINDING_BIT); }
    };


    /*=======================================================
     * crea e mantiene una lista delle "surface format" diposibili per il device fisico
     */
    class VPhyDevicekSurfaceFormatKHRList : public VulkanList<VkSurfaceFormatKHR>
    {
    public:
                VPhyDevicekSurfaceFormatKHRList()         { }
        virtual ~VPhyDevicekSurfaceFormatKHRList()        { }

        void    build (gos::Allocator *allocatorIN, VkPhysicalDevice &phyDevice, const VkSurfaceKHR &vkSurface);
        void    printInfo() const;
    };

    /*=======1================================================
     * crea e mantiene una lista delle "surface format" diposibili per il device fisico
     */
    class VPhyDevicekSurfacePresentModesKHRList : public VulkanList<VkPresentModeKHR>
    {
    public:
                VPhyDevicekSurfacePresentModesKHRList()         { }
        virtual ~VPhyDevicekSurfacePresentModesKHRList()        { }

        void    build (gos::Allocator *allocatorIN, VkPhysicalDevice &phyDevice, const VkSurfaceKHR &vkSurface);
        void    printInfo() const;
        bool    exists (VkPresentModeKHR mode) const;
    };    

    /*=======================================================
     * In base alla [vtxDecl], crea la struct necessaria a Vulkan durante
     * la creazione di una pipeline
     */
    class VkPipelineVertexInputStage
    {
    public:
                VkPipelineVertexInputStage()                    { }
        bool    build (const GPU *gpu, const GPUVtxDeclHandle handle);

    public:
        VkPipelineVertexInputStateCreateInfo vkVertexInputState;

    private:
        VkVertexInputBindingDescription vxtBindingDescrList[8];
        VkVertexInputAttributeDescription vtxAttributeDescrList[64];
    };
} //namespace gos




#endif //_gosGPUVukanHelpers_h_