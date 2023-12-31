#include "gosUtils.h"

using namespace gos;

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

