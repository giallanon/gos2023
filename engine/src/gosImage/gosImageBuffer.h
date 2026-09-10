#ifndef _gosImageBuffer_h_
#define _gosImageBuffer_h_
#include "../gos/gos.h"


namespace gos
{
	namespace image
	{
		/**************************************
		* @brief	BufferDescr
		*			Rappresenta un buffer composto da <num_row> righe per <num_col> colonne.
		*			Ogni singolo elemento e' grosso <sizeof_one_elem>
		*			Ogni singola riga e' grossa <sizeof_one_row> dove <sizeof_one_row> NON e' necessariamente uguale
		*			a <sizeof_one_elem> * <num_col>
		*/
		class BufferDescr
		{
		public:
			u32		sizeof_one_elem;
			u32		sizeof_one_row;
			u32		num_row;
			u32		num_col;

		public:
			void	setup (u32 rows, u32 cols, u32 sizeof_one_elemIN)									{ setup (rows, cols, sizeof_one_elemIN, sizeof_one_elemIN * cols); }
			void	setup (u32 rows, u32 cols, u32 sizeof_one_elemIN, u32 sizeof_one_rowIN)				{ num_row = rows; num_col = cols; sizeof_one_elem = sizeof_one_elemIN; sizeof_one_row = sizeof_one_rowIN; }
		};


		/*
		* @brief	blt
		*			Copia il rect definito da <srcX1, srcY1, scrDimx, srcDimy> in <dst> a partire da xDST, y DST.
		*			Se <src> e' piu' grosso di <dst>, copia solo quello che effettivamente ci sta in <dst>.
		*			Le coordinate di dst possono essere negative e la fn se ne prende cura.
		*			E' mandatorio che src.sizeof_one_elem == dst.sizeof_one_elem*/
		bool	blt (const BufferDescr &srcDesc, const void *pSRC, u32 srcX, u32 srcY, u32 srcDimX, u32 srcDimY, const BufferDescr &dstDesc, void *pDST, i32 dstX, i32 dstY);



	} //namespace image
} //namespace gos

#endif //_gosImageBuffer_h_