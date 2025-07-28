#include "gosGPUUtils.h"
#include "../gosImage/gosImage.h"

using namespace gos;



//*************************************************************************
const char* gpu::enumToString (eVIBufferMode s)
{
    switch (s)
    {
    default:                                        return "??INVALID-VALUE??";
    case eVIBufferMode::onGPU:                      return "onGPU";
    case eVIBufferMode::shared_cpuW_autoSync:       return "shared_cpuW_autoSync";
    case eVIBufferMode::shared_cpuW_manualSync:     return "shared_cpuW_manualSync";
    case eVIBufferMode::unknown:                    return "unknown";
    }
}

//*************************************************************************
const char* gpu::enumToString (eGPUQueueType s)
{
    switch (s)
    {
    default:                        return "??INVALID-VALUE??";
    case eGPUQueueType::gfx:        return "gfx";
    case eGPUQueueType::compute:    return "compute";
    case eGPUQueueType::transfer:   return "transfer";
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

//**********************************************************
VkImageLayout gpu::toVulkan (eDepthStencilLayout s, eImageFormat fmt)
{
    switch (s)
    {
    default:
        DBGBREAK;
        return VK_IMAGE_LAYOUT_UNDEFINED;

	case eDepthStencilLayout::undefined:                            return VK_IMAGE_LAYOUT_UNDEFINED;
    case eDepthStencilLayout::depth_attachment_optimal:
        if (image::isFormatWithStencil(fmt))
            return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        return VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;

    case eDepthStencilLayout::depth_shader_readonly:
        if (image::isFormatWithStencil(fmt))
            return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        return VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL;
 
    }
};

//**********************************************************
VkFormat gpu::toVulkan (eImageFormat fmt)
{
    switch (fmt)
    {
    default:
        DBGBREAK;
        return VK_FORMAT_UNDEFINED;

    case eImageFormat::U8_RGBA_sRGB: return VK_FORMAT_R8G8B8A8_SRGB;
    case eImageFormat::U8_RGBA: return VK_FORMAT_R8G8B8A8_UNORM;
    case eImageFormat::U8_RGB: return VK_FORMAT_R8G8B8_UNORM;
    case eImageFormat::U8_R: return VK_FORMAT_R8_UNORM;

    case eImageFormat::U16_RGBA: return VK_FORMAT_R16G16B16A16_UNORM;
    case eImageFormat::U16_RGB: return VK_FORMAT_R16G16B16_UNORM;
    case eImageFormat::U16_R: return VK_FORMAT_R16_UNORM;

    case eImageFormat::U32_RGBA: return VK_FORMAT_R32G32B32A32_UINT;
    case eImageFormat::U32_RGB: return VK_FORMAT_R32G32B32_UINT;
    case eImageFormat::U32_R: return VK_FORMAT_R32_UINT;

    case eImageFormat::F32_RGBA: return VK_FORMAT_R32G32B32A32_SFLOAT;
    case eImageFormat::F32_RGB: return VK_FORMAT_R32G32B32_SFLOAT;
    case eImageFormat::F32_R: return VK_FORMAT_R32_SFLOAT;

    case eImageFormat::U8_BGRA_sRGB:       return VK_FORMAT_B8G8R8A8_SRGB;

    //depth buffer format
    case eImageFormat::DEPTH_F32:               return VK_FORMAT_D32_SFLOAT;
    case eImageFormat::DEPTH_U16:               return VK_FORMAT_D16_UNORM;
    case eImageFormat::DEPTH_F32_STENCIL_U8:    return VK_FORMAT_D32_SFLOAT_S8_UINT;
    case eImageFormat::DEPTH_U16_STENCIL_U8:    return VK_FORMAT_D16_UNORM_S8_UINT;
    case eImageFormat::DEPTH_U24_STENCIL_U8:    return VK_FORMAT_D24_UNORM_S8_UINT;

    }
}

//**********************************************************
eImageFormat gpu::fromVulkan (VkFormat fmt)
{
    switch (fmt)
    {
    default:
        DBGBREAK;
        return eImageFormat::U8_RGBA;

    case VK_FORMAT_R8G8B8A8_SRGB:       return eImageFormat::U8_RGBA_sRGB;
    case VK_FORMAT_R8G8B8A8_UNORM:      return eImageFormat::U8_RGBA;
    case VK_FORMAT_R8G8B8_UNORM:        return eImageFormat::U8_RGB;
    case VK_FORMAT_R8_UNORM:            return eImageFormat::U8_R;

    case VK_FORMAT_R16G16B16A16_UNORM:  return eImageFormat::U16_RGBA;
    case VK_FORMAT_R16G16B16_UNORM:     return eImageFormat::U16_RGB;
    case VK_FORMAT_R16_UNORM:           return eImageFormat::U16_R;

    case VK_FORMAT_R32G32B32A32_UINT:   return eImageFormat::U32_RGBA;
    case VK_FORMAT_R32G32B32_UINT:      return eImageFormat::U32_RGB;
    case VK_FORMAT_R32_UINT:            return eImageFormat::U32_R;

    case VK_FORMAT_R32G32B32A32_SFLOAT: return eImageFormat::F32_RGBA;
    case VK_FORMAT_R32G32B32_SFLOAT:    return eImageFormat::F32_RGB;
    case VK_FORMAT_R32_SFLOAT:          return eImageFormat::F32_R;

    case VK_FORMAT_B8G8R8A8_SRGB:       return eImageFormat::U8_BGRA_sRGB;

    //depth buffer format
    case VK_FORMAT_D32_SFLOAT:          return eImageFormat::DEPTH_F32;
    case VK_FORMAT_D16_UNORM:           return eImageFormat::DEPTH_U16;
    case VK_FORMAT_D32_SFLOAT_S8_UINT:  return eImageFormat::DEPTH_F32_STENCIL_U8;
    case VK_FORMAT_D16_UNORM_S8_UINT:   return eImageFormat::DEPTH_U16_STENCIL_U8;
    case VK_FORMAT_D24_UNORM_S8_UINT:   return eImageFormat::DEPTH_U24_STENCIL_U8;

    }
}

//**********************************************************
VkFilter gpu::toVulkan (eSamplerFilter s)
{
    switch (s)
    {
    default:
        DBGBREAK;
        return VK_FILTER_NEAREST;

    case eSamplerFilter::point: return VK_FILTER_NEAREST;
    case eSamplerFilter::linear: return VK_FILTER_LINEAR;
    }

}

//**********************************************************
VkImageLayout gpu::toVulkan (eImageLayout s)
{
    switch (s)
    {
    default:
        DBGBREAK;
        return VK_IMAGE_LAYOUT_UNDEFINED;

    case eImageLayout::undefined:                   return VK_IMAGE_LAYOUT_UNDEFINED;
    case eImageLayout::general:                     return VK_IMAGE_LAYOUT_GENERAL;
    case eImageLayout::color_attachment_optimal:    return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    case eImageLayout::shader_readonly:             return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    case eImageLayout::transfer_src:                return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    case eImageLayout::transfer_dst:                return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    case eImageLayout::presentation:                return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    }
}


//**********************************************************
VkAttachmentLoadOp gpu::toVulkan (eAttachmentLoadOp s)
{
    switch (s)
    {
    default:
        DBGBREAK;
        return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    case eAttachmentLoadOp::load:           return VK_ATTACHMENT_LOAD_OP_LOAD;
    case eAttachmentLoadOp::clear:          return VK_ATTACHMENT_LOAD_OP_CLEAR;
    case eAttachmentLoadOp::dont_care:      return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    };
}

//**********************************************************
VkAttachmentStoreOp gpu::toVulkan (eAttachmentStoreOp s)
{
    switch (s)
    {
    default:
        DBGBREAK;
        return VK_ATTACHMENT_STORE_OP_DONT_CARE;
    case eAttachmentStoreOp::store:             return VK_ATTACHMENT_STORE_OP_STORE;
    case eAttachmentStoreOp::dont_care:         return VK_ATTACHMENT_STORE_OP_DONT_CARE;
    case eAttachmentStoreOp::none:              return VK_ATTACHMENT_STORE_OP_NONE;
    };
}
