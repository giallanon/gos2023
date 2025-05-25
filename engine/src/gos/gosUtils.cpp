#include "gosUtils.h"
#include "gosString.h"
#include "string/gosCompileTimeHashedString.h"

using namespace gos;

#define ENUM_TO_STRING_CASE(enumClass,enumValue) case enumClass::enumValue: return #enumValue

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
inline u32 gos_utils_CRC32 (const u8 *buffer, int len)
{
    if (len < 0)
        return 0xFFFFFFFF;
    return (gos_utils_CRC32(buffer, len-1) >> 8) ^ crc_table[(gos_utils_CRC32(buffer, len-1) ^ buffer[len]) & 0x000000FF];
}

//********************************************************
u32 utils::crc32 (const void *buffer, u32 sizeOfBuffer)
{
	assert (NULL != buffer);
    return (gos_utils_CRC32 (reinterpret_cast<const u8*>(buffer), (int)sizeOfBuffer) ^ 0xFFFFFFFF);
}

//********************************************************
u32 utils::crc32 (const char *str)
{
	assert (NULL != str);
	return crc32(str, gos::string::utf8::lengthInByte(str));
}