#ifndef _gosImageBufferA_h_
#define _gosImageBufferA_h_
#include "../gos/gos.h"


namespace gos
{ 
	namespace image
	{
		class BufferRGBA; //fwd

		/***********************************************
		 * BufferA
		 *
		 * sprite/immagine nel formato 8 bit monocanale
		 */
		class BufferA
		{
		public:
							//============================ static
			static void		blt (BufferA &dst, i16 xDST, i16 yDST, const BufferA &src, u16 x1, u16 y1, i16 dimx, i16 dimy);

		public:
							//============================ fn
							BufferA()																	{ _bufferA = NULL; _w = _h = 0; }
							~BufferA()																	{ }


			bool			alloc (Allocator *allocator, u16 w, u16 h);
			void			free  (Allocator *allocator);

			void			clear (u8 col);

			void			putPixel (i16 x, i16 y, u8 col);

			u8				getPixel (i16 x, i16 y);

			void			line (i16 x1, i16 y1, i16 x2, i16 y2, u8 col);

			void			fillRect (i16 x1, i16 y1, i16 dimx, i16 dimy, u8 col);

			//============================ query
			u8*				getBuffer() const																{ return _bufferA; }
			u16				getW() const																	{ return _w; }
			u16				getH() const																	{ return _h; }


		private:
			static void		priv_do_putPixel (const BufferA &dst, i16 x, i16 y, u8 col);
			static void		priv_line_ori (const BufferA &dst, i16 x1, i16 y1, i16 x2, u8 col);
			static void		priv_line_ver (const BufferA &dst, i16 x1, i16 y1, i16 y2, u8 col);

		private:
			u32				priv_calcOffset (i16 x, i16 y) const											{ return ((x + (y * _w))); }
			u8*				priv_getPointerTo (i16 x, i16 y) const											{ return &_bufferA[priv_calcOffset(x,y)]; }

		public:
			u8				*_bufferA;
			u16				_w;
			u16				_h;

			friend class BufferRGBA;
		};
	} //namespace image
 } //namespace gos

#endif //_gosImageBufferA_h_