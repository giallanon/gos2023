#include "../gos.h"
#include "../gosString.h"


using namespace gos;

#define UTF16_UNI_SUR_HIGH_START	0xD800
#define UTF16_UNI_SUR_HIGH_END		0xDBFF
#define UTF16_UNI_SUR_LOW_START		0xDC00
#define UTF16_UNI_SUR_LOW_END		0xDFFF
#define UTF16_UNI_MAX_LEGAL_UTF32	0x0010FFFF
#define UTF16_HALFBASE				0x00010000

//***************************************************************
bool string::utf32::toUTF8(const UTF32Char &in, UTF8Char *out)
{
	if (in.data < 0x00000080)
	{
		out->data[0] = (u8)in.data;
		out->data[1] = out->data[2] = out->data[3] = 0;
		return true;
	}

	// Upper bounds checking.
	if (in.data > 0x0010FFFF)
	{
		// Invalid character.  What should we do?
		// Return a default character.
		DBGBREAK;
		out->data[0] = out->data[1] = out->data[2] = out->data[3] = 0;
		return false;
	}

	u8 c[4] = { 0,0,0,0 };

	// Every other case uses bit markers.
	// Start from the lowest encoding and check upwards.
	u32 ui32High = 0x00000800;
	u32 ui32Mask = 0xC0;
	u8 nU8Needed = 2;
	while (in.data >= ui32High)
	{
		ui32High <<= 5;
		ui32Mask = (ui32Mask >> 1) | 0x80UL;
		++nU8Needed;
	}

	// Encode the first byte.
	u32 ui32BottomMask = ~((ui32Mask >> 1) | 0xFFFFFF80);
	c[0] = (u8)(ui32Mask | ((in.data >> ((nU8Needed - 1) * 6)) & ui32BottomMask));

	// Now fill in the rest of the bits.
	u8 iNext = 1;
	for (u32 I = nU8Needed - 1; I--;)
	{
		// Shift down, mask off 6 bits, and add the 10xxxxxx flag.
		c[iNext++] = (u8)(((in.data >> (I * 6)) & 0x3F) | 0x80);
	}

	if (iNext <= 4)
	{
		memcpy(out->data, c, 4);
		return true;
	}

	DBGBREAK;
	return false;
}


//***************************************************************
bool string::utf32::toUTF16 (const UTF32Char &in, UTF16Char *out)
{
	if (in.data <= 0xFFFF)
	{ 
		/* UTF-16 surrogate values are illegal in UTF-32; 0xffff or 0xfffe are both reserved values */
		if (in.data >= UTF16_UNI_SUR_HIGH_START && in.data <= UTF16_UNI_SUR_LOW_END)
			return false;

		out->data[0] = (u16)in.data;
		out->data[1] = 0;
		return true;
	}

	if (in.data > UTF16_UNI_MAX_LEGAL_UTF32)
	{
		DBGBREAK; //codifica invalida
		return false;
	}

	/* target is a character in range 0xFFFF - 0x10FFFF. */
	u32 u = in.data;
	u -= UTF16_HALFBASE;
	out->data[0] = static_cast<u16>(((u >> 10) + UTF16_UNI_SUR_HIGH_START));
	out->data[1] = static_cast<u16>(((u & 0x000003FF) + UTF16_UNI_SUR_LOW_START));
	return true;
}

//**************************************************
u32 string::utf32::lengthInBytes (const u32* str)
{
	if (NULL == str)
		return 0;

	//il primo 0x0000 che trovo...
	u32 i = 0;
	while (str[i] != 0x0000)
	{
		i++;
#ifdef _DEBUG
		//se siamo arrivati cos� avanti, prob � un errore
		if (i >= 4096)
			DBGBREAK;
#endif
	}
	return i*4;
}

//**************************************************
u8 string::utf32::extractAChar (const u32* p, u32 lenInBytes, UTF32Char *out)
{
	if (NULL == p || lenInBytes<4)
		return 0;

	if (p[0] == 0x00)
	{
		out->data = 0;
		return 0;
	}

	out->data = p[0];
	return 4;
}