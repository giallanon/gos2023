#ifndef _gosGPUVulkanEnumAndDefine_h_
#define _gosGPUVulkanEnumAndDefine_h_
#include "GLFW/gosGLFWInclude.h"
#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>
#include "../../gos/gos.h"
#include "../../gos/string/gosStringList.h"

static constexpr u8 SWAPCHAIN_NUM_MAX_IMAGES = 8;

namespace gos
{
    enum class eVulkanVersion : u8
    {
        v1_0 = 10,
        v1_1 = 11,
        v1_2 = 12,
        v1_3 = 13,
    };

    enum class eGPUQueueFamily : u8
    {
        gfx = 0,
        compute = 1,
        transfer = 2,
        video_encode = 3,
        video_decode = 4,
        
        _NUM = 5,       //tenere aggiornato questo

        unknown = 0xff
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
        VkPhysicalDeviceProperties deviceProperties;
    };

    /*************************************
    * @brief    VulkanQFamily
    * 
    */
    class VulkanQFamily
    {
    public:
                            VulkanQFamily();
                            ~VulkanQFamily()                                                  { unsetup(); }

        void                setup (VkDevice vkDev, eGPUQueueFamily familyType, u32 familyIndex);
        void                unsetup();

        void                waitIdle ();
        VkResult            submit(u32 submitCount, const VkSubmitInfo *submitInfo, VkFence fence);

        VkCommandPool       getOrCreateCommandPool (u32 threadID);
        void                deleteCommandBuffer (VkCommandPool vkPool, VkCommandBuffer vkHandle);

        u8                  getFamilyIndex() const                                              { return familyIndex; }
        eGPUQueueFamily     getNativeFamilyType() const                                         { return familyType; }

    private:
        static constexpr u8 NUM_MAX_THREAD  = 16;

    private:
        struct sPool
        {
            u32             threadID;
            VkCommandPool   vkPoolHandle;
        };

    private:
        VkQueue             vkQueueHandle;
        VkDevice            vkDev;
        sPool               poolList[NUM_MAX_THREAD];      //un cmdPool per ogni possibile thread
        u8                  familyIndex;
        eGPUQueueFamily     familyType;
    };


    /*************************************
    * @brief    VulkanDevice
    * 
    */
    class VulkanDevice
    {

    public:
                                VulkanDevice()                                          { priv_reset(); }
                                ~VulkanDevice()                                         { unsetup(); }

        bool                    setup (const sPhyDeviceInfo &phyInfo, const gos::StringList &requiredExtensionList, eVulkanVersion vulkanVersion);
        void                    unsetup();

        void                    queue_waitIdle (eGPUQueueFamily whichOne)                                                                       { getQFamily(whichOne)->waitIdle(); }
        VkResult                queue_submit (eGPUQueueFamily whichOne, u32 submitCount, const VkSubmitInfo *submitInfo, VkFence fence)         { return getQFamily(whichOne)->submit(submitCount, submitInfo, fence); }

        bool                    getMemoryType (uint32_t typeBits, VkMemoryPropertyFlags properties, u32 *out_index);
        bool                    allocMemory (const VkMemoryAllocateInfo *pAllocateInfo, const VkAllocationCallbacks *pAllocator, VkDeviceMemory *pMemory);
        void                    freeMemory (VkDeviceMemory memory, const VkAllocationCallbacks *pAllocator, u64 memSize);

        VulkanQFamily*          getQFamily (eGPUQueueFamily type)                       { return &qfamilyList[priv_from_family_to_index(type)]; }
        const VulkanQFamily*    getQFamily (eGPUQueueFamily type) const                 { return &qfamilyList[priv_from_family_to_index(type)]; }
        
    public:
        sPhyDeviceInfo      phyDevInfo;
        sSwapChainInfo      swapChainInfo;
        VkDevice            vkDev;
        u64                 memory_maxAllocated;
        u64                 memory_curAllocated;

    private:
        static constexpr u8 NUM_MAX_QFAMILY = static_cast<u8>(eGPUQueueFamily::_NUM);

    private:
        void                priv_reset();
        u8                  priv_from_family_to_index (eGPUQueueFamily type) const          { return map_qfamily_to_q[static_cast<u8>(type)]; }
        void                priv_addNativeQFamily (eGPUQueueFamily familyType, u32 familyIndex);

    private:
        VulkanQFamily       qfamilyList[NUM_MAX_QFAMILY];
        u8                  map_qfamily_to_q[NUM_MAX_QFAMILY];
        u8                  numQFamily;

    };


	namespace gpu
	{
		struct sVtxDescriptor
		{
			u8              streamIndex;
			u8              bindingLocation;
			eDataFormat     format;
			u8              offset;
		};

		struct sMappedBuffer
		{
            void            *host_pt;
			VkDeviceMemory  _vkMemHandle;
            u64             size;
            u32             offset;
		};

		struct sMappedImage
		{
            void            *host_image_pt; //questo punta direttamente al buffer con l'immagine
			VkDeviceMemory  _vkMemHandle;
            u64             size;
            u32             offset;
            u32             row_stride;
		};        
	
        struct SwapchainImg
        {
            VkImage 	image;
            VkImageView	imageView;
            u32     	imageIndex;
        };     
    } //namespace gpu



}  //namespace gos

#endif //_gosGPUVulkanEnumAndDefine_h_