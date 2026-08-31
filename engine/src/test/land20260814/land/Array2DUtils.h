#ifndef _Array2DUtils_h_
#define _Array2DUtils_h_
#include "gos.h"

namespace gos
{
	struct Array2D
	{
		u32 		sizeof_elem;		//un singolo elemento e' grosso <sizeof_elem> byte
		u32			num_row;			//numero di righe
		u32			num_col;

		void 	set (u32 row, u32 col, u32 sizeof_elemIN)	{ num_row=row; num_col=col; sizeof_elem=sizeof_elemIN; }
	};

	bool 	array2DUtils_is_valid_coord (const Array2D &a, u32 x, u32 y, u32 dimx, u32 dimy);
	bool 	array2DUtils_copy (const void *psrc, const Array2D &src, u32 srcX, u32 srcY, u32 dimx, u32 dimy, void *pdst, const Array2D &dst, u32 dstX, u32 dstY);
} //namespace gos

#endif //_Array2DUtils_h_