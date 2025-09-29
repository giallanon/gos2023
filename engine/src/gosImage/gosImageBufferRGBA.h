#ifndef _gosImageBufferRGBA_h_
#define _gosImageBufferRGBA_h_
#include "gosImageBufferA.h"
#include "../gos/dataTypes/gosColorU32.h"
#include "../gos/dataTypes/gosPoint2.h"


namespace gos
{ 
	namespace image
	{
		/***********************************************
		 * BufferRGBA
		 *
		 * sprite/immagine nel formato 32bit R G B A
		 */
		class BufferRGBA
		{
		public:
			enum class eAlphaOP : u8
			{
				copy = 0,					//dst.rgba = src.rgba
				copyOnlyIfAlphaIsNotZero = 1	//se src.a!=0, allora dst.rgba=src.rgba, altrimenti lascia inalterato dst
			};
		public:
							//============================ static
			static void		blt (BufferRGBA &dst, i16 xDST, i16 yDST, const BufferRGBA &src, u16 x1, u16 y1, i16 dimx, i16 dimy, eAlphaOP alphaOP);
			
			static void		bltRChannelOnly (BufferRGBA &dst, i16 xDST, i16 yDST, const BufferRGBA &src, u16 x1, u16 y1, i16 dimx, i16 dimy, u8 r, u8 g, u8 b);
								//usa rchannel come intensita per il colore r,g,b indicato come parametro

			static void		bltMask (BufferRGBA &dst, i16 xDST, i16 yDST, const BufferA &src, u16 x1, u16 y1, i16 dimx, i16 dimy, u8 r, u8 g, u8 b);
			static void		bltMask (BufferRGBA &dst, i16 xDST, i16 yDST, const u8 *srcBufferA, u16 srcBufferW, u16 srcBufferH, u16 x1, u16 y1, i16 dimx, i16 dimy, u8 r, u8 g, u8 b);

			
							//============================ fn
							BufferRGBA()																	{ _bufferRGBA = NULL; _w = _h = 0; }
							~BufferRGBA()																	{ }

			bool			loadFromFile (Allocator *allocator, const char* filePathAndName);
			bool			loadFromFileInMemory (Allocator *allocator, const void *fileData, u32 sizeOfData);
							//accettano PNG, GIF, BMP, JPG		

			bool			saveAsTGA (const char *filename) const;

			bool			alloc (Allocator *allocator, u16 w, u16 h);
			void			free  (Allocator *allocator);

			void			clear (u8 r, u8 g, u8 b, u8 a = 0xff);
			void			clear (const gos::ColorU32 &col)											{ clear (col.r(), col.g(), col.b(), col.a()); }

			void			putPixel (i16 x, i16 y, u8 r, u8 g, u8 b, u8 a = 0xff);
			void			putPixel (const gos::Point2 &p, u8 r, u8 g, u8 b, u8 a = 0xff)				{ putPixel (p.x, p.y, r, g, b, a); }
			void			putPixel (i16 x, i16 y, const gos::ColorU32 &col)							{ putPixel (x, y, col.r(), col.g(), col.b(), col.a()); }
			void			putPixel (const gos::Point2 &p, const gos::ColorU32 &col)					{ putPixel (p.x, p.y, col.r(), col.g(), col.b(), col.a()); }

			const ColorU32	getPixel (i16 x, i16 y);

			void			line (i16 x1, i16 y1, i16 x2, i16 y2, u8 r, u8 g, u8 b, u8 a = 0xff);
			void			line (i16 x1, i16 y1, i16 x2, i16 y2, const gos::ColorU32 &col)					{ line (x1, y1, x2, y2, col.r(), col.g(), col.b(), col.a()); }
			
			void			rect (i16 x1, i16 y1, i16 dimx, i16 dimy, u8 r, u8 g, u8 b, u8 a = 0xff);
			void			rect (i16 x1, i16 y1, i16 dimx, i16 dimy, const gos::ColorU32 &col)				{ rect (x1, y1, dimx, dimy, col.r(), col.g(), col.b(), col.a()); }
			void			rectSoloQuadranteTopLeft (i16 xLeft, i16 yTop, i16 lato, u8 spessoreBordo, u8 red, u8 green, u8 blue);
							//filla un'area di dimensioni lato X lato il cui pixel in alto a sx � xLeft,yTop

			void			fillRect (i16 x1, i16 y1, i16 dimx, i16 dimy, u8 r, u8 g, u8 b, u8 a = 0xff);
			void			fillRect (i16 x1, i16 y1, i16 dimx, i16 dimy, const gos::ColorU32 &col)			{ fillRect (x1, y1, dimx, dimy, col.r(), col.g(), col.b(), col.a()); }

			void			circle (i16 cx, i16 cy, i16 r, u8 red, u8 green, u8 blue);
							//filla un'area di dimensioni r*2 X r*2 il cui centro � in cx,cy

			void			circleSoloQuadranteTopLeft (i16 xLeft, i16 yTop, i16 r, u8 red, u8 green, u8 blue);
							//filla un'area di dimensioni r X r il cui pixel in alto a sx � xLeft,yTop

			void			circonferenza (i16 cx, i16 cy, i16 r, u8 spessore, u8 red, u8 green, u8 blue);
							//filla un'area di dimensioni r*2 X r*2 il cui centro � in cx,cy con una circonferenza di spessore [spessore]

			void			circonferenzaSoloQuadranteTopLeft (i16 xLeft, i16 yTop, i16 r, u8 spessore, u8 red, u8 green, u8 blue);
							//filla un'area di dimensioni r X r il cui angolo in alto a sx � xLeft,yTop con una circonferenza di spessore [spessore]

			void			capsulaPiena (i16 xLeft, i16 yTop, i16 totalDimx, i16 totalDimy, i16 r, u8 red, u8 green, u8 blue);
							//filla un'area di dimensioni totalDimx X totalDimY il cui pixel in alto a sx � xLeft,yTop, con una capsula avente i lati tondeggianti di raggio [r]

			void			capsulaSoloBordata (i16 xLeft, i16 yTop, i16 totalDimx, i16 totalDimy, i16 r, u8 spessoreBordo, u8 red, u8 green, u8 blue);
							//filla un'area di dimensioni totalDimx X totalDimY il cui pixel in alto a sx � xLeft,yTop, con una capsula vuota avente i lati tondeggianti di raggio [r] e spessore del bordo = spessoreBordo

							//============================ query
			u8*				getBuffer() const																{ return _bufferRGBA; }
			u16				getW() const																	{ return _w; }
			u16				getH() const																	{ return _h; }


		private:
			static void		priv_do_putPixel (const BufferRGBA &dst, i16 x, i16 y, u8 r, u8 g, u8 b, u8 a);
			static void		priv_line_ori (const BufferRGBA &dst, i16 x1, i16 y1, i16 x2, u8 r, u8 g, u8 b, u8 a);
			static void		priv_line_ver (const BufferRGBA &dst, i16 x1, i16 y1, i16 y2, u8 r, u8 g, u8 b, u8 a);

		private:
			u32				priv_calcOffset (i16 x, i16 y) const											{ return ((x + (y * _w)) << 2); }
			u8*				priv_getPointerTo (i16 x, i16 y) const											{ return &_bufferRGBA[priv_calcOffset(x,y)]; }
			u8				priv_circonferenza_quadrante (i16 px, i16 py, i16 r, u8 spessore) const;
			u8				priv_circle_quadrante (i16 px, i16 py, i16 r) const;

		public:
			u8				*_bufferRGBA;
			u16				_w;
			u16				_h;
		};
	} //namespace image
 } //namespace gos

#endif //_gosImageBufferRGBA_h_