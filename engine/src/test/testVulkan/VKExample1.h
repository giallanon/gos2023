#ifndef _VKExample1_h_
#define _VKExample1_h_
#define _GLFW_X11
#define GLFW_INCLUDE_VULKAN
#define VK_ENABLE_BETA_EXTENSIONS
#include "GLFW/glfw3.h"
#include "gos.h"
#include "gosBufferLinear.h"
#include <vulkan/vk_enum_string_helper.h>

/************************************
 *  VulkanExample
 */
class VulkanExample1
{
public:
    
                VulkanExample1();
    bool        init();
    void        mainLoop();
    void        cleanup();
    

private:
    struct sSwapChainInfo
    {
        static constexpr u8 N_MAX_IMAGES = 16;

                sSwapChainInfo()            { reset(); }
        void    reset()
        {
            imageCount=0; 
            vkSwapChain=VK_NULL_HANDLE; 
            for (u8 i=0;i<N_MAX_IMAGES;i++)
            { 
                vkImage[i]=VK_NULL_HANDLE; 
                vkImageView[i]=VK_NULL_HANDLE;
                frameBuffers[i]=VK_NULL_HANDLE;
            } 
        }
        void    destroy(VkDevice &vkDevice)
        {
            for (u8 i=0;i<imageCount;i++)
            {
                if (VK_NULL_HANDLE != frameBuffers[i])
                    vkDestroyFramebuffer(vkDevice, frameBuffers[i], nullptr);
                if (VK_NULL_HANDLE != vkImageView[i])
                    vkDestroyImageView(vkDevice, vkImageView[i], nullptr);
            }
            if (VK_NULL_HANDLE != vkSwapChain)
                vkDestroySwapchainKHR(vkDevice, vkSwapChain, nullptr);
            reset();
        }
        bool    createFrameBuffer (VkDevice &vkDevice, VkRenderPass &renderPass)
        {
            bool ret = true;
            gos::logger::log ("sSwapChainInfo::createFrameBuffer\n");
            gos::logger::incIndent();

            for (u32 i = 0; i < imageCount; i++) 
            {
                VkFramebufferCreateInfo framebufferInfo{};
                framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                framebufferInfo.renderPass = renderPass;
                framebufferInfo.attachmentCount = 1;
                framebufferInfo.pAttachments = &vkImageView[i];
                framebufferInfo.width = imageExtent.width;
                framebufferInfo.height = imageExtent.height;
                framebufferInfo.layers = 1;

                const VkResult result = vkCreateFramebuffer(vkDevice, &framebufferInfo, nullptr, &frameBuffers[i]);
                if (VK_SUCCESS != result)
                {
                    gos::logger::err ("vkCreateFramebuffer() => %s\n", string_VkResult(result));
                    ret = false;
                    break;
                }
            }

            if (ret)
                gos::logger::log (eTextColor::green, "OK\n");
            gos::logger::decIndent();
            return ret;
        }

        VkSwapchainKHR      vkSwapChain;
        VkFormat            imageFormat;
        VkExtent2D          imageExtent;
        VkColorSpaceKHR     colorSpace;
        u32                 imageCount;
        VkImage             vkImage[N_MAX_IMAGES];
        VkImageView         vkImageView[N_MAX_IMAGES];
        VkFramebuffer       frameBuffers[N_MAX_IMAGES];
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

    struct sVkPipeline
    {
    public:
                    sVkPipeline()                   { reset(); }
        void        reset()                         { layout=VK_NULL_HANDLE; renderPass=VK_NULL_HANDLE; pipe=VK_NULL_HANDLE; }
        void        destroy(VkDevice &vkDevice)
        {
            if (VK_NULL_HANDLE != pipe)
                vkDestroyPipeline (vkDevice, pipe, nullptr);

            if (VK_NULL_HANDLE != layout)
                vkDestroyPipelineLayout (vkDevice, layout, nullptr);

            if (VK_NULL_HANDLE != renderPass)
                vkDestroyRenderPass (vkDevice, renderPass, nullptr);

            reset();
        }

    public:
        VkPipelineLayout    layout;
        VkRenderPass        renderPass;
        VkPipeline          pipe;
    };


    class StringList
    {
    public:
                    StringList ()                                           { cursor=0; count=0; }
                    StringList (gos::Allocator *allocatorIN, u32 size=512)  { setup (allocatorIN, size); }
                    ~StringList ()                                          { unsetup(); }

        void        setup (gos::Allocator *allocatorIN, u32 size)           { buffer.setup(allocatorIN, size); reset(); }
        void        unsetup()                                               { buffer.unsetup(); }

        void        reset()                                                 { cursor=0; count=0; buffer.zero(); }
        void        add (const char *m)
        {
            const u32 n = strlen(m) +1;
            if (n == 1)
                return;
            if (cursor + n >= buffer.getTotalSizeAllocated())
                buffer.growIncremental (512);

            buffer.write (m, cursor, n, false);
            cursor += n;
            count++;
        }

        u32         getNumString() const                                    { return count; }
        void        toStart (u32 *iter) const                               { (*iter) = 0;};
        const char* next(u32 *iter) const 
        {
            u32 offset = ((*iter) & 0x0000FFFF);
            u32 stringNum = (((*iter) & 0xFFFF0000) >> 16);
            if (stringNum >= count)
                return NULL;
            
            const char *ret = reinterpret_cast<const char*>(buffer._getPointer(offset));

            stringNum++;
            const u32 n = strlen(ret);
            offset += (n+1);
            (*iter) = offset | (stringNum<<16);
            return ret;
        }

    private:
        gos::BufferLinear buffer;
        u32             cursor;
        u32             count;
    };

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

        void    build (gos::Allocator *allocator)
        {
            this->free();
            u32 nElem;
            vkEnumerateInstanceLayerProperties(&nElem, nullptr);
            if (nElem)
            {
                this->alloc (allocator, nElem);
                vkEnumerateInstanceLayerProperties(&nElem, this->_getRawBuffer());
            }            
        }
        void    printInfo () const
        {
            gos::logger::log ("available validation layers (%d):\n", getCount());
            gos::logger::incIndent();
            for (u32 i=0; i<getCount(); i++)
                gos::logger::log (" %s", get(i)->layerName);
            gos::logger::log ("\n");
            gos::logger::decIndent();    
        }            
        u32     find (const char *layerName) const
        {
            for (u32 i=0; i<getCount(); i++)
            {
                if (strcasecmp(layerName, get(i)->layerName) == 0)
                    return i;
            }
            return i32MAX;
        }
    };
    
    /*=======================================================
     * crea e mantiene una lista delle "extensionr" diposibili per la vkIntance
     */
    class VkInstanceExtensionList : public VulkanList<VkExtensionProperties>
    {
    public:
                VkInstanceExtensionList()         { }
        virtual ~VkInstanceExtensionList()        { }

        void    build (gos::Allocator *allocator)
        {
            this->free();
            u32 nElem;
            vkEnumerateInstanceExtensionProperties(nullptr, &nElem, nullptr);
            if (nElem)
            {
                this->alloc (allocator, nElem);
                vkEnumerateInstanceExtensionProperties(nullptr, &nElem, this->_getRawBuffer());
            }            
        }
        void    printInfo () const
        {
            gos::logger::log ("available extensions (%d):\n", getCount());
            gos::logger::incIndent();
            for (u32 i=0; i<getCount(); i++)
                gos::logger::log (" %s ", get(i)->extensionName);
            gos::logger::log ("\n");
            gos::logger::decIndent();
        }            
        u32     find (const char *extensionName) const
        {
            for (u32 i=0; i<getCount(); i++)
            {
                if (strcasecmp(extensionName, get(i)->extensionName) == 0)
                    return i;
            }
            return i32MAX;
        }
    };

    /*=======================================================
     * crea e mantiene una lista delle "extensionr" diposibili per il device fisico
     */
    class VkPhyDeviceExtensionList : public VulkanList<VkExtensionProperties>
    {
    public:
                VkPhyDeviceExtensionList()         { }
        virtual ~VkPhyDeviceExtensionList()        { }

        void    build (gos::Allocator *allocator, VkPhysicalDevice &phyDevice)
        {
            this->free();
            u32 nElem;
            vkEnumerateDeviceExtensionProperties(phyDevice, nullptr, &nElem, nullptr);
            if (nElem)
            {
                this->alloc (allocator, nElem);
                vkEnumerateDeviceExtensionProperties(phyDevice, nullptr, &nElem, this->_getRawBuffer());
            }            
        }
        void    printInfo () const
        {
            gos::logger::log ("supported extensions:\n");
            gos::logger::incIndent();
            for (u32 i2=0; i2<getCount(); i2++)
                gos::logger::log ("%s\n", get(i2)->extensionName);
            gos::logger::decIndent();
        }            
        u32     find (const char *extensionName) const
        {
            for (u32 i=0; i<getCount(); i++)
            {
                if (strcasecmp(extensionName, get(i)->extensionName) == 0)
                    return i;
            }
            return i32MAX;
        }
    };


    /*=======================================================
     * crea e mantiene una lista delle "queue" diposibili per il device fisico
     */
    class VkPhyDeviceQueueList : public VulkanList<VkQueueFamilyProperties>
    {
    public:
                VkPhyDeviceQueueList()         { }
        virtual ~VkPhyDeviceQueueList()        { }

        void    build (gos::Allocator *allocator, VkPhysicalDevice &phyDevice)
        {
            this->free();
            u32 nElem;
            vkGetPhysicalDeviceQueueFamilyProperties(phyDevice, &nElem, nullptr);
            if (nElem)
            {
                this->alloc (allocator, nElem);
                vkGetPhysicalDeviceQueueFamilyProperties(phyDevice, &nElem, this->_getRawBuffer());
            }            
        }
        void    printQueueFamilyInfo (u32 i) const
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

        void    build (gos::Allocator *allocator, VkPhysicalDevice &phyDevice, const VkSurfaceKHR &vkSurface)
        {
            this->free();
            u32 nElem;
            vkGetPhysicalDeviceSurfaceFormatsKHR(phyDevice, vkSurface, &nElem, nullptr);
            if (nElem)
            {
                this->alloc (allocator, nElem);
                vkGetPhysicalDeviceSurfaceFormatsKHR(phyDevice, vkSurface, &nElem, this->_getRawBuffer());
            }            
        }
        void    printInfo() const
        {
            gos::logger::log ("supported format:\n");
            gos::logger::incIndent();
            for (u32 i2=0; i2<getCount(); i2++)
                gos::logger::log ("%s, %s\n", string_VkFormat(get(i2)->format), string_VkColorSpaceKHR (get(i2)->colorSpace));
            gos::logger::decIndent();
        }
    };

    /*=======================================================
     * crea e mantiene una lista delle "surface format" diposibili per il device fisico
     */
    class VPhyDevicekSurfacePresentModesKHRList : public VulkanList<VkPresentModeKHR>
    {
    public:
                VPhyDevicekSurfacePresentModesKHRList()         { }
        virtual ~VPhyDevicekSurfacePresentModesKHRList()        { }

        void    build (gos::Allocator *allocator, VkPhysicalDevice &phyDevice, const VkSurfaceKHR &vkSurface)
        {
            this->free();
            u32 nElem;
            vkGetPhysicalDeviceSurfacePresentModesKHR(phyDevice, vkSurface, &nElem, nullptr);
            if (nElem)
            {
                this->alloc (allocator, nElem);
                vkGetPhysicalDeviceSurfacePresentModesKHR(phyDevice, vkSurface, &nElem, this->_getRawBuffer());
            }            
        }
        void    printInfo() const
        {
            gos::logger::log ("present mode:\n");
            gos::logger::incIndent();
            for (u32 i2=0; i2<getCount(); i2++)
                gos::logger::log ("%s\n", string_VkPresentModeKHR (*get(i2)));
            gos::logger::decIndent();
        }
        bool    exists (VkPresentModeKHR mode) const
        {
            for (u32 i2=0; i2<getCount(); i2++)
            {
                if ( *get(i2) == mode)
                    return true;
            }
            return false;
        }
    };    

private:
    static bool                     vulkanCreateInstance (VkInstance *out, const StringList &requiredValidationLayer, const StringList &requiredExtensionList);
    static bool                     vulkanScanAndSelectAPhysicalDevices (VkInstance &vkInstance, const VkSurfaceKHR &vkSurface, const StringList &requiredExtensionList, sPhyDeviceInfo *out);
    static bool                     vulkanCreateDevice (sPhyDeviceInfo &vkPhyDevInfo, const StringList &requiredExtensionList, sVkDevice *out);
    static bool                     vulkanCreateSwapChain (sVkDevice &vulkan, const VkSurfaceKHR &vkSurface, sSwapChainInfo *out);
    static bool                     vulkanCreateShaderFromMemory (const u8 *buffer, u32 bufferSize, VkDevice &dev, VkShaderModule *out);
    static bool                     vulkanCreateShaderFromFile (const u8 *filename, VkDevice &dev, VkShaderModule *out);
    static bool                     vulkanCreatePipeline1 (sVkDevice &vulkan, sVkPipeline *out);
    static bool                     vulkanCreateRenderPass (sVkDevice &vulkan, VkRenderPass *out);
    static bool                     vulkanCreateCommandBuffer (sVkDevice &vulkan, VkCommandBuffer *out);
    static bool                     vulkanCreateSemaphore  (sVkDevice &vulkan, VkSemaphore *out);
    static void                     vulkanDestroySemaphore  (sVkDevice &vulkan, VkSemaphore &in);
    static bool                     vulkanCreateFence  (sVkDevice &vulkan, bool bStartAsSignaled, VkFence *out);
    static void                     vulkanDestroyFence  (sVkDevice &vulkan, VkFence &in);

private:
    void                            vulkanAddDebugCallback();
    bool                            recordCommandBuffer(u32 imageIndex);
    void                            mainLoop_waitEveryFrame();
    void                            mainLoop_multiFrame();

private:
    GLFWwindow                  *window;
    VkInstance                  vkInstance;
    VkSurfaceKHR                vkSurface;
    VkDebugUtilsMessengerEXT    vkDebugMessenger;
    sVkDevice                   vulkan;
    VkSurfaceCapabilitiesKHR    vkSurfCapabilities;
    sVkPipeline                 pipe1;
    VkCommandBuffer             vkCommandBuffer;

};


#endif //_VKExample1_h_