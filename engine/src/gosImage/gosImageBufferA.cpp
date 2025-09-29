#include "gosImageBufferA.h"


using namespace gos;
using namespace gos::image;

//***********************************************************
bool BufferA::alloc (Allocator *allocator, u16 width, u16 height)
{
	if (width == 0 || width > 10000 || height == 0 || height > 10000)
	{
		DBGBREAK;
		return false;
	}

	this->_w = width;
	this->_h = height;
	_bufferA = GOSALLOCT (u8*, allocator, width * height);
	return true;
}

//***********************************************************
void BufferA::free (Allocator *allocator)
{
	if (NULL == _bufferA)
		return;
	GOSFREE(allocator, _bufferA);
	_bufferA = NULL;
	_w = _h = 0;
}

//***********************************************************
void BufferA::clear (u8 col)
{
	memset (_bufferA, col, getW() * getH());
}

//***********************************************************
void BufferA::putPixel (i16 x, i16 y, u8 col)
{
	if (x < 0 || x >= getW() || y < 0 || y >= getH())
		return;
	BufferA::priv_do_putPixel (*this, x, y, col);
}

//***********************************************************
void BufferA::priv_do_putPixel (const BufferA &dst, i16 x, i16 y, u8 col)
{
	assert (x >= 0 && x < dst.getW());
	assert (y >= 0 && y < dst.getH());
	dst._bufferA[dst.priv_calcOffset(x, y)] = col;
}

//***********************************************************
void BufferA::priv_line_ori (const BufferA &dst, i16 x1, i16 y1, i16 x2, u8 col)
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

	//u8 *p = dst.priv_getPointerTo(x1, y1);
	memset (dst._bufferA, col, (x2 - x1) + 1);
}

//***********************************************************
void BufferA::priv_line_ver (const BufferA &dst, i16 x1, i16 y1, i16 y2, u8 col)
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

	u8 *p = dst.priv_getPointerTo (x1, y1);
	while (y1++ <= y2)
	{
		(*p) = col;
		p += dst.getW();
	}
}

//***********************************************************
u8 BufferA::getPixel (i16 x, i16 y)
{
	if (x < 0 || x >= getW() || y < 0 || y >= getH())
		return 0;
	const u8 *p = priv_getPointerTo(x, y);
	return p[0];
}

//***********************************************************
void BufferA::line (i16 x1, i16 y1, i16 x2, i16 y2, u8 col)
{
	if (y1 == y2)
		BufferA::priv_line_ori (*this, x1, y1, x2, col);
	else if (x1 == x2)
		BufferA::priv_line_ver (*this, x1, y1, y2, col);
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
				BufferA::priv_do_putPixel (*this, x1, y1, col);

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
void BufferA::fillRect (i16 xDST, i16 yDST, i16 dimx, i16 dimy, u8 col)
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

	while (dimy--)
	{
		u8 *p = this->priv_getPointerTo(xDST, yDST);
		memset (p, col, dimx);
		yDST++;
	}
}

//***********************************************************
void BufferA::blt (BufferA &dst, i16 xDST, i16 yDST, const BufferA &src, u16 x1, u16 y1, i16 dimx, i16 dimy)
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
	
	for (u16 y=0; y<dimy; y++)
	{
		const u8 *pSrc = src.priv_getPointerTo(x1, y1);
		u8 *pDst = dst.priv_getPointerTo (xDST, yDST);
		memcpy (pDst, pSrc, dimx);
		yDST++;
		y1++;
	}
}

