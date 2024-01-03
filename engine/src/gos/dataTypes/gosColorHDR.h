#ifndef _gosColorHDR_h_
#define _gosColorHDR_h_
#include <math.h>
#include "../gosEnumAndDefine.h"

#ifdef GOS_COMPILER__GCC
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wpedantic"
#endif
#ifdef GOS_COMPILER__MSVC
	#pragma warning(disable:4201)
#endif

namespace gos
{
	/*================================================================
	 * ColorHDR
	 *
	 *	Accetta anche la sintassi stringa nei seguenti formati:
	 *	#rgb, #argb, #rrggbb, #aarrggbb
	 *	Al posto di un colore, � possibile indicare un punto "." per dire "non modificare il colore di quel canale"-
	 *	Ad esempio:  #..f0 significa non toccare alfa, non toccare red, metti green=ff, metti blue=00
	 *	Per i colori che non indicano un alfa, si assume alfa = 1
	 *================================================================*/
	class ColorHDR
	{
	public:
							ColorHDR ()											{ set (1.0f, 1, 1, 1); }
							ColorHDR (f32 r, f32 g, f32 b)						{ set (r, g, b); }
							ColorHDR (f32 a, f32 r, f32 g, f32 b)				{ set (a, r, g, b); }
							ColorHDR (u32 colARGB)								{ fromU32(colARGB); }
							ColorHDR (const u8* s)								{ setFromString(s); }

				ColorHDR&	operator= (const u8* s)								{ setFromString(s); return *this; }
				ColorHDR&	operator= (const ColorHDR &b)						{ set (b.col.a, b.col.r, b.col.g, b.col.b); return *this; }
				ColorHDR&	operator*= (f32 alfa)								{ col.a*=alfa; return *this; }
				bool		operator== (const ColorHDR &b) const				{ return memcmp(col.rgba, b.col.rgba, sizeof(col.rgba))==0; }
				bool		operator!= (const ColorHDR &b) const				{ return memcmp(col.rgba, b.col.rgba, sizeof(col.rgba))!=0; }

				void		set (f32 r, f32 g, f32 b)							{ col.a=1.0f;	col.r=r; col.g=g; col.b=b; }
				void		set (f32 a, f32 r, f32 g, f32 b)					{ col.a=a;		col.r=r; col.g=g; col.b=b; }
				void		setFromString (const u8* s, u32 lenOfS=u32MAX);
				void		setU32_argb (u32 argb)								{ fromU32(argb); }
				void		setU8_argb (u8 a, u8 r, u8 g, u8 b)					{ col.a = (f32)a / 255.0f; col.r = (f32)r / 255.0f; col.g = (f32)g / 255.0f; col.b = (f32)b / 255.0f; }
				void		setU8_a (u8 a)										{ col.a = (f32)a / 255.0f; }
				void		setU8_r (u8 r)										{ col.r = (f32)r / 255.0f; }
				void		setU8_g (u8 g)										{ col.g = (f32)g / 255.0f; }
				void		setU8_b (u8 b)										{ col.b = (f32)b / 255.0f; }

				void		sRGBToLinear()										{ col.r = powf (col.r, 2.2f); col.g = powf (col.g, 2.2f); col.b = powf (col.b, 2.2f); }
				void		sRGBToLinear(f32 *out) const 
							{ 
								//vedi http://chilliant.blogspot.com/2012/08/srgb-approximations-for-hlsl.html
								//float3 RGB = sRGB * (sRGB * (sRGB * 0.305306011 + 0.682171111) + 0.012522878);
								out[0] = priv_sRGBSingleColorToLinear(col.r);
								out[1] = priv_sRGBSingleColorToLinear(col.g);
								out[2] = priv_sRGBSingleColorToLinear(col.b);
								out[3] = col.a;
							}
				u32			toU32ARGB() const;
				void		fromU32 (u32 argb);

		static	void		lerp (const ColorHDR &c1, const ColorHDR &c2, f32 t01, ColorHDR *out)
							{
								out->col.r = c1.col.r + (c2.col.r - c1.col.r) * t01;
								out->col.g = c1.col.g + (c2.col.g - c1.col.g) * t01;
								out->col.b = c1.col.b + (c2.col.b - c1.col.b) * t01;
								out->col.a = c1.col.a + (c2.col.a - c1.col.a) * t01;
							}
	public:
		union
		{
			struct
			{
				f32	r,g,b,a;
			};
			f32	rgba[4];
		}col;


	private:
		f32					priv_sRGBSingleColorToLinear(f32 colIN) const
		{
			//float3 RGB = sRGB * (sRGB * (sRGB * 0.305306011 + 0.682171111) + 0.012522878);
			//float3 RGB = sRGB * (sRGB * a + 0.012522878);
			//float3 RGB = sRGB * b;
			const f32 a = (colIN * 0.305306011f + 0.682171111f);
			const f32 b = (colIN * a + 0.012522878f);
			return colIN * b;			
		}
	};

	ColorHDR				operator* (const ColorHDR &a, f32 alfa);
	
}//namespace gos

#ifdef GOS_COMPILER__GCC
	#pragma GCC diagnostic pop
#endif
#ifdef GOS_COMPILER__MSVC
	#pragma warning(default:4201)
#endif

#endif //_gosColorHDR_h_
