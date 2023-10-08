#include "../gos.h"
#include "../gosString.h"

using namespace gos;

//**************************************************
u32 string::ansi::lengthInBytes (const char* p)
{
    if (NULL == p)
        return 0;

    //il primo 0x00 che trovo...
    u32 i = 0;
    while (p[i] != 0x00)
    {
        i++;
#ifdef _DEBUG
        //se siamo arrivati così' avanti, prob e' un errore
        if (i >= 16384)
            DBGBREAK;
#endif
    }
    return i;
}


//**************************************************
bool string::ansi::toUTF8 (const ANSIChar& in, UTF8Char* out)
{
    if ((u8)in.data < 128)
	{
		out->data[0] = in.data;
		out->data[1] = out->data[2] = out->data[3] = 0;
	}
    else
	{
        out->data[0] = 0xc2 + ((u8)in.data > 0xbf);
		out->data[1] =(in.data & 0x3f) + 0x80;
		out->data[2] = out->data[3] = 0;
	}
	return true;
}

//**************************************************
bool string::ansi::toUTF16  (const ANSIChar& in, UTF16Char* out)		{ UTF8Char temp; if (!ansi::toUTF8(in, &temp)) return false; return utf8::toUTF16(temp, out); }

//**************************************************
bool string::ansi::toUTF32  (const ANSIChar& in, UTF32Char* out)		{ UTF8Char temp; if (!ansi::toUTF8(in, &temp)) return false; return utf8::toUTF32(temp, out); }

//**************************************************
u8 string::ansi::extractAChar (const char* p, u32 lenInBytes, ANSIChar *out)
{
	if (NULL == p || lenInBytes<1)
		return 0;
	out->data = p[0];
	if (p[0] == 0x00)
		return 0;
	return 1;
}

//*******************************************************************
static bool priv_convert_HexDigitToInt (char hex, u32 *out)
{
	if (hex >='0' && hex<='9')
	{
		*out = (u32)(hex - '0');
		return true;
	}
	
	if (hex >='A' && hex<='F')
	{
		hex -= 'A';
		hex += 'a';
	}

	if (hex>='a' && hex<='f')
	{
		hex -='a';
		hex += 10;
		*out = (u32)hex;
		return true;
	}

	return false;
}

//*******************************************************************
bool string::ansi::hexToInt (const char *s, u32 *out, u32 lenInByteOf_s)
{
	assert (NULL != s && lenInByteOf_s>0);

	u32 len;
	if (lenInByteOf_s != u32MAX)
		len = lenInByteOf_s;
	else
	{
		len = 0;
		while (s[len] != 0)
			++len;
	}


	--len;
	if (!priv_convert_HexDigitToInt (s[len], out))
		return false;

	u32 mul = 16;
	while (len--)
	{
		u32 n;
		if (!priv_convert_HexDigitToInt (s[len], &n))
			return false;
		*out += n*mul;
		mul <<= 4;
	}

	return true;
}

//*******************************************************************
f32 string::ansi::toF32  (const char *s, u32 lenOfS)
{ 
	if (NULL==s || lenOfS==0) 
		return 0; 

    if (lenOfS == u32MAX)
		return (f32)atof(s); 

	char temp[64];
	if (lenOfS > 63)
	{
		DBGBREAK;
		lenOfS = 62;
	}
	memcpy (temp, s, lenOfS);
	temp[lenOfS] = 0;
	return (f32)atof(temp); 
}
