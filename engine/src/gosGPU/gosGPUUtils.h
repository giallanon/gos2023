#ifndef _gosGPUUtils_h_
#define _gosGPUUtils_h_
#include "gosGPUEnumAndDefine.h"

namespace gos
{
    namespace gpu
    {
        const char*     enumToString (eVIBufferMode s);

        VkFormat                toVulkan (eDataFormat f);
        VkPrimitiveTopology     toVulkan (eDrawPrimitive f);
        VkCompareOp             toVulkan (eZFunc f);
        VkCompareOp             toVulkan (eStencilFunc f);
        VkStencilOp             toVulkan (eStencilOp f);

    } //namespace gos
} //namespace gos

#endif //_gosGPUUtils_h_