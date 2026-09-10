#include "gosImageBuffer.h"

using namespace gos;


//**********************************************************
bool image::blt (const image::BufferDescr &srcDesc, const void *_pSRC, u32 srcX, u32 srcY, u32 srcDimX, u32 srcDimY, const image::BufferDescr &dstDesc, void *_pDST, i32 dstX, i32 dstY)
{
	if (srcDesc.sizeof_one_elem != dstDesc.sizeof_one_elem)
	{
		DBGBREAK;
		return false;
	}

	assert (srcX < srcDesc.num_col);
	assert (srcY < srcDesc.num_row);
	assert (srcX + srcDimX <= srcDesc.num_col);
	assert (srcY + srcDimY <= srcDesc.num_row);


	u32 x1, dimx;
	if (dstX >= 0)	{ x1 = dstX; dimx = srcDimX; }
	else			{ x1 = 0; srcX += (u32)(-dstX); dimx = srcDimX - (u32)(-dstX); }
	if (x1 >= dstDesc.num_col)			{ DBGBREAK; return false; }
	if (x1 + dimx > dstDesc.num_col)	{ dimx = dstDesc.num_col - x1; }
	if (dimx == 0)						{ DBGBREAK; return false; }
	assert (x1 < dstDesc.num_col);
	assert (x1 + dimx <=  dstDesc.num_col);

	u32 y1, dimy;
	if (dstY >= 0)	{ y1 = dstY; dimy = srcDimY; }
	else			{ y1 = 0; srcY += (u32)(-dstY); dimy = srcDimY - (u32)(-dstY); }
	if (y1 >= dstDesc.num_row)			{ DBGBREAK; return false; }
	if (y1 + dimy > dstDesc.num_row)	{ dimy = dstDesc.num_row - y1; }
	if (dimy == 0)						{ DBGBREAK; return false; }
	assert (y1 < dstDesc.num_row);
	assert (y1 + dimy <=  dstDesc.num_row);

	//x1, y1, dimx, dimy rappresentano il quad di destinazione
	//srcX, srcY, dimx, dimy rappresentano il quad sorgente
	assert (srcX < srcDesc.num_col);
	assert (srcX + dimx <=  srcDesc.num_col);
	assert (srcY < srcDesc.num_row);
	assert (srcY + dimy <=  srcDesc.num_row);

	u32 srcCT = srcX * srcDesc.sizeof_one_elem + srcY * srcDesc.sizeof_one_row;
	u32 dstCT = x1 * dstDesc.sizeof_one_elem + y1 * dstDesc.sizeof_one_row;
	const u32 to_copy = dstDesc.sizeof_one_elem * dimx;
	const u8 *pSRC = static_cast<const u8*>(_pSRC);
	u8 *pDST = static_cast<u8*>(_pDST);
	for (u32 y = 0; y < dimy; y++)
	{
		memcpy (&pDST[dstCT], &pSRC[srcCT], to_copy);
		dstCT += dstDesc.sizeof_one_row;
		srcCT += srcDesc.sizeof_one_row;
	}

	return true;
}