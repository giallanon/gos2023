#include "gosImageBufferRGBA.h"
#include "loader/stb_image.h"
#include "../gos/dataTypes/gosColorHDR.h"

using namespace gos;
using namespace gos::image;

//***********************************************************
bool BufferRGBA::loadFromFile (Allocator *allocator, const char* filePathAndName)
{
	u32 size = 0;
	u8 *buffer = fs::fileLoadInMemory (gos::getScrapAllocator(), filePathAndName, &size);
	if (NULL == buffer)
	{
		gos::logger::err ("image::BufferRGBA::loadFromFile() => file not found [%s]\n", filePathAndName);
		return false;
	}

	bool ret = loadFromFileInMemory (allocator, buffer, size);
	GOSFREE(mem::getScrapAllocator(), buffer);
	return ret;	
}

//***********************************************************
bool BufferRGBA::loadFromFileInMemory (Allocator *allocator, const void *fileData, u32 sizeOfData)
{
	assert (fileData && sizeOfData);

	int width, height, comp;
	u8 *rgba = stbi_load_from_memory ((const stbi_uc*)fileData, sizeOfData, &width, &height, &comp, 4);
	if (rgba)
	{
		BufferRGBA::alloc (allocator, static_cast<u16>(width), static_cast<u16>(height));
		memcpy (_bufferRGBA, rgba, width * height * 4);
		stbi_image_free (rgba);
		return true;
	}
	else
	{
		_bufferRGBA = NULL;
		_w = 0;
		_h = 0;
	}

	gos::logger::err ("image::BufferRGBA::loadFromFileInMemory() => unknown image format\n");
	return false;

}

//***********************************************************
bool BufferRGBA::alloc (Allocator *allocator, u16 width, u16 height)
{
	if (width == 0 || width > 10000 || height == 0 || height > 10000)
	{
		DBGBREAK;
		return false;
	}

	this->_w = width;
	this->_h = height;
	_bufferRGBA = GOSALLOCT (u8*, allocator, width * height * 4);
	return true;
}

//***********************************************************
void BufferRGBA::free (Allocator *allocator)
{
	if (NULL == _bufferRGBA)
		return;
	GOSFREE(allocator, _bufferRGBA);
	_bufferRGBA = NULL;
	_w = _h = 0;
}

//***********************************************************
void BufferRGBA::clear (u8 r, u8 g, u8 b, u8 a)
{
	u8	*p = _bufferRGBA;
	u32 size = _w * _h;
	while (size--)
	{
		*(p++) = r;
		*(p++) = g;
		*(p++) = b;
		*(p++) = a;
	}
}

//***********************************************************
void BufferRGBA::rect (i16 x1, i16 y1, i16 dimx, i16 dimy, u8 r, u8 g, u8 b, u8 a)
{
	const i16 x2 = x1 + dimx - 1;
	const i16 y2 = y1 + dimy - 1;

	line (x1, y1, x2, y1, r, g, b, a);
	line (x2, y1, x2, y2, r, g, b, a);
	line (x2, y2, x1, y2, r, g, b, a);
	line (x1, y2, x1, y1, r, g, b, a);
}

//***********************************************************
void BufferRGBA::fillRect (i16 xDST, i16 yDST, i16 dimx, i16 dimy, u8 r, u8 g, u8 b, u8 a)
{
	if (xDST >= this->getW() || yDST >= this->getH())
		return;
	if (dimx <= 0 || dimy <= 0)
		return;

	if (xDST < 0)
	{
		dimx -= (-xDST);
		xDST = 0;
	}
	if (yDST < 0)
	{
		dimy -= (-yDST);
		yDST = 0;
	}

	if (xDST + dimx > this->getW())
		dimx = this->getW() - xDST;
	if (dimx <= 0)
		return;

	if (yDST + dimy > this->getH())
		dimy = this->getH() - yDST;
	if (dimy <= 0)
		return;

	const u32 offsetToNextRow = (this->getW() - dimx) * 4;
	u8 *p = this->priv_getPointerTo(xDST, yDST);
	while (dimy--)
	{
		for (u16 x = 0; x < dimx; x++)
		{
			*(p++) = r;
			*(p++) = g;
			*(p++) = b;
			*(p++) = a;
		}

		p += offsetToNextRow;
	}
}

//***********************************************************
void BufferRGBA::rectSoloQuadranteTopLeft (i16 xLeft, i16 yTop, i16 lato, u8 spessoreBordo, u8 red, u8 green, u8 blue)
{
	const i16 x0 = xLeft;
	const i16 x1 = xLeft + (lato-1);
	//const i16 x2 = xLeft + (spessoreBordo-1);
	const i16 y0 = yTop + spessoreBordo;
	const i16 y1 = yTop + (lato-1);
	
	i16 x = x0;
	//i16 y = y0;
	for (u8 i = 0; i < spessoreBordo; i++)
	{
		line (x0, yTop, x1, yTop, red, green, blue);
		++yTop;

		line (x, y0, x, y1, red, green, blue);
		++x;
	}
}

//***********************************************************
u8 BufferRGBA::priv_circle_quadrante (i16 px, i16 py, i16 r) const
{
	const float d = sqrtf((f32)(px * px + py * py));
	if (d > r + 1)
		return 0;
	else if (d <= r)
		return 255;
	return (u8)roundf((r - d) * 255.0f);
}

//***********************************************************
void BufferRGBA::circle (i16 cx, i16 cy, i16 r, u8 red, u8 green, u8 blue)
{
	if (r <= 0)
		return;

	//calcolo solo il quadrante in alto a sx e faccio il mirror per il resto
	for (i16 y = -r; y < 0; y++)
	{
		for (i16 x = -r; x < 0; x++)
		{
			const u8 alfa = priv_circle_quadrante (x, y, r);
			putPixel (cx + x, cy + y, red, green, blue, alfa);
			putPixel (cx - x -1, cy + y, red, green, blue, alfa);
			putPixel (cx + x, cy - y -1, red, green, blue, alfa);
			putPixel (cx - x -1, cy - y -1, red, green, blue, alfa);
		}
	}
}

//***********************************************************
void BufferRGBA::circleSoloQuadranteTopLeft (i16 xLeft, i16 yTop, i16 r, u8 red, u8 green, u8 blue)
{
	if (r <= 0)
		return;

	const i16 cx = xLeft + r;
	const i16 cy = yTop + r;
	for (i16 y = -r; y < 0; y++)
	{
		for (i16 x = -r; x < 0; x++)
		{
			const u8 alfa = priv_circle_quadrante (x, y, r);
			putPixel (cx + x, cy + y, red, green, blue, alfa);
		}
	}
}

//***********************************************************
u8 BufferRGBA::priv_circonferenza_quadrante (i16 px, i16 py, i16 r, u8 spessore) const
{
	const float d = sqrtf((f32)(px * px + py * py));
	u8 alfa;

	const float aMax = (f32)(r + 1);
	const float aMid = (f32)(r - spessore + 1);
	const float aMin = (f32)(r - spessore) - 0.5f;
	if (d > aMax || d < aMin)
		alfa = 0;
	else
	{
		if (d > r)
			alfa = (u8)roundf((r - d) * 255.0f);
		else if (d > aMid)
			alfa = 255;
		else
		{
			const f32 t01 = ((d - aMin) / (aMid - aMin));
			//alfa = (u8)roundf(255.0f * BufferRGBA_Smoothstep(t01));
			alfa = (u8)roundf(255.0f * t01);
		}
	}

	return alfa;
}

//***********************************************************
void BufferRGBA::capsulaSoloBordata (i16 p1x, i16 p1y, i16 totalDimx, i16 totalDimy, i16 r, u8 spessoreBordo, u8 red, u8 green, u8 blue)
{
	if (spessoreBordo < 1)
		return;
	if (totalDimx < r + r)
	{
		DBGBREAK;
		return;
	}
	if (totalDimy < r + r)
	{
		DBGBREAK;
		return;
	}

	/*
				x0    x1      x2    x3

		y0		P1    P2------P3    P4

		y1		P5    P6      P7    P8

		y2		P9    P10-----P11   P12

		y3		P13   P14-----P15   P16
	*/
	const i16 x0 = p1x;
	const i16 x1 = x0 + (r);
	const i16 x3 = x0 + (totalDimx - 1);
	const i16 x2 = x3 - (r);

	const i16 y0 = p1y;
	const i16 y1 = y0 + (r);
	const i16 y3 = y0 + (totalDimy - 1);
	const i16 y2 = y3 - (r);
	
	
	for (i16 y = -r; y < 0; y++)
	{
		for (i16 x = -r; x < 0; x++)
		{
			const u8 alfa = priv_circonferenza_quadrante(x, y, r, spessoreBordo);
			putPixel (x1 + x, y1 + y, red, green, blue, alfa);
			putPixel (x2 - x, y1 + y, red, green, blue, alfa);

			putPixel (x1 + x, y2 - y, red, green, blue, alfa);
			putPixel (x2 - x, y2 - y, red, green, blue, alfa);
		}
	}

	for (u8 i = 0; i < spessoreBordo; i++)
	{
		line (x1, y0+i, x2, y0+i, red, green, blue);
		line (x1, y3-i, x2, y3-i, red, green, blue);

		line (x0+i, y1, x0+i, y2, red, green, blue);
		line (x3-i, y1, x3-i, y2, red, green, blue);
	}
}

//***********************************************************
void BufferRGBA::capsulaPiena (i16 p1x, i16 p1y, i16 totalDimx, i16 totalDimy, i16 r, u8 red, u8 green, u8 blue)
{
	if (totalDimx < r + r)
	{
		DBGBREAK;
		return;
	}
	if (totalDimy < r + r)
	{
		DBGBREAK;
		return;
	}

	/*
				x0    x1      x2    x3

		y0		P1    P2------P3    P4

		y1		P5    P6      P7    P8

		y2		P9    P10-----P11   P12

		y3		P13   P14-----P15   P16
	*/
	const i16 x0 = p1x;
	const i16 x1 = x0 + (r);
	const i16 x3 = x0 + (totalDimx - 1);
	const i16 x2 = x3 - (r);

	const i16 y0 = p1y;
	const i16 y1 = y0 + (r);
	const i16 y3 = y0 + (totalDimy - 1);
	const i16 y2 = y3 - (r);
	
	
	for (i16 y = -r; y < 0; y++)
	{
		for (i16 x = -r; x < 0; x++)
		{
			const u8 alfa = priv_circle_quadrante (x, y, r);
			putPixel (x1 + x, y1 + y, red, green, blue, alfa);
			putPixel (x2 - x, y1 + y, red, green, blue, alfa);

			putPixel (x1 + x, y2 - y, red, green, blue, alfa);
			putPixel (x2 - x, y2 - y, red, green, blue, alfa);
		}
	}

	fillRect (x1, y0, x2-x1+1, y3-y0+1, red, green, blue);
	fillRect (x0, y1, x1-x0+1, y2-y1+1, red, green, blue);
	fillRect (x2, y1, x3-x2+1, y2-y1+1, red, green, blue);

	
}

//***********************************************************
void BufferRGBA::circonferenza (i16 cx, i16 cy, i16 r, u8 spessore, u8 red, u8 green, u8 blue)
{
	if (spessore < 1)
		return;
	for (i16 y = -r; y < 0; y++)
	{
		for (i16 x = -r; x < 0; x++)
		{
			const u8 alfa = priv_circonferenza_quadrante(x, y, r, spessore);
			putPixel (cx + x, cy + y, red, green, blue, alfa);
			putPixel (cx - x - 1, cy + y, red, green, blue, alfa);
			putPixel (cx + x, cy - y - 1, red, green, blue, alfa);
			putPixel (cx - x - 1, cy - y - 1, red, green, blue, alfa);
		}
	}
}

//***********************************************************
void BufferRGBA::circonferenzaSoloQuadranteTopLeft (i16 xLeft, i16 yTop, i16 r, u8 spessore, u8 red, u8 green, u8 blue)
{
	if (spessore < 1)
		return;
	const i16 cx = xLeft + r;
	const i16 cy = yTop + r;

	for (i16 y = -r; y < 0; y++)
	{
		for (i16 x = -r; x < 0; x++)
		{
			const u8 alfa = priv_circonferenza_quadrante(x, y, r, spessore);
			putPixel (cx + x, cy + y, red, green, blue, alfa);
		}
	}
}

//***********************************************************
void BufferRGBA::priv_do_putPixel (const BufferRGBA &dst, i16 x, i16 y, u8 r, u8 g, u8 b, u8 a)
{
	assert (x >= 0 && x < dst.getW());
	assert (y >= 0 && y < dst.getH());
	
	u8 *p = dst.getBuffer();
	p += 4 * (y * dst.getW() + x);
	*(p++) = r;
	*(p++) = g;
	*(p++) = b;
	*(p) = a;
}

//***********************************************************
void BufferRGBA::priv_line_ori (const BufferRGBA &dst, i16 x1, i16 y1, i16 x2, u8 r, u8 g, u8 b, u8 a)
{
	if (y1 < 0 || y1 >= dst.getH())
		return;
	if (x1 > x2)
	{
		i16 s = x1;
		x1 = x2;
		x2 = s;
	}
	if (x2 < 0)
		return;
	if (x1 >= dst.getW())
		return;
	if (x1 < 0)
		x1 = 0;
	if (x2 >= dst.getW())
		x2 = dst.getW() -1;

	u8 *p = dst.priv_getPointerTo(x1, y1);
	while (x1++ <= x2)
	{
		*(p++) = r;
		*(p++) = g;
		*(p++) = b;
		*(p++) = a;
	}
}

//***********************************************************
void BufferRGBA::priv_line_ver (const BufferRGBA &dst, i16 x1, i16 y1, i16 y2, u8 r, u8 g, u8 b, u8 a)
{
	if (x1 < 0 || x1 >= dst.getW())
		return;
	if (y1 > y2)
	{
		i16 s = y1;
		y1 = y2;
		y2 = s;
	}
	if (y2 < 0)
		return;
	if (y1 >= dst.getH())
		return;
	if (y1 < 0)
		y1 = 0;
	if (y2 >= dst.getH())
		y2 = dst.getH() -1;

	const u32 incr = (dst.getW() * 4) - 3;
	u8 *p = dst.priv_getPointerTo (x1, y1);
	while (y1++ <= y2)
	{
		*(p++) = r;
		*(p++) = g;
		*(p++) = b;
		*(p) = a;
		p += incr;
	}
}

//***********************************************************
void BufferRGBA::putPixel (i16 x, i16 y, u8 r, u8 g, u8 b, u8 a)
{
	if (x < 0 || x >= getW() || y < 0 || y >= getH())
		return;
	BufferRGBA::priv_do_putPixel (*this, x, y, r, g, b, a);
}

//***********************************************************
const gos::ColorU32	BufferRGBA::getPixel (i16 x, i16 y)
{
	if (x < 0 || x >= getW() || y < 0 || y >= getH())
		return gos::ColorU32(0,0,0,0);
	
	const u8 *p = priv_getPointerTo(x, y);
	return gos::ColorU32 (p[3], p[0], p[1], p[2]);
}

//***********************************************************
void BufferRGBA::line (i16 x1, i16 y1, i16 x2, i16 y2, u8 r, u8 g, u8 b, u8 a)
{
	if (y1 == y2)
		BufferRGBA::priv_line_ori (*this, x1, y1, x2, r, g, b, a);
	else if (x1 == x2)
		BufferRGBA::priv_line_ver (*this, x1, y1, y2, r, g, b, a);
	else
	{
		const i16 dx =  abs(x2 - x1);
		const i16 sx = (x1 < x2) ? 1 : -1;
		const i16 dy = -abs(y2-y1);
		const i16 sy = (y1 < y2) ? 1 : -1;
    
		i16 err = dx+dy;
		while (1)
		{
			if (x1 >=0 && x1 < getW() && y1>=0 && y1< getH())
				BufferRGBA::priv_do_putPixel (*this, x1, y1, r, g, b, a);

			if (x1 == x2 && y1 == y2)
				break;
			i16 e2 = 2 * err;
			if (e2 >= dy)
			{
				err += dy;
				x1 += sx;
			}

			if (e2 <= dx)
			{
				err += dx;
				y1 += sy;
			}
		}
	}
}

//***********************************************************
void BufferRGBA::blt (BufferRGBA &dst, i16 xDST, i16 yDST, const BufferRGBA &src, u16 x1, u16 y1, i16 dimx, i16 dimy, eAlphaOP alphaOP)
{
	if (xDST >= dst.getW() || yDST >= dst.getH())
		return;
	if (dimx <= 0 || dimy <= 0)
		return;

	assert (x1 >= 0 && y1 >= 0);
	assert (x1 + dimx <= src.getW());
	assert (y1 + dimy <= src.getH());

	if (xDST < 0)
	{
		x1 += (-xDST);
		dimx -= (-xDST);
		xDST = 0;
	}
	if (yDST < 0)
	{
		y1 += (-yDST);
		dimy -= (-yDST);
		yDST = 0;
	}

	if (x1 + dimx > dst.getW())
		dimx = dst.getW() - x1;
	if (dimx <= 0)
		return;

	if (y1 + dimy > dst.getH())
		dimy = dst.getH() - y1;
	if (dimy <= 0)
		return;
	

	const u8 *pSrc = src.priv_getPointerTo(x1, y1);
	u8 *pDst = dst.priv_getPointerTo (xDST, yDST);
	switch (alphaOP)
	{
	case eAlphaOP::copy:
		{
			const u32 n = dimx * 4;
			const u32 offsetToNextRowSRC = src.getW() * 4;
			const u32 offsetToNextRowDST = dst.getW() * 4;
			for (u16 y = 0; y < dimy; y++)
			{
				memcpy (pDst, pSrc, n);
				pSrc += offsetToNextRowSRC;
				pDst += offsetToNextRowDST;
			}
		}
		break;

	case eAlphaOP::copyOnlyIfAlphaIsNotZero:
		{
			const u32 offsetToNextRowSRC = (src.getW() - dimx) * 4;
			const u32 offsetToNextRowDST = (dst.getW() - dimx) * 4;

			for (u16 y = 0; y < dimy; y++)
			{
				for (u16 x = 0; x < dimx; x++)
				{
					//if (pSrc[3] != 0)
					memcpy (pDst, pSrc, 4);
					pDst += 4;
					pSrc += 4;
				}

				pSrc += offsetToNextRowSRC;
				pDst += offsetToNextRowDST;
			}
		}
		break;
	}
}

//***********************************************************
void BufferRGBA::bltRChannelOnly (BufferRGBA &dst, i16 xDST, i16 yDST, const BufferRGBA &src, u16 x1, u16 y1, i16 dimx, i16 dimy, u8 r, u8 g, u8 b)
{
	if (xDST >= dst.getW() || yDST >= dst.getH())
		return;
	if (dimx <= 0 || dimy <= 0)
		return;

	assert (x1 >= 0 && y1 >= 0);
	assert (x1 + dimx <= src.getW());
	assert (y1 + dimy <= src.getH());

	if (xDST < 0)
	{
		x1 += (-xDST);
		dimx -= (-xDST);
		xDST = 0;
	}
	if (yDST < 0)
	{
		y1 += (-yDST);
		dimy -= (-yDST);
		yDST = 0;
	}

	if (x1 + dimx > dst.getW())
		dimx = dst.getW() - x1;
	if (dimx <= 0)
		return;

	if (y1 + dimy > dst.getH())
		dimy = dst.getH() - y1;
	if (dimy <= 0)
		return;
	

	const u32 offsetToNextRowSRC = (src.getW() - dimx) * 4;
	const u32 offsetToNextRowDST = (dst.getW() - dimx) * 4;
	const u8 *pSrc = src.priv_getPointerTo(x1, y1);
	u8 *pDst = dst.priv_getPointerTo (xDST, yDST);
	for (u16 y=0; y<dimy; y++)
	{
		for (u16 x = 0; x < dimx; x++)
		{
			pDst[0] = gos::ColorU32::applyAlpha(r, pSrc[0]);
			pDst[1] = gos::ColorU32::applyAlpha(g, pSrc[0]);
			pDst[2] = gos::ColorU32::applyAlpha(b, pSrc[0]);
			pDst[3] = 255;
			pDst += 4;
			pSrc += 4;
		}

		pSrc += offsetToNextRowSRC;
		pDst += offsetToNextRowDST;
	}
}
//***********************************************************
void BufferRGBA::bltMask (BufferRGBA &dst, i16 xDST, i16 yDST, const BufferA &src, u16 x1, u16 y1, i16 dimx, i16 dimy, u8 r, u8 g, u8 b)
{
	BufferRGBA::bltMask (dst, xDST, yDST, src._bufferA, src._w, src._h, x1, y1, dimx, dimy, r, g, b);
}

//***********************************************************
void BufferRGBA::bltMask (BufferRGBA &dst, i16 xDST, i16 yDST, const u8 *srcBufferA, u16 srcBufferW, u16 srcBufferH, u16 x1, u16 y1, i16 dimx, i16 dimy, u8 r, u8 g, u8 b)
{
	if (xDST >= dst.getW() || yDST >= dst.getH())
		return;
	if (dimx <= 0 || dimy <= 0)
		return;

	assert (x1 >= 0 && y1 >= 0);
	assert (x1 + dimx <= srcBufferW);
	assert (y1 + dimy <= srcBufferH);

	if (xDST < 0)
	{
		x1 += (-xDST);
		dimx -= (-xDST);
		xDST = 0;
	}
	if (yDST < 0)
	{
		y1 += (-yDST);
		dimy -= (-yDST);
		yDST = 0;
	}

	if (x1 + dimx > dst.getW())
		dimx = dst.getW() - x1;
	if (dimx <= 0)
		return;

	if (y1 + dimy > dst.getH())
		dimy = dst.getH() - y1;
	if (dimy <= 0)
		return;
	

	const u32 offsetToNextRowSRC = (srcBufferW - dimx);
	const u32 offsetToNextRowDST = (dst.getW() - dimx) * 4;
	const u8 *pSrc = &srcBufferA[x1 + y1 * srcBufferW];
	u8 *pDst = dst.priv_getPointerTo (xDST, yDST);
	for (u16 y=0; y<dimy; y++)
	{
		for (u16 x = 0; x < dimx; x++)
		{
			/*if (pSrc[0] != 0)
			{
				pDst[0] = gos::ColorU32::applyAlpha(r, pSrc[0]);
				pDst[1] = gos::ColorU32::applyAlpha(g, pSrc[0]);
				pDst[2] = gos::ColorU32::applyAlpha(b, pSrc[0]);
				pDst[3] = 255;
			}*/
			pDst[0] = r; pDst[1] = g; pDst[2] = b; pDst[3] = pSrc[0];

			pDst += 4;
			pSrc++;
		}

		pSrc += offsetToNextRowSRC;
		pDst += offsetToNextRowDST;
	}
}

//***********************************************************
bool BufferRGBA::saveAsTGA (const char *filename) const
{
	gos::File f;
	if (!fs::fileOpenForW (&f, filename))
		return false;

	//preparo il tga
	const u16 dx = getW();
	const u16 dy = getH();
	const u32 imgSizeInByte = (u32)dx * dy * 4;

	u8 *tga = GOSALLOC_SCRAPT(u8*, 18 + imgSizeInByte);
	
	//header
	memset (tga, 0, 18);
	tga[2]	= 2;	/* image type = uncompressed RGB */
	tga[12] = (char) (dx & 0x00FF);
	tga[13] = (char) ((dx & 0xFF00) >> 8);
	tga[14] = (char) (dy & 0x00FF);
	tga[15] = (char) ((dy & 0xFF00) >> 8);
	tga[16] = 32;
	tga[17] = 0x20;	/* Top-down, non-interlaced */

	u32 ctSRC = 0;
	u32 ctDST = 18;
	while (ctSRC < imgSizeInByte)
	{
		u8  r = _bufferRGBA[ctSRC++];
		u8  g = _bufferRGBA[ctSRC++];
		u8  b = _bufferRGBA[ctSRC++];
		u8  a = _bufferRGBA[ctSRC++];

		tga[ctDST++] = b;
		tga[ctDST++] = g;
		tga[ctDST++] = r;
		tga[ctDST++] = a;
	}
	
	fs::fileWrite (f, tga, 18 + imgSizeInByte);
	fs::fileClose(f);

	GOSFREE_SCRAP(tga);
	return true;
}

//***********************************************************
void BufferRGBA::convert_sRGB_to_RGB()
{
	const u32 size = _w * _h * 4;
	u32 ct = 0;
	while (ct < size)
	{
		u32 startCT = ct;
		const u8 r = _bufferRGBA[ct++];
		const u8 g = _bufferRGBA[ct++];
		const u8 b = _bufferRGBA[ct++];
		const u8 a = _bufferRGBA[ct++];

		gos::ColorHDR col;
		col.setU8_argb(a, r, g, b);
		col.sRGBToLinear();

		const u32 argb = col.toU32ARGB();

		_bufferRGBA[startCT++] = (u8)((argb & 0x00FF0000) >> 16);
		_bufferRGBA[startCT++] = (u8)((argb & 0x0000FF00) >> 8);
		_bufferRGBA[startCT++] = (u8)((argb & 0x000000FF));
		_bufferRGBA[startCT] = (u8)((argb & 0xFF000000) >> 24);

	}
}