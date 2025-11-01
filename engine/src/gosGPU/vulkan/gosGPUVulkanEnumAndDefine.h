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
    public:
                sSwapChainInfo()            { reset(); }
        
        void    reset()
                {
                    imageCount=0; 
                    vkSwapChain=VK_NULL_HANDLE; 
                    for (u8 i=0;i<SWAPCHAIN_NUM_MAX_IMAGES;i++)
                    { 
                        vkImageList[i]=VK_NULL_HANDLE; 
                        vkImageListView[i]=VK_NULL_HANDLE;
                    } 
                }

        u32                 getWidth() const                        { return imageExtent.width; }
        u32                 getHeight() const                       { return imageExtent.height; }
        f32                 calcAspectRatio() const                 { return (f32)getWidth() / (f32)getHeight(); }
        VkFormat            getImageFormat() const                  { return imageFormat; }
        u8                  getImageCount() const                   { return static_cast<u8>(imageCount); }
        VkExtent2D          getImageExten2D() const                 { return imageExtent; }

    public:
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