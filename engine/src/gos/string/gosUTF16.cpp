#include "../gos.h"
#include "../gosString.h"


using namespace gos;

//**************************************************
bool string::utf16::toUTF8  (const UTF16Char &in, UTF8Char *out)		
{ 
	UTF32Char temp; 
	if (utf16::toUTF32(in, &temp)) 
		return utf32::toUTF8(temp, out); 
	return false; 
}

//**************************************************
bool string::utf16::toUTF32(const UTF16Char &in, UTF32Char *out)
{
	if (in.data[0] == 0)
	{
		out->data = 0;
		return true;
	}

	if ((in.data[0] & 0xFC00) != 0xD800)
	{
		out->data = (u32)in.data[0];
		return true;
	}

	//se arriviamo qui, vuol dire che la codifica prevede l'utilizzo di 2 u16
	if ((in.data[1] & 0xFC00) != 0xDC00)
	{
		DBGBREAK; //codifica invalida
		return false;
	}

	out->data = (in.data[0] & 0x03FF);
	out->data <<= 10;
	out->data |= (in.data[1] & 0x03FF);
	return true;
}




//**************************************************
u32 string::utf16::lengthInBytes (const u16* p)
{
	if (NULL == p)
		return 0;

	//il primo 0x0000 che trovo...
	u32 i = 0;
	while (p[i] != 0x0000)
	{
		i++;
#ifdef _DEBUG
		//se siamo arrivati cos� avanti, prob � un errore
		if (i >= 8192)
			DBGBREAK;
#endif
	}
	return i*2;
}

//**************************************************
u8 string::utf16::extractAChar (const u16* p, u32 lenInBytes, UTF16Char *out)
{
	if (NULL == p || lenInBytes<2)
		return 0;

	if (p[0] == 0x00)
	{
		out->data[0] = out->data[1] = 0;
		return 0;
	}

	if ((p[0] & 0xFC00) != 0xD800)
	{
		out->data[0] = p[0];
		out->data[1] = 0;
		return 2;
	}

	//se arriviamo qui, vuol dire che la codifica prevede l'utilizzo di 2 u16
	if (lenInBytes < 4)
	{
		DBGBREAK; //seq invalida, mi aspetto 4 bytes
		return 0;
	}
	out->data[0] = p[0];
	out->data[1] = p[1];
	return 4;
}

//***************************************************************
u32 string::utf16::rtrim(u16 *s)
{
	u32 n = utf16::lengthInBytes(s) / 2;
	if (n == 0)
		return 0;

	u32 i = n -1;
	while (i>0 && s[i]==0x00)
		--i;
	if (s[i] == 0x00)
	{
		assert (i == 0);
		return 0;
	}

	while (i>0 && s[i]==' ')
		--i;
	
	if (s[i] == ' ')
	{
		assert (i == 0);
		s[i] = 0x00;
		return 0;
	}
	
	i++;
	assert (i <= n);
	s[i] = 0;
	return i*2;
}

//***************************************************************
void string::utf16::concatFromASCII (u16 *dst, u32 sizeofDstInBytes, const char* src)
{
	if (NULL == src)
		return;
	if (src[0] == 0x00)
		return;

	if (NULL == dst || sizeofDstInBytes == 0)
		return;

	const u32 nMax = (sizeofDstInBytes / 2) -1;
	u32 n = utf16::lengthInBytes(dst) / 2;
	u32 i = 0;
	while (src[i] != 0x00 && n<nMax)
		dst[n++] = src[i++];
	dst[n] = 0x00;
}

//***************************************************************
void string::utf16::prepend (u16 *dst, u32 sizeOfDstInBytes, const u16* const strToPrepend)
{
	const u32 lenPrependInBytes = utf16::lengthInBytes(strToPrepend);
	if (0 == lenPrependInBytes)
		return;

	u32 lenDst = utf16::lengthInBytes(dst) / 2;

	//devo shiftare a destra [dst] di [lenPrepend] bytes
	u32 nBytesToShift = (lenDst+1) * 2;
	if (sizeOfDstInBytes <= lenPrependInBytes + nBytesToShift)
	{
		nBytesToShift = 0;
		if (sizeOfDstInBytes >= lenPrependInBytes+2)
			nBytesToShift = (sizeOfDstInBytes - lenPrependInBytes) - 2;
	}

	if (nBytesToShift)
        memmove(&dst[lenPrependInBytes/2], dst, nBytesToShift);

	memcpy(dst, strToPrepend, lenPrependInBytes);

}