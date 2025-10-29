#ifndef _gosGPUVulkanEnumAndDefine_h_
#define _gosGPUVulkanEnumAndDefine_h_
#include "GLFW/gosGLFWInclude.h"
#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>
#include "../gosGPUEnumAndDefine.h"


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

    enum class eGPUQueueType : u8
    {
        gfx = 0,
        compute = 1,
        transfer = 2,

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

    struct sVkDevice
    {
    public:
        static constexpr u8 NUM_QUEUE       = 3;

    public:
        struct sQueueInfo
        {
        public:
            static constexpr u8 NUM_MAX_THREAD  = 16;

        public:
            void            reset()                                     
            { 
                vkQueueHandle = VK_NULL_HANDLE; 
                isAnAliasFor=eGPUQueueType::unknown; 
                familyIndex=u32MAX; 
                for (u32 i = 0; i < NUM_MAX_THREAD; i++)
                {
                    _poolList[i].threadID = u32MAX;
                    _poolList[i].vkPoolHandle = VK_NULL_HANDLE;
                }
            }

        public:
            struct sPool
            {
                u32     threadID;
                VkCommandPool   vkPoolHandle;
            };

        public:
            VkQueue         vkQueueHandle;
            u32             familyIndex;
            eGPUQueueType   isAnAliasFor;                   // se == unknown, allora e' una Q vera e propria, altrimenti e' un alias per un'altra Q
            sPool           _poolList[NUM_MAX_THREAD];      //un cmdPool per ogni possibile thread
        };

    public:
                            sVkDevice()
                            { 
                                reset(); 
                            }

        void                reset()             
                            {
                                phyDevInfo.reset(); 
                                dev=VK_NULL_HANDLE;
                                swapChainInfo.reset();
                                memory_maxAllocated = memory_curAllocated = 0;
                                for (u8 i=0; i<NUM_QUEUE; i++)
                                    _queueList[i].reset();
                            }

        void                destroy()
                            {
                                if (VK_NULL_HANDLE == dev)
                                    return;

                                for (u8 i=0; i<NUM_QUEUE; i++)
                                {
                                    if (eGPUQueueType::unknown == _queueList[i].isAnAliasFor)
                                    {
                                        for (u32 i2 = 0; i2 < sQueueInfo::NUM_MAX_THREAD; i2++)
                                        {
                                            if (VK_NULL_HANDLE != _queueList[i]._poolList[i2].vkPoolHandle)
                                            {
                                                vkDestroyCommandPool (dev, _queueList[i]._poolList[i2].vkPoolHandle, nullptr);
                                                _queueList[i]._poolList[i2].vkPoolHandle = VK_NULL_HANDLE;
                                                _queueList[i]._poolList[i2].threadID = u32MAX;
                                            }
                                        }
                                    }
                                }

                                swapChainInfo.destroy(dev);

                                vkDestroyDevice(dev, nullptr);
                                reset();
                            }

        VkCommandPool       getOrCreateCommandPool (eGPUQueueType qtype, u32 threadID)
        {
            sQueueInfo *q = getQueueInfo(qtype);
            if (eGPUQueueType::unknown != q->isAnAliasFor)
            {
                q = getQueueInfo(q->isAnAliasFor);
            }
            assert (eGPUQueueType::unknown == q->isAnAliasFor); //mi assicuro che la Q non sia un alias di un'altra q

            for (u32 i = 0; i <sQueueInfo::NUM_MAX_THREAD; i++)
            {
                if (q->_poolList[i].threadID == threadID)
                    return q->_poolList[i].vkPoolHandle;
                
                if (u32MAX == q->_poolList[i].threadID)
                {
                    VkCommandPoolCreateInfo poolInfo{};
                    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
                    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
                    poolInfo.queueFamilyIndex = q->familyIndex;
                
                    const VkResult result = vkCreateCommandPool (this->dev, &poolInfo, nullptr, &q->_poolList[i].vkPoolHandle);
                    if (VK_SUCCESS == result)
                    {
                        q->_poolList[i].threadID = threadID;
                        return q->_poolList[i].vkPoolHandle;
                    }
                }
            }
            DBGBREAK;
            return VK_NULL_HANDLE;
        }

        void                deleteCommandBuffer (eGPUQueueType qtype, VkCommandPool vkPool, VkCommandBuffer &vkHandle)
        {
            sQueueInfo *q = getQueueInfo(qtype);
            if (eGPUQueueType::unknown != q->isAnAliasFor)
            {
                q = getQueueInfo(q->isAnAliasFor);
            }
            assert (eGPUQueueType::unknown == q->isAnAliasFor); //mi assicuro che la Q non sia un alias di un'altra q

            for (u32 i = 0; i < sQueueInfo::NUM_MAX_THREAD; i++)
            {
                if (q->_poolList[i].vkPoolHandle == vkPool)
                {
                    VkCommandBuffer vkCmdBufferList[] = { vkHandle };
                    vkFreeCommandBuffers (this->dev, q->_poolList[i].vkPoolHandle, 1, vkCmdBufferList);

                    const u32 lastIndex = sQueueInfo::NUM_MAX_THREAD - 1;
                    const u32 nToCopy = lastIndex - i;
                    if (nToCopy)
                        memcpy (&q->_poolList[i], &q->_poolList[i + 1], sizeof(sQueueInfo::sPool) * nToCopy);

                    q->_poolList[lastIndex].vkPoolHandle = VK_NULL_HANDLE;
                    q->_poolList[lastIndex].threadID = u32MAX;
                    return;
                }
            }

            DBGBREAK;
        }

        sQueueInfo*         getQueueInfo (eGPUQueueType type)
        {
            switch (type)
            {
            default:
                DBGBREAK;
                return &_queueList[0];
            case eGPUQueueType::gfx:         return &_queueList[0];
            case eGPUQueueType::compute:     return &_queueList[1];
            case eGPUQueueType::transfer:    return &_queueList[2];
            }
        }

        const sQueueInfo*   getQueueInfo (eGPUQueueType type) const
        {
            switch (type)
            {
            default:
                DBGBREAK;
                return &_queueList[0];
            case eGPUQueueType::gfx:         return &_queueList[0];
            case eGPUQueueType::compute:     return &_queueList[1];
            case eGPUQueueType::transfer:    return &_queueList[2];
            }
        }
        
        const sQueueInfo*   getQueueInfoByIndex (u32 i) const
        {
            assert (i < NUM_QUEUE);
            return &_queueList[i];
        }
        
    public:
        sPhyDeviceInfo      phyDevInfo;
        sSwapChainInfo      swapChainInfo;
        VkDevice            dev;
        sQueueInfo          _queueList[NUM_QUEUE];
        u64                 memory_maxAllocated;
        u64                 memory_curAllocated;
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