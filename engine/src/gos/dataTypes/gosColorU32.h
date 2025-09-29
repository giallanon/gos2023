#ifndef _gosColorU32_h_
#define _gosColorU32_h_
#include "../gosEnumAndDefine.h"

namespace gos
{
	/*================================================================
	 * ColorU32
	 *
	 * Accetta anche la sintassi stringa nei seguenti formati:
	 *	#rgb, #argb, #rrggbb, #aarrggbb
	 *	Al posto di un colore, e' possibile indicare un punto "." per dire "non modificare il colore di quel canale"-
	 *	Ad esempio:  #..f0 significa non toccare alfa, non toccare red, metti green=ff, metti blue=00
	 *	Per i colori che non indicano un alfa, si assume alfa = 1
	 *
	 *	Implementato come un u32 nel formato ARGB
	 *================================================================*/
	class ColorU32
	{
	public:
		static u8			multiplyAlpha (u8 alpha1_0_255, u8 alpha2_0_255)	{ return static_cast<u8>((static_cast<u16>(alpha1_0_255) * static_cast<u16>(alpha2_0_255)) / 255); }

	public:
							ColorU32 ()											{ argb = 0; }
							ColorU32 (u8 r, u8 g, u8 b)							{ set (r, g, b); }
							ColorU32 (u8 a, u8 r, u8 g, u8 b)					{ set (a, r, g, b); }
							ColorU32 (u32 argb)									{ set (argb); }
							ColorU32 (const char *s)							{ setFromString(s); }

				ColorU32&	operator= (const char *s)							{ setFromString(s); return *this; }
				ColorU32&	operator= (const ColorU32 &b)						{ argb=b.argb; return *this; }
				bool		operator== (const ColorU32 &b) const				{ return argb==b.argb; }
				bool		operator!= (const ColorU32 &b) const				{ return argb!=b.argb; }

				void		set (u8 r, u8 g, u8 b)								{ set (0xff, r, g, b); }
				void		set (u8 a, u8 r, u8 g, u8 b)						{ argb = (u32)b | (((u32)a) << 24) | (((u32)r) << 16) | (((u32)g) << 8); }
				void		set (u32 argbIN)									{ argb = argbIN; }
				void		setA (u8 aa)										{ argb &= 0x00FFFFFF; argb |= (((u32)aa) << 24); }
				void		setFromString (const char *s, u32 lenOfS=u32MAX);

				u8			a() const											{ return (u8)((argb & 0xff000000) >> 24); }
				u8			r() const											{ return (u8)((argb & 0x00ff0000) >> 16); }
				u8			g() const											{ return (u8)((argb & 0x0000ff00) >> 8); }
				u8			b() const											{ return (u8)((argb & 0x000000ff)); }

				void		sRGBToLinear();

		static	void		lerp (const ColorU32 &c1, const ColorU32 &c2, f32 t01, ColorU32 *out);
		static	u8			applyAlpha (u8 colSRC_0_255, u8 alpha_0_255);

	public:
		u32		argb;
	};
} //namespace gos
#endif //_gosColorU32_h_