#ifndef _gosBufferLinear_h_
#define _gosBufferLinear_h_
#include "memory/gosMemory.h"

namespace gos
{
	/******************************************************************************
	 * BufferLinear
	 *
	 * Gestisce un blocco di memoria, che viene mantenuto contiguo anche a seguito di eventuale crescita del buffer.
	 * Nel momento in cui si volesse far crescere il buffer, BufferLinear si appoggia ad un Allocator per riservare nuova
	 * memoria. Il buffer precedente viene memcpy nel buffer nuovo.
	 * In questo modo il blocco di memoria e' sempre contiguo, anche a seguito di espansione
	 */
	class BufferLinear
	{
	public:
						BufferLinear() : allocator(NULL), mem(NULL), allocatedSize(0), bFreeMemBlock(0) { }
                        ~BufferLinear()			{ priv_FreeCurBuffer (); }

		void			setupWithBase (void *startingBlock, u32 sizeOfStartingBlock, Allocator *backingallocator);
							/*	utilizza startingBlock come blocco di memoria iniziale. Non ne fa il free.
								Se il buffer dovesse espandersi, allora nuova memoria viene allocata tramite il backingallocator
								e il contenuto di startingBlock viene memcpiato nel nuovo buffer.
								In ogni caso, startingBlock non viene mai freed, anche perche' non e' possibile sapere da chi e' stato allocato
							*/

		void			setup (Allocator *backingallocator, u32 preallocNumBytes=0);
							/*	alloca un blocco iniziale di dimensione=preallocNumBytes usando il backingallocator.
								Se il buffer dovessere crescere, nuova memoria viene allocata tramite il backingAllocator e il contenuto
								del precedente buffer viene memcpiato nel nuovo.
								Il buffer precedente viene poi freed()
							*/

		void			unsetup ()																{ priv_FreeCurBuffer(); mem = NULL; allocatedSize = 0; bFreeMemBlock = 0; allocator = NULL; }

		bool			copyFrom (const BufferLinear &src, u32 srcOffset, u32 nBytesToCopy, u32 dstOffset, bool bCangrow=true);
							/*  copia [nBytesToCopy] bytes di [src] a partire da [srcOffset] e li mette in this a partire da [dstOffset].
							    Valgono le stesse considerazioni di read/write relativamente al fallimento della funzione
							*/

		void			shrink (u32 newSize, Allocator *newAllocator = NULL);
							/*	riduce la dimensione del buffer allocato portandola a newSize e memcpyando tutti i dati fino a newSize.
								se viene indicato un newAllocator, allora si usa quello, altrimenti si usa il solito allocator
							*/

		u8*				_getPointer (u32 pos) const																				{ assert(pos<allocatedSize); return &mem[pos]; }

		void			zero() 																									{ memset(mem, 0, allocatedSize); }
		bool			read  (void *dest, u32 offset, u32 nBytesToread) const;
		bool			write (const void *src, u32 offset, u32 nBytesTowrite, bool bCangrow=true);
		bool			growIncremental (u32 howManyBytesToAdd);
		bool			growUpTo (u32 finalSize);
		u32				getTotalSizeAllocated() const																			{ return allocatedSize; }

		Allocator*	    getAllocator() const																					{ return allocator; }
			
	private:
		void			priv_FreeCurBuffer ();

	private:
		Allocator	    *allocator;
		u8				*mem;
		u32				allocatedSize;
		u8				bFreeMemBlock;
	};
} //namespace gos
#endif //_gosBufferLinear_h_