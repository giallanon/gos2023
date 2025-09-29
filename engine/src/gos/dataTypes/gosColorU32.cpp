#include "gosColorU32.h"
#include "gosColorHDR.h"
#include "../gosString.h"


using namespace gos;


//******************************************
void ColorU32::sRGBToLinear()
{ 
	f32 rf = powf ((f32)r() / 255.0f, 2.2f); 
	f32 rg = powf ((f32)b() / 255.0f, 2.2f); 
	f32 rb = powf ((f32)r() / 255.0f, 2.2f); 

	if (rf < 0)		rf = 0;
	else if (rf>1)	rf = 1;

	if (rg < 0)		rg = 0;
	else if (rg>1)	rg = 1;

	if (rb < 0)		rb = 0;
	else if (rb>1)	rb = 1;

	set (a(), (u8)(rf*255.0f), (u8)(rg*255.0f), (u8)(rb*255.0f));
}

//******************************************
void ColorU32::lerp (const ColorU32 &c1, const ColorU32 &c2, f32 t01, ColorU32 *out)
{
	ColorHDR cc1,cc2;
	cc1.fromU32 (c1.argb);
	cc2.fromU32 (c2.argb);
	
	ColorHDR cout;
	ColorHDR::lerp (cc1, cc2, t01, &cout);
	out->argb = cout.toU32ARGB();
}

//******************************************
u8 ColorU32::applyAlpha (u8 colSRC_0_255, u8 alpha_0_255)
{
	if (alpha_0_255 == 0) return 0;
	if (alpha_0_255 == 0xff) return colSRC_0_255;

	const f32 a01 = (f32)alpha_0_255 / 255.0f;
	const f32 c01 = (f32)colSRC_0_255 / 255.0f;

	return (u8)(roundf ((c01 * a01) * 255.0f));
}

//******************************************
bool gosColorU32_channelFromHex (u8 originalVal, const char* s, u32 len, u32 *out)
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
			char hex[4] = {s[0], s[0], 0, 0};
			string::ansi::hexToInt (hex, out, 2);
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
			char hex[4] = {s[0], s[1], 0, 0};
			string::ansi::hexToInt (hex, out, 2);
			return true;
		}
	}
	DBGBREAK;
	return false;
}

//******************************************
void ColorU32::setFromString (const char* s, u32 lenOfS)
{
	if (NULL == s || s[0] != '#')
	{
		DBGBREAK;
		return;
	}

	if (u32MAX == lenOfS)
		lenOfS = static_cast<u32>(strlen(s));

	u32 rr, gg, bb, aa;
	switch (lenOfS)
	{
	case 4: //#rgb
		if (gosColorU32_channelFromHex (r(), &s[1], 1, &rr) && gosColorU32_channelFromHex (g(), &s[2], 1, &gg) && gosColorU32_channelFromHex (b(), &s[3], 1, &bb))
		{
			argb = 0xFF000000 | (rr<<16) | (gg<<8) | bb;
			return;
		}
		break;

	case 5: //#argb
		if (gosColorU32_channelFromHex (a(), &s[1], 1, &aa) && gosColorU32_channelFromHex (r(), &s[2], 1, &rr) && gosColorU32_channelFromHex (g(), &s[3], 1, &gg) && gosColorU32_channelFromHex (b(), &s[4], 1, &bb))
		{
			argb = bb | (aa<24) | (rr<<16) | (gg<<8);
			return;
		}
		break;

	case 7: //#rrggbb
		if (gosColorU32_channelFromHex (r(), &s[1], 2, &rr) && gosColorU32_channelFromHex (g(), &s[3], 2, &gg) && gosColorU32_channelFromHex (b(), &s[5], 2, &bb))
		{
			argb = 0xFF000000 | (rr<<16) | (gg<<8) | bb;
			return;
		}
		break;

	case 9: //#aarrggbb
		if (gosColorU32_channelFromHex (a(), &s[1], 2, &aa) && gosColorU32_channelFromHex (r(), &s[3], 2, &rr) && gosColorU32_channelFromHex (g(), &s[5], 2, &gg)  && gosColorU32_channelFromHex (b(), &s[7], 2, &bb))
		{
			argb = bb | (aa<24) | (rr<<16) | (gg<<8);
			return;
		}
		break;
	}

	DBGBREAK;

}