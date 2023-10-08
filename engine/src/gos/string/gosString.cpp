#include "../gos.h"
#include "../gosString.h"


using namespace gos;

//**************************************************
bool string::strANSItoUTF8 (const char* in, u8 *out, u32 sizeOfOut)
{
	return strANSItoUTF8 (in, (u32)strlen(in), out, sizeOfOut);
}

//**************************************************
bool string::strANSItoUTF8  (const char* in, u32 lenOfIN, u8* out,  u32 sizeOfOut)
{
	assert (out && sizeOfOut);

	if (NULL == in)
	{
		out[0] = 0;
		return true;
	}
	
	u32 i = 0;
	u32 n = 0;
	while (i < lenOfIN)
	{
		ANSIChar a;
		UTF8Char u;

		a.data = in[i++];
		if (!string::ansi::toUTF8(a, &u))
		{
			DBGBREAK;
			out[n] = 0;
			return false;
		}

		u8 nBytes = 1;
		if (u.data[3]) nBytes = 4;
		else if (u.data[2]) nBytes = 3;
		else if (u.data[1]) nBytes = 2;

		if (n + nBytes >= sizeOfOut)
		{
			DBGBREAK; //non c'� abbastanza spazio in *out
			out[n] = 0;
			return false;
		}

		memcpy (&out[n], u.data, nBytes);
		n += nBytes;
	}
	out[n] = 0;
	return true;
}

//**************************************************
bool string::strANSItoUTF16 (const char* in, u16* out, u32 sizeOfOutInBytes)
{
	assert (out && sizeOfOutInBytes);

	if (NULL == in)
	{
		out[0] = 0;
		return true;
	}
	
	u32 i = 0;
	u32 n = 0;
	while (in[i])
	{
		ANSIChar a;
		UTF16Char u;

		a.data = in[i++];
		if (!string::ansi::toUTF16(a, &u))
		{
			DBGBREAK;
			out[n] = 0;
			return false;
		}

		u8 nBytes = 2;
		if (u.data[1]) 
			nBytes = 4;

		if (n + nBytes >= sizeOfOutInBytes)
		{
			DBGBREAK; //non c'� abbastanza spazio in *out
			out[n] = 0;
			return false;
		}

		memcpy (&out[n], u.data, nBytes);
		n += nBytes;
	}
	out[n] = 0;
	return true;
}

//**************************************************
bool string::strUTF8toUTF16 (const u8 *in, u16* outIN, u32 sizeOfOutInBytes)
{
	assert (outIN && sizeOfOutInBytes);

	outIN[0] = 0;
	if (NULL == in)
		return true;

	u8 *out = (u8*)outIN;
	u32 n = 0;
	string::utf8::Iter parser;
	parser.setup (in);
	while (!parser.getCurChar().isEOF())
	{
		UTF16Char u;
		if (!string::utf8::toUTF16 (parser.getCurChar(), &u))
		{
			DBGBREAK;
			out[n] = 0;
			return false;
		}

		u8 nBytes = 2;
		if (u.data[1]) 
			nBytes = 4;

		if (n + nBytes >= sizeOfOutInBytes)
		{
			DBGBREAK; //non c'� abbastanza spazio in *out
			out[n] = 0;
			return false;
		}

		memcpy (&out[n], u.data, nBytes);
		n += nBytes;

		parser.advanceOneChar();
	}

	return true;
}

//**************************************************
bool string::strUTF16toUTF8 (const u16* in, u8 *out, u32 sizeOfOutInBytes)
{
	assert (out && sizeOfOutInBytes);
	out[0] = 0x00;

	if (NULL == in)
		return true;

	u32 lenInBytes = utf16::lengthInBytes(in);
	u32 ct = 0;
	while (lenInBytes)
	{
		UTF16Char ch;
		const u8 nBytesUsed = utf16::extractAChar (&in[ct], lenInBytes, &ch);

		assert (lenInBytes >= nBytesUsed);
		lenInBytes -= nBytesUsed;
		ct += (nBytesUsed / 2);

		UTF8Char utf8Chars[4];
		if (!utf16::toUTF8 (ch, utf8Chars))
		{
			DBGBREAK;
			out[0] = 0x00;
			return false;
		}

		for (u8 i = 0; i < 4; i++)
		{
			if (utf8Chars[i].isEOF())
				break;
			utf8::appendUTF8Char (out, sizeOfOutInBytes, utf8Chars[i]);
		}
	}


	return true;
}