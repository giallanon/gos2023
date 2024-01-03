#include "gosColorHDR.h"
#include "../gosString.h"

using namespace gos;

//******************************************
gos::ColorHDR gos::operator* (const gos::ColorHDR &a, f32 alfa)
{
	ColorHDR ret=a; ret*=alfa; return ret; 
}


//******************************************
u32	ColorHDR::toU32ARGB() const
{
	const u32	a = (u32)(floorf(col.a*255.0f));
	const u32	r = (u32)(floorf(col.r*255.0f));
	const u32	g = (u32)(floorf(col.g*255.0f));
	const u32	b = (u32)(floorf(col.b*255.0f));

	assert (a<=0xff && r<=0xff && g<=0xff && b<=0xff);

	const u32 ret = (a<<24) | (r<<16) | (g<<8) | b;
	return ret;
}

//******************************************
void ColorHDR::fromU32 (u32 argb)
{
	col.a = (f32) ((argb & 0xFF000000) >> 24) / 255.0f;
	col.r = (f32) ((argb & 0x00FF0000) >> 16) / 255.0f;
	col.g = (f32) ((argb & 0x0000FF00) >> 8) / 255.0f;
	col.b = (f32) ((argb & 0x000000FF)) / 255.0f;
}

bool gosColorHDR_channelFromHex (f32 originalVal, const u8* s, u32 len, f32 *out)
{
	if (len==1)
	{
		if (s[0] == '.')
		{
			*out = originalVal;
			return true;
		}

		if ((s[0]>='0' && s[0]<='9') || (s[0]>='a' && s[0]<='f') || (s[0]>='A' && s[0]<='F'))
		{
			u32 n;
			const u8 hex[4] = {s[0], s[0], 0, 0};
			string::ansi::hexToInt (reinterpret_cast<const char*>(hex), &n, 2);
			*out = (f32)n / 255.0f;
			return true;
		}
	}
	else if (len == 2)
	{
		if (s[0] == '.' && s[1] == '.')
		{
			*out = originalVal;
			return true;
		}
		if (
			((s[0]>='0' && s[0]<='9') || (s[0]>='a' && s[0]<='f') || (s[0]>='A' && s[0]<='F')) &&
			((s[1]>='0' && s[1]<='9') || (s[1]>='a' && s[1]<='f') || (s[1]>='A' && s[1]<='F'))
			)
		{
			u32 n;
			const u8 hex[4] = {s[0], s[1], 0, 0};
			string::ansi::hexToInt (reinterpret_cast<const char*>(hex), &n, 2);
			*out = (f32)n / 255.0f;;
			return true;
		}
	}
	DBGBREAK;
	return false;
}

//******************************************
void ColorHDR::setFromString (const u8* s, u32 lenOfS)
{
	if (NULL == s || s[0] != '#')
	{
		DBGBREAK;
		return;
	}

	if (u32MAX == lenOfS)
		lenOfS = string::utf8::lengthInByte(s);

	f32 rr, gg, bb, aa;
	switch (lenOfS)
	{
	case 4: //#rgb
		if (gosColorHDR_channelFromHex (col.r, &s[1], 1, &rr) && gosColorHDR_channelFromHex (col.g, &s[2], 1, &gg) && gosColorHDR_channelFromHex (col.b, &s[3], 1, &bb))
		{
			col.a=1; col.r=rr; col.g=gg; col.b=bb;
			return;
		}
		break;

	case 5: //#argb
		if (gosColorHDR_channelFromHex (col.a, &s[1], 1, &aa) && gosColorHDR_channelFromHex (col.r, &s[2], 1, &rr) && gosColorHDR_channelFromHex (col.g, &s[3], 1, &gg) && gosColorHDR_channelFromHex (col.b, &s[4], 1, &bb))
		{
			col.a=aa; col.r=rr; col.g=gg; col.b=bb;
			return;
		}
		break;

	case 7: //#rrggbb
		if (gosColorHDR_channelFromHex (col.r, &s[1], 2, &rr) && gosColorHDR_channelFromHex (col.g, &s[3], 2, &gg) && gosColorHDR_channelFromHex (col.b, &s[5], 2, &bb))
		{
			col.a=1; col.r=rr; col.g=gg; col.b=bb;
			return;
		}
		break;

	case 9: //#aarrggbb
		if (gosColorHDR_channelFromHex (col.a, &s[1], 2, &aa) && gosColorHDR_channelFromHex (col.r, &s[3], 2, &rr) && gosColorHDR_channelFromHex (col.g, &s[5], 2, &gg)  && gosColorHDR_channelFromHex (col.b, &s[7], 2, &bb))
		{
			col.a=aa; col.r=rr; col.g=gg; col.b=bb;
			return;
		}
		break;
	}

	DBGBREAK;
}

