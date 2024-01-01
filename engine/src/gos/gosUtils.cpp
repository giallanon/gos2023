#include "gosUtils.h"

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


//******************************************
u8	utils::getFormatSize (eDataFormat f)
{
	switch (f)
	{
    default:
    	DBGBREAK;
	    return 0;

	case eDataFormat::_1f32: 
	case eDataFormat::_1i32: 
	case eDataFormat::_1u32: 
	case eDataFormat::_4u8: 
	case eDataFormat::_4u8_norm:
		return 4;

	case eDataFormat::_2f32: 
	case eDataFormat::_2i32: 
	case eDataFormat::_2u32: 
		return 8;
	
	case eDataFormat::_3f32: 
	case eDataFormat::_3i32: 
	case eDataFormat::_3u32: 
		return 12;
	
	case eDataFormat::_4f32: 
	case eDataFormat::_4i32: 
	case eDataFormat::_4u32: 
		return 16;
	}
}

//******************************************
void utils::bitZERO  (void *dst, u32 sizeof_dst)
{
	assert (NULL != dst);
	assert (sizeof_dst);
	memset (dst, 0, sizeof_dst);
}

//******************************************
void utils::bitSET (void *dst, u32 sizeof_dst, u32 pos)
{
	assert (NULL != dst);
	assert (sizeof_dst);

	const u32	byte = (pos >> 3); //pos / 8;
	const u32	bit  = (pos & 0x07); //pos % 8;
	const u8 mask = 0x01 << bit;

	u8 *p = reinterpret_cast<u8*>(dst);
	p[byte] |= mask;
}

//******************************************
void utils::bitCLEAR (void *dst, u32 sizeof_dst, u32 pos)
{
	assert (NULL != dst);
	assert (sizeof_dst);

	const u32	byte = (pos >> 3); //pos / 8;
	const u32	bit  = (pos & 0x07); //pos % 8;
	const u8 mask = 0x01 << bit;

	u8 *p = reinterpret_cast<u8*>(dst);
	p[byte] &= ~mask;
}

//******************************************
bool utils::isBitSET (const void *dst, u32 sizeof_dst, u32 pos)
{
	assert (NULL != dst);
	assert (sizeof_dst);

	const u32	byte = (pos >> 3); //pos / 8;
	const u32	bit  = (pos & 0x07); //pos % 8;
	const u8 mask = 0x01 << bit;

	const u8 *p = reinterpret_cast<const u8*>(dst);
	return ((p[byte] & mask) != 0);
}

//******************************************
void utils::bitZERO  (u32 *dst)                                         { *dst = 0; }
void utils::bitSET (u32 *dst, u32 pos)									{  assert (NULL != dst); assert (pos < 32); (*dst) |= (0x00000001 << pos); }
void utils::bitCLEAR (u32 *dst, u32 pos)                                {  assert (NULL != dst); assert (pos < 32); (*dst) &= ~((0x00000001 << pos)); }
bool utils::isBitSET (const u32 *dst, u32 pos)                          {  assert (NULL != dst); assert (pos < 32); return (((*dst) & (0x00000001 << pos)) != 0); }

//******************************************
void utils::bitZERO  (u16 *dst)                                        	{ *dst = 0; }
void utils::bitSET (u16 *dst, u32 pos)									{  assert (NULL != dst); assert (pos < 16); (*dst) |= (0x00001 << pos); }
void utils::bitCLEAR (u16 *dst, u32 pos)                                {  assert (NULL != dst); assert (pos < 16); (*dst) &= ~((0x00001 << pos)); }
bool utils::isBitSET (const u16 *dst, u32 pos)                          {  assert (NULL != dst); assert (pos < 16); return (((*dst) & (0x0001 << pos)) != 0); }

//******************************************
void utils::bitZERO  (u8 *dst)                                         	{ *dst = 0; }
void utils::bitSET (u8 *dst, u32 pos)									{  assert (NULL != dst); assert (pos < 8); (*dst) |= (0x001 << pos); }
void utils::bitCLEAR (u8 *dst, u32 pos)                                	{  assert (NULL != dst); assert (pos < 8); (*dst) &= ~((0x001 << pos)); }
bool utils::isBitSET (const u8 *dst, u32 pos)                          	{  assert (NULL != dst); assert (pos < 8); return (((*dst) & (0x01 << pos)) != 0); }

//******************************************
void utils::byteSET (u32 *dst, u8 value, u32 pos)
{
	assert (NULL != dst); 
	assert (pos < 4);

	const u32 shift = (pos<<3);
	(*dst) &= ~(0x000000FF << shift );
	(*dst) |= (((u32)value) << shift);
}

//******************************************
u8 utils::byteGET (const u32 *dst, u32 pos)
{
	assert (NULL != dst); 
	assert (pos < 4);

	const u32 shift = (pos<<3);
	return static_cast<u8>( ((*dst) >> shift) & 0x000000FF );
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