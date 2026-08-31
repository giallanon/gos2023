#include "Array2DUtils.h"

using namespace gos;


//*******************************************
bool gos::array2DUtils_is_valid_coord (const Array2D &a, u32 x, u32 y, u32 dimx, u32 dimy)
{
	if (x >= a.num_col) return false;
	if (y >= a.num_row) return false;

	if (0 == dimx || 0 == dimy) return false;
	if (x + dimx > a.num_col) return false;
	if (y + dimy > a.num_row) return false;

	return true;
}

//*******************************************
bool gos::array2DUtils_copy (const void *psrc, const Array2D &src, u32 srcX, u32 srcY, u32 dimx, u32 dimy, void *pdst, const Array2D &dst, u32 dstX, u32 dstY)
{
	assert (NULL != psrc);
	assert (NULL != pdst);
	assert (src.sizeof_elem == dst.sizeof_elem);
	if (!array2DUtils_is_valid_coord(src, srcX, srcY, dimx, dimy))	{ DBGBREAK; return false; }


	u32 dst_avail_dimx = (dst.num_col - dstX);
	if (dst_avail_dimx > dst.num_col) { DBGBREAK; return false; }
	if (dimx > dst_avail_dimx)	dimx = dst_avail_dimx;

	u32 dst_avail_dimy = (dst.num_row - dstY);
	if (dst_avail_dimy > dst.num_row) { DBGBREAK; return false; }
	if (dimy > dst_avail_dimy)	dimy = dst_avail_dimy;

	const u8 *pSRC = reinterpret_cast<const u8*>( psrc );
	u8 *pDST = reinterpret_cast<u8*>( pdst );
	const u32 src_stride = src.sizeof_elem * src.num_col;
	const u32 dst_stride = dst.sizeof_elem * dst.num_col;
	const u32 to_copy_per_row = src.sizeof_elem * dimx;

	u32 src_ct = srcX * src.sizeof_elem + srcY * src_stride;
	u32 dst_ct = dstX * dst.sizeof_elem + dstY * dst_stride;
	for (u32 y=0; y<dimy; y++)
	{
		memcpy (&pDST[dst_ct], &pSRC[src_ct], to_copy_per_row);
		src_ct += src_stride;
		dst_ct += dst_stride;
	}

	return true;
}