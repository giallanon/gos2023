#include "gosGPUUtils.h"


using namespace gos;

//**********************************************************
VkFormat gpu::dataFormat_to_vulkan (eDataFormat f)
{
    switch (f)
    {
    default:
        DBGBREAK;
        return VK_FORMAT_UNDEFINED;

    case eDataFormat::_1f32: return VK_FORMAT_R32_SFLOAT;
    case eDataFormat::_2f32: return VK_FORMAT_R32G32_SFLOAT;
    case eDataFormat::_3f32: return VK_FORMAT_R32G32B32_SFLOAT;
    case eDataFormat::_4f32: return VK_FORMAT_R32G32B32A32_SFLOAT;

    case eDataFormat::_1i32: return VK_FORMAT_R32_SINT;
    case eDataFormat::_2i32: return VK_FORMAT_R32G32_SINT;
    case eDataFormat::_3i32: return VK_FORMAT_R32G32B32_SINT;
    case eDataFormat::_4i32: return VK_FORMAT_R32G32B32A32_SINT;

    case eDataFormat::_1u32: return VK_FORMAT_R32_UINT;
    case eDataFormat::_2u32: return VK_FORMAT_R32G32_UINT;
    case eDataFormat::_3u32: return VK_FORMAT_R32G32B32_UINT;
    case eDataFormat::_4u32: return VK_FORMAT_R32G32B32A32_UINT;

    case eDataFormat::_4u8:         return VK_FORMAT_R8G8B8A8_UINT;
    case eDataFormat::_4u8_norm:    return VK_FORMAT_R8G8B8A8_UNORM;
    }
}

//**********************************************************
VkPrimitiveTopology gpu::drawPrimitive_to_vulkan (eDrawPrimitive f)
{
    switch (f)
    {
    default:
        DBGBREAK;
        return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        case eDrawPrimitive::pointList: return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
        case eDrawPrimitive::lineList: return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
        case eDrawPrimitive::lineStrip: return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
        case eDrawPrimitive::trisList: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        case eDrawPrimitive::trisStrip: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
        case eDrawPrimitive::trisFan: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
    }
}

