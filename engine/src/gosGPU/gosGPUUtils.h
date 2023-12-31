#ifndef _gosGPUUtils_h_
#define _gosGPUUtils_h_
#include "gosGPUEnumAndDefine.h"

namespace gos
{
    namespace gpu
    {
        VkFormat             dataFormat_to_vulkan (eDataFormat f);
        VkPrimitiveTopology  drawPrimitive_to_vulkan (eDrawPrimitive f);

    } //namespace gos
} //namespace gos

#endif //_gosGPUUtils_h_