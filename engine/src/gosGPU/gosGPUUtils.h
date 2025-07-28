#ifndef _gosGPUUtils_h_
#define _gosGPUUtils_h_
#include "gosGPUEnumAndDefine.h"
#include "vulkan/gosGPUVulkanEnumAndDefine.h"
#include "../gosImage/gosImageEnumAndDefine.h"

namespace gos
{
    namespace gpu
    {
        const char*     enumToString (eVIBufferMode s);
        const char*     enumToString (eGPUQueueType s);

        VkFormat                toVulkan (eDataFormat f);
        VkPrimitiveTopology     toVulkan (eDrawPrimitive f);
        VkCompareOp             toVulkan (eZFunc f);
        VkCompareOp             toVulkan (eStencilFunc f);
        VkStencilOp             toVulkan (eStencilOp f);
        VkFilter                toVulkan (eSamplerFilter s);
        VkFormat                toVulkan (eImageFormat fmt);
        eImageFormat            fromVulkan (VkFormat fmt);
        VkImageLayout           toVulkan (eImageLayout s);
        VkImageLayout           toVulkan (eDepthStencilLayout s, eImageFormat fmt);
        VkAttachmentLoadOp      toVulkan (eAttachmentLoadOp s);
        VkAttachmentStoreOp     toVulkan (eAttachmentStoreOp s);

    } //namespace gos
} //namespace gos

#endif //_gosGPUUtils_h_