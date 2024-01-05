#include "gosGPUUtils.h"


using namespace gos;



//*************************************************************************
const char* gpu::enumToString (eVIBufferMode s)
{
    switch (s)
    {
    default:                        return "??INVALID-VALUE??";
    case eVIBufferMode::onGPU:      return "onGPU";
    case eVIBufferMode::mappale:    return "mappale";
    case eVIBufferMode::unknown:    return "unknown";
    }
}


//**********************************************************
VkFormat gpu::toVulkan (eDataFormat f)
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
VkPrimitiveTopology gpu::toVulkan (eDrawPrimitive f)
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

//**********************************************************
VkCompareOp gpu::toVulkan (eZFunc f)
{
    switch (f)
    {
    default:
        DBGBREAK;
        return VK_COMPARE_OP_LESS;

    case eZFunc::NEVER:         return VK_COMPARE_OP_NEVER;
    case eZFunc::LESS:          return VK_COMPARE_OP_LESS;
    case eZFunc::EQUAL:         return VK_COMPARE_OP_EQUAL;
    case eZFunc::LESS_EQUAL:    return VK_COMPARE_OP_LESS_OR_EQUAL;
    case eZFunc::GREATER:       return VK_COMPARE_OP_GREATER;
    case eZFunc::NOT_EQUAL:     return VK_COMPARE_OP_NOT_EQUAL;
    case eZFunc::GREATER_EQUAL: return VK_COMPARE_OP_GREATER_OR_EQUAL;
    case eZFunc::ALWAYS:        return VK_COMPARE_OP_ALWAYS;
    }
}

//**********************************************************
VkCompareOp gpu::toVulkan (eStencilFunc f)
{
    switch (f)
    {
    default:
        DBGBREAK;
        return VK_COMPARE_OP_LESS;

    case eStencilFunc::NEVER:         return VK_COMPARE_OP_NEVER;
    case eStencilFunc::LESS:          return VK_COMPARE_OP_LESS;
    case eStencilFunc::EQUAL:         return VK_COMPARE_OP_EQUAL;
    case eStencilFunc::LESS_EQUAL:    return VK_COMPARE_OP_LESS_OR_EQUAL;
    case eStencilFunc::GREATER:       return VK_COMPARE_OP_GREATER;
    case eStencilFunc::NOT_EQUAL:     return VK_COMPARE_OP_NOT_EQUAL;
    case eStencilFunc::GREATER_EQUAL: return VK_COMPARE_OP_GREATER_OR_EQUAL;
    case eStencilFunc::ALWAYS:        return VK_COMPARE_OP_ALWAYS;
    }
}

//**********************************************************
VkStencilOp gpu::toVulkan (eStencilOp f)
{
    switch (f)
    {
    default:
        DBGBREAK;
        return VK_STENCIL_OP_KEEP;

    case eStencilOp::KEEP:              return VK_STENCIL_OP_KEEP;
    case eStencilOp::ZERO:              return VK_STENCIL_OP_ZERO;
    case eStencilOp::REPLACE:           return VK_STENCIL_OP_REPLACE;
    case eStencilOp::INCR_AND_CLAMP:    return VK_STENCIL_OP_INCREMENT_AND_CLAMP;
    case eStencilOp::DECR_AND_CLAMP:    return VK_STENCIL_OP_DECREMENT_AND_CLAMP;
    case eStencilOp::INVERT:            return VK_STENCIL_OP_INVERT;
    case eStencilOp::INCR_AND_WRAP:     return VK_STENCIL_OP_INCREMENT_AND_WRAP;
    case eStencilOp::DECR_AND_WRAP:     return VK_STENCIL_OP_DECREMENT_AND_WRAP;
    }
}