#include "gosUtils.h"
#include "gosString.h"
#include "string/gosCompileTimeHashedString.h"

using namespace gos;

#define ENUM_TO_STRING_CASE(enumClass,enumValue) case enumClass::enumValue: return #enumValue

//**************************************
bool utils::stringIsTrueOrFalse (const char *val, bool *out)
{
    if (NULL == val)
        return false;
    if (0x00 == val[0])
        return false;

    if (strcasecmp(val, "1") == 0)      { *out=true; return true; }
    if (strcasecmp(val, "0") == 0)      { *out=false; return true; }

    if (strcasecmp(val, "y") == 0)      { *out=true; return true; }
    if (strcasecmp(val, "n") == 0)      { *out=false; return true; }

    if (strcasecmp(val, "yes") == 0)    { *out=true; return true; }
    if (strcasecmp(val, "no") == 0)     { *out=false; return true; }

    return false;
}


//***********************************************
const char*	utils::enumToString (const eImageFormat fmt)
{
#define HELPER(s)	case eImageFormat::s: return #s;

    switch (fmt)
    {
    default:
        DBGBREAK;
        return "??INVALID-VALUE??";

		HELPER(U8_RGBA_sRGB)
		HELPER(U8_RGBA)
		HELPER(U8_RGB)
		HELPER(U8_R)

		HELPER(U16_RGBA)
		HELPER(U16_RGB)
		HELPER(U16_R)

		HELPER(U32_RGBA)
		HELPER(U32_RGB)
		HELPER(U32_R)

		HELPER(F32_RGBA)
		HELPER(F32_RGB)
		HELPER(F32_R)

		HELPER(U8_BGRA_sRGB)

		//depth buffer format
		HELPER(DEPTH_F32)
		HELPER(DEPTH_U16)
		HELPER(DEPTH_F32_STENCIL_U8)
		HELPER(DEPTH_U16_STENCIL_U8)
		HELPER(DEPTH_U24_STENCIL_U8)
        HELPER(_DEPTH_BEST)
    }

#undef HELPER
}

//********************************************************** 
bool utils::stringToEnum (const char *str, eImageFormat *out)
{
	assert (NULL != out);
	if (NULL == str)
		return false;
        
	const u32 n = gos::string::utf8::lengthInByte(str);
	if (n < 4)
		return false;

#define HELPER(fmt)			if (0 == strcasecmp(str, #fmt)) { *out=eImageFormat::fmt; return true; }

	if (0 == strncasecmp(str, "sameAsSwapchain", 15))
	{
		*out = eImageFormat::_SAME_AS_CURRENT_SWAPCHAIN;
		return true;
	}


	if (0 == strncasecmp(str, "U8_", 3))
	{
		HELPER(U8_RGBA_sRGB)
		HELPER(U8_RGBA)
		HELPER(U8_RGB)
		HELPER(U8_R)
		HELPER(U8_BGRA_sRGB)
	}
	else if (0 == strncasecmp(str, "U16_", 4))
	{
		HELPER(U16_RGBA)
		HELPER(U16_RGB)
		HELPER(U16_R)
	}
	else if (0 == strncasecmp(str, "U32_", 4))
	{
		HELPER(U32_RGBA)
		HELPER(U32_RGB)
		HELPER(U32_R)
	}
	else if (0 == strncasecmp(str, "F32_", 4))
	{
		HELPER(F32_RGBA)
		HELPER(F32_RGB)
		HELPER(F32_R)
	}
	else if (0 == strncasecmp(str, "DDS_", 3))
	{
		HELPER(DDS_BC3)
		HELPER(DDS_BC4)
		HELPER(DDS_BC5)
	}
	else
	{
    	HELPER(DEPTH_F32)
		HELPER(DEPTH_U16)
    	HELPER(DEPTH_F32_STENCIL_U8)
    	HELPER(DEPTH_U16_STENCIL_U8)
    	HELPER(DEPTH_U24_STENCIL_U8)
        HELPER(_DEPTH_BEST)
	}
	
#undef HELPER	
	return false;
}

//***********************************************
const char*	utils::enumToString (const eImageLayout e)
{
#define HELPER(s)	case eImageLayout::s: return #s;

    switch (e)
    {
    default:
        DBGBREAK;
        return "??INVALID-VALUE??";

    HELPER(undefined)
    HELPER(general)
    HELPER(color_attachment_optimal)
    HELPER(shader_readonly)
    HELPER(transfer_src)
    HELPER(transfer_dst)
    HELPER(presentation)
    HELPER(depth_attachment_optimal)
    HELPER(depth_shader_readonly)    
    }

#undef HELPER
}

//********************************************************** 
bool utils::stringToEnum (const char *str, eImageLayout *out)
{
	assert (NULL != out);
	if (NULL == str)
		return false;
    if (0 == str[0])
        return false;
	
#define HELPER(fmt)			if (0 == strcasecmp(str, #fmt)) { *out=eImageLayout::fmt; return true; }

    HELPER(undefined)
    HELPER(general)
    HELPER(color_attachment_optimal)
    HELPER(shader_readonly)
    HELPER(transfer_src)
    HELPER(transfer_dst)
    HELPER(presentation)
    HELPER(depth_attachment_optimal)
    HELPER(depth_shader_readonly)    
	
#undef HELPER	
	return false;
}

//***********************************************
const char*	utils::enumToString (const eAttachmentLoadOp e)
{
#define HELPER(s)	case eAttachmentLoadOp::s: return #s;

    switch (e)
    {
    default:
        DBGBREAK;
        return "??INVALID-VALUE??";

    HELPER(load)
    HELPER(clear)
    HELPER(dont_care)
    }

#undef HELPER
}

//********************************************************** 
bool utils::stringToEnum (const char *str, eAttachmentLoadOp *out)
{
	assert (NULL != out);
	if (NULL == str)
		return false;
    if (0 == str[0])
        return false;
	
#define HELPER(fmt)			if (0 == strcasecmp(str, #fmt)) { *out=eAttachmentLoadOp::fmt; return true; }

    HELPER(load)
    HELPER(clear)
    HELPER(dont_care)
	
#undef HELPER	
	return false;
}

//***********************************************
const char*	utils::enumToString (const eAttachmentStoreOp e)
{
#define HELPER(s)	case eAttachmentStoreOp::s: return #s;

    switch (e)
    {
    default:
        DBGBREAK;
        return "??INVALID-VALUE??";

    HELPER(store)
    HELPER(dont_care)
    HELPER(none)
    }

#undef HELPER
}

//********************************************************** 
bool utils::stringToEnum (const char *str, eAttachmentStoreOp *out)
{
	assert (NULL != out);
	if (NULL == str)
		return false;
    if (0 == str[0])
        return false;
	
#define HELPER(fmt)			if (0 == strcasecmp(str, #fmt)) { *out=eAttachmentStoreOp::fmt; return true; }

    HELPER(store)
    HELPER(dont_care)
    HELPER(none)
	
#undef HELPER	
	return false;
}

//***********************************************
const char*	utils::enumToString (const eZFunc e)
{
#define HELPER(s)	case eZFunc::s: return #s;

    switch (e)
    {
    default:
        DBGBREAK;
        return "??INVALID-VALUE??";

    HELPER(NEVER)
    HELPER(LESS)
    HELPER(EQUAL)
	HELPER(LESS_EQUAL)
	HELPER(GREATER)
	HELPER(NOT_EQUAL)
	HELPER(GREATER_EQUAL)
	HELPER(ALWAYS)
    }

#undef HELPER
}

//********************************************************** 
bool utils::stringToEnum (const char *str, eZFunc *out)
{
	assert (NULL != out);
	if (NULL == str)
		return false;
    if (0 == str[0])
        return false;
	
#define HELPER(fmt)			if (0 == strcasecmp(str, #fmt)) { *out=eZFunc::fmt; return true; }

    HELPER(NEVER)
	HELPER(LESS)
	HELPER(EQUAL)
	HELPER(LESS_EQUAL)
	HELPER(GREATER)
	HELPER(NOT_EQUAL)
	HELPER(GREATER_EQUAL)
	HELPER(ALWAYS)
#undef HELPER	
	return false;
}

//***********************************************
const char*	utils::enumToString (const eStencilOp e)
{
#define HELPER(s)	case eStencilOp::s: return #s;

    switch (e)
    {
    default:
        DBGBREAK;
        return "??INVALID-VALUE??";

    HELPER(KEEP)
    HELPER(ZERO)
    HELPER(REPLACE)
	HELPER(INCR_AND_CLAMP)
	HELPER(DECR_AND_CLAMP)
	HELPER(INVERT)
	HELPER(INCR_AND_WRAP)
	HELPER(DECR_AND_WRAP)
    }

#undef HELPER
}

//********************************************************** 
bool utils::stringToEnum (const char *str, eStencilOp *out)
{
	assert (NULL != out);
	if (NULL == str)
		return false;
    if (0 == str[0])
        return false;
	
#define HELPER(fmt)			if (0 == strcasecmp(str, #fmt)) { *out=eStencilOp::fmt; return true; }

    HELPER(KEEP)
	HELPER(ZERO)
	HELPER(REPLACE)
	HELPER(INCR_AND_CLAMP)
	HELPER(DECR_AND_CLAMP)
	HELPER(INVERT)
	HELPER(INCR_AND_WRAP)
	HELPER(DECR_AND_WRAP)
#undef HELPER	
	return false;
}

//***********************************************
const char*	utils::enumToString (const eStencilFunc e)
{
#define HELPER(s)	case eStencilFunc::s: return #s;

    switch (e)
    {
    default:
        DBGBREAK;
        return "??INVALID-VALUE??";

    HELPER(NEVER)
	HELPER(LESS)
	HELPER(EQUAL)
	HELPER(LESS_EQUAL)
	HELPER(GREATER)
	HELPER(NOT_EQUAL)
	HELPER(GREATER_EQUAL)
	HELPER(ALWAYS)
    }

#undef HELPER
}

//********************************************************** 
bool utils::stringToEnum (const char *str, eStencilFunc *out)
{
	assert (NULL != out);
	if (NULL == str)
		return false;
    if (0 == str[0])
        return false;
	
#define HELPER(fmt)			if (0 == strcasecmp(str, #fmt)) { *out=eStencilFunc::fmt; return true; }

    HELPER(NEVER)
	HELPER(LESS)
	HELPER(EQUAL)
	HELPER(LESS_EQUAL)
	HELPER(GREATER)
	HELPER(NOT_EQUAL)
	HELPER(GREATER_EQUAL)
	HELPER(ALWAYS)
#undef HELPER	
	return false;
}

//***********************************************
const char*	utils::enumToString (const eCullMode e)
{
#define HELPER(s)	case eCullMode::s: return #s;

    switch (e)
    {
    default:
        DBGBREAK;
        return "??INVALID-VALUE??";

    HELPER(NONE)
	HELPER(CW)
	HELPER(CCW)
    }

#undef HELPER
}

//********************************************************** 
bool utils::stringToEnum (const char *str, eCullMode *out)
{
	assert (NULL != out);
	if (NULL == str)
		return false;
    if (0 == str[0])
        return false;
	
#define HELPER(fmt)			if (0 == strcasecmp(str, #fmt)) { *out=eCullMode::fmt; return true; }

    HELPER(NONE)
	HELPER(CW)
	HELPER(CCW)
#undef HELPER	
	return false;
}

//***********************************************
const char*	utils::enumToString (const eDrawPrimitive e)
{
#define HELPER(s)	case eDrawPrimitive::s: return #s;

    switch (e)
    {
    default:
        DBGBREAK;
        return "??INVALID-VALUE??";

    HELPER(pointList)
	HELPER(lineList)
	HELPER(lineStrip)
	HELPER(trisList)
	HELPER(trisStrip)
	HELPER(trisFan)
    }

#undef HELPER
}

//********************************************************** 
bool utils::stringToEnum (const char *str, eDrawPrimitive *out)
{
	assert (NULL != out);
	if (NULL == str)
		return false;
    if (0 == str[0])
        return false;
	
#define HELPER(fmt)			if (0 == strcasecmp(str, #fmt)) { *out=eDrawPrimitive::fmt; return true; }

    HELPER(pointList)
	HELPER(lineList)
	HELPER(lineStrip)
	HELPER(trisList)
	HELPER(trisStrip)
	HELPER(trisFan)
#undef HELPER	
	return false;
}

//*************************************************************************
const char* utils::enumToString (eSocketError s)
{
    switch (s)
    {
    default: return "invalid value";
    ENUM_TO_STRING_CASE(eSocketError, none);
    ENUM_TO_STRING_CASE(eSocketError, denied);
    ENUM_TO_STRING_CASE(eSocketError, unsupported);
    ENUM_TO_STRING_CASE(eSocketError, tooMany);
    ENUM_TO_STRING_CASE(eSocketError, noMem);
    ENUM_TO_STRING_CASE(eSocketError, addressInUse);
    ENUM_TO_STRING_CASE(eSocketError, addressProtected);
    ENUM_TO_STRING_CASE(eSocketError, alreadyBound);
    ENUM_TO_STRING_CASE(eSocketError, invalidDescriptor);
    ENUM_TO_STRING_CASE(eSocketError, errorSettingReadTimeout);
    ENUM_TO_STRING_CASE(eSocketError, errorSettingWriteTimeout);
    ENUM_TO_STRING_CASE(eSocketError, errorListening);
    ENUM_TO_STRING_CASE(eSocketError, no_such_host);
    ENUM_TO_STRING_CASE(eSocketError, connRefused);
    ENUM_TO_STRING_CASE(eSocketError, timedOut);
    ENUM_TO_STRING_CASE(eSocketError, invalidParameter);
    ENUM_TO_STRING_CASE(eSocketError, unknown);
    ENUM_TO_STRING_CASE(eSocketError, unable_to_handshake);
    }
}

//*************************************************************************
const char* utils::enumToString (const eDataFormat f)
{
	switch (f)
	{
    default: return "invalid value";
    ENUM_TO_STRING_CASE(eDataFormat, _unknown);
    ENUM_TO_STRING_CASE(eDataFormat, _1f32);
    ENUM_TO_STRING_CASE(eDataFormat, _2f32);
    ENUM_TO_STRING_CASE(eDataFormat, _3f32);
    ENUM_TO_STRING_CASE(eDataFormat, _4f32);

    ENUM_TO_STRING_CASE(eDataFormat, _1u32);
    ENUM_TO_STRING_CASE(eDataFormat, _2u32);
    ENUM_TO_STRING_CASE(eDataFormat, _3u32);
    ENUM_TO_STRING_CASE(eDataFormat, _4u32);

    ENUM_TO_STRING_CASE(eDataFormat, _1i32);
    ENUM_TO_STRING_CASE(eDataFormat, _2i32);
    ENUM_TO_STRING_CASE(eDataFormat, _3i32);
    ENUM_TO_STRING_CASE(eDataFormat, _4i32);

    ENUM_TO_STRING_CASE(eDataFormat, _1u16);
    ENUM_TO_STRING_CASE(eDataFormat, _2u16);
    ENUM_TO_STRING_CASE(eDataFormat, _3u16);
    ENUM_TO_STRING_CASE(eDataFormat, _4u16);

    ENUM_TO_STRING_CASE(eDataFormat, _1i16);
    ENUM_TO_STRING_CASE(eDataFormat, _2i16);
    ENUM_TO_STRING_CASE(eDataFormat, _3i16);
    ENUM_TO_STRING_CASE(eDataFormat, _4i16);

    ENUM_TO_STRING_CASE(eDataFormat, _1u8); 
    ENUM_TO_STRING_CASE(eDataFormat, _2u8); 
    ENUM_TO_STRING_CASE(eDataFormat, _3u8); 
    ENUM_TO_STRING_CASE(eDataFormat, _4u8);
    ENUM_TO_STRING_CASE(eDataFormat, _4u8_norm);

    ENUM_TO_STRING_CASE(eDataFormat, _1i8); 
    ENUM_TO_STRING_CASE(eDataFormat, _2i8); 
    ENUM_TO_STRING_CASE(eDataFormat, _3i8); 
    ENUM_TO_STRING_CASE(eDataFormat, _4i8); 

    ENUM_TO_STRING_CASE(eDataFormat, _mat2x2);
    ENUM_TO_STRING_CASE(eDataFormat, _mat3x3);
    ENUM_TO_STRING_CASE(eDataFormat, _mat4x4);
    }
}

//***************************************************
const char* utils::enumToString (const eGPUDescriptrorType s)
{
    switch (s)
    {
    default: return "eGPUDescriptrorType::invalid value";
    case eGPUDescriptrorType::SAMPLER: return "SAMPLER";
    case eGPUDescriptrorType::COMBINED_IMAGE_SAMPLER: return "COMBINED_IMAGE_SAMPLER";
    case eGPUDescriptrorType::TEXTURE2D: return "TEXTURE2D";
    case eGPUDescriptrorType::STORAGE_IMAGE: return "STORAGE_IMAGE";
    case eGPUDescriptrorType::UNIFORM_TEXEL_BUFFER: return "UNIFORM_TEXEL_BUFFER";
    case eGPUDescriptrorType::STORAGE_TEXEL_BUFFER: return "STORAGE_TEXEL_BUFFER";
    case eGPUDescriptrorType::UNIFORM_BUFFER: return "UNIFORM_BUFFER";
    case eGPUDescriptrorType::STORAGE_BUFFER: return "STORAGE_BUFFER";
    case eGPUDescriptrorType::DYNAMIC_UNIFORM_BUFFER: return "DYNAMIC_UNIFORM_BUFFER";
    case eGPUDescriptrorType::DYNAMIC_STORAGE_BUFFER: return "DYNAMIC_STORAGE_BUFFER";
    case eGPUDescriptrorType::INPUT_ATTACHMENT: return "INPUT_ATTACHMENT";
    case eGPUDescriptrorType::UNKNOWN: return "UNKNOWN";
    }
}



//******************************************************************************
u8 gos::utils::bufferWriteF32 (u8 *buffer, f32 val)
{
    const u8 *p = reinterpret_cast<const u8*>(&val);
    buffer[0] = p[0];
    buffer[1] = p[1];
    buffer[2] = p[2];
    buffer[3] = p[3];
    return 4;
}

//******************************************************************************
f32 gos::utils::bufferReadF32 (const u8 *buffer)
{
    f32 ret = 0;
    u8 *p = reinterpret_cast<u8*>(&ret);
    p[0] = buffer[0];
    p[1] = buffer[1];
    p[2] = buffer[2];
    p[3] = buffer[3];
    return ret;
}

//******************************************************************************
u8 gos::utils::bufferWriteU64(u8 *buffer, u64 val)			                
{ 
	buffer[0] = (u8)((val & 0xFF00000000000000) >> 56); 
	buffer[1] = (u8)((val & 0x00FF000000000000) >> 48); 
	buffer[2] = (u8)((val & 0x0000FF0000000000) >> 40); 
	buffer[3] = (u8)((val & 0x000000FF00000000) >> 32);  
	buffer[4] = (u8)((val & 0x00000000FF000000) >> 24); 
	buffer[5] = (u8)((val & 0x0000000000FF0000) >> 16); 
	buffer[6] = (u8)((val & 0x000000000000FF00) >> 8); 
	buffer[7] = (u8) (val & 0x00000000000000FF); 
    return 8;
}

//******************************************************************************
u8 gos::utils::bufferWriteU64_LSB_MSB (u8 *buffer, u64 val)
{ 
	buffer[7] = (u8)((val & 0xFF00000000000000) >> 56); 
	buffer[6] = (u8)((val & 0x00FF000000000000) >> 48); 
	buffer[5] = (u8)((val & 0x0000FF0000000000) >> 40); 
	buffer[4] = (u8)((val & 0x000000FF00000000) >> 32);  
	buffer[3] = (u8)((val & 0x00000000FF000000) >> 24); 
	buffer[2] = (u8)((val & 0x0000000000FF0000) >> 16); 
	buffer[1] = (u8)((val & 0x000000000000FF00) >> 8); 
	buffer[0] = (u8) (val & 0x00000000000000FF); 
    return 8;
}

//******************************************************************************
u64 gos::utils::bufferReadU64(const u8 *buffer)
{ 
    return  (((u64)buffer[0]) << 56) | 
            (((u64)buffer[1]) << 48) | 
            (((u64)buffer[2]) << 40) | 
            (((u64)buffer[3]) << 32) | 
            (((u64)buffer[4]) << 24) | 
            (((u64)buffer[5]) << 16) | 
            (((u64)buffer[6]) << 8) | 
             ((u64)buffer[7]); 
}

//******************************************************************************
u64 gos::utils::bufferReadU64_LSB_MSB (const u8 *buffer)
{
    return  (((u64)buffer[7]) << 56) | 
            (((u64)buffer[6]) << 48) | 
            (((u64)buffer[5]) << 40) | 
            (((u64)buffer[4]) << 32) | 
            (((u64)buffer[3]) << 24) | 
            (((u64)buffer[2]) << 16) | 
            (((u64)buffer[1]) << 8) | 
             ((u64)buffer[0]); 
}

//******************************************************************************
u8 gos::utils::bufferWriteI64(u8 *buffer, i64 val)			                
{ 
	buffer[0] = (u8)((val & 0xFF00000000000000) >> 56); 
	buffer[1] = (u8)((val & 0x00FF000000000000) >> 48); 
	buffer[2] = (u8)((val & 0x0000FF0000000000) >> 40); 
	buffer[3] = (u8)((val & 0x000000FF00000000) >> 32);  
	buffer[4] = (u8)((val & 0x00000000FF000000) >> 24); 
	buffer[5] = (u8)((val & 0x0000000000FF0000) >> 16); 
	buffer[6] = (u8)((val & 0x000000000000FF00) >> 8); 
	buffer[7] = (u8) (val & 0x00000000000000FF); 
    return 8;
}

//******************************************************************************
u8 gos::utils::bufferWriteI64_LSB_MSB (u8 *buffer, i64 val)
{ 
	buffer[7] = (u8)((val & 0xFF00000000000000) >> 56); 
	buffer[6] = (u8)((val & 0x00FF000000000000) >> 48); 
	buffer[5] = (u8)((val & 0x0000FF0000000000) >> 40); 
	buffer[4] = (u8)((val & 0x000000FF00000000) >> 32);  
	buffer[3] = (u8)((val & 0x00000000FF000000) >> 24); 
	buffer[2] = (u8)((val & 0x0000000000FF0000) >> 16); 
	buffer[1] = (u8)((val & 0x000000000000FF00) >> 8); 
	buffer[0] = (u8) (val & 0x00000000000000FF); 
    return 8;
}

//*************************************************************************
u8 utils::simpleChecksum8_calc (const void *bufferIN, u32 lenInBytes)
{
    const u8 *buffer = (const u8*)bufferIN;
    u8 ret = 0;
    for (u32 i=0;i<lenInBytes;i++)
        ret += buffer[i];
    return ret;
}

//*************************************************************************
u16 utils::simpleChecksum16_calc (const void *bufferIN, u32 lenInBytes)
{
    const u8 *buffer = (const u8*)bufferIN;
    u16 ret = 0;
    for (u32 i=0;i<lenInBytes;i++)
        ret += buffer[i];
    return ret;
}


//********************************************************
u32 utils::crc32 (const void *bufferIN, u32 sizeof_buffer)
{
    u32 crc = 0;
    const u8 *buffer = reinterpret_cast<const u8*>(bufferIN);
    for (u32 i=0; i<sizeof_buffer; i++)
    {
        //crc = crc_table[( crc ^ buffer[i]) & 0xFF] ^ (crc >> 8);
        crc = crc_table[ ((crc & 0xFF000000) >> 24) ^ buffer[i]]  ^  ( (crc & 0x00FFFFFF) <<8 );

    }
    return (crc ^ 0xFFFFFFFF);
}

//********************************************************
u32 utils::crc32 (const char *str)
{
	assert (NULL != str);
	return crc32(str, gos::string::utf8::lengthInByte(str));
}

//********************************************************** 
bool utils::isFormatWithDepth (const eImageFormat fmt)
{
	return (static_cast<u8>(fmt) >= 0xE0 && static_cast<u8>(fmt)<=0xEF);
}

//********************************************************** 
bool utils::isFormatWithStencil (const eImageFormat fmt)
{
	return (static_cast<u8>(fmt) >= 0xEA && static_cast<u8>(fmt)<=0xEF);
}

//********************************************************** 
u16 utils::getFormatSize (const eImageFormat fmt)
{
    switch (fmt)
    {
    default:
        DBGBREAK;
        return 0;

    case eImageFormat::U8_RGBA_sRGB: return 4;
    case eImageFormat::U8_RGBA: return 4;
    case eImageFormat::U8_RGB: return 3;
    case eImageFormat::U8_R: return 1;

    case eImageFormat::U16_RGBA: return sizeof(u16)*4;
    case eImageFormat::U16_RGB: return sizeof(u16)*3;
    case eImageFormat::U16_R: return sizeof(u16);

    case eImageFormat::U32_RGBA: return sizeof(u32)*4;
    case eImageFormat::U32_RGB: return sizeof(u32)*3;
    case eImageFormat::U32_R: return sizeof(u32);

    case eImageFormat::F32_RGBA: return sizeof(f32)*4;
    case eImageFormat::F32_RGB: return sizeof(f32)*3;
    case eImageFormat::F32_R: return sizeof(f32);
    }
}

//********************************************************** 
u32 utils::calcClosestPowerOf2 (u32 num)
{
    if (0 == num)
        return 0;

    u32 ret = 1;
    while (ret < num)
    {
        ret <<=1;
    }
    return ret;    
}