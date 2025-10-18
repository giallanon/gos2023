#ifndef _gosBufferSparse_h_
#define _gosBufferSparse_h_
#include "memory/gosMemory.h"

namespace gos
{
	/**
 	* @brief BufferSparse
 	* 
	* Fornisce i metodi per il read/write di un blocco di memoria che, internalmente, viene gestito come una serie
	* di blocchi di memoria non contigui
	* Nel momento in cui si volesse far crescere il buffer, BufferSparse si appoggia ad un Allocator per riservare nuova
	* memoria. 
	* La dimensione della nuova memoria allocata, e' di almeno [minSizeOfBlocksWhengrowing] bytes
	*/
	class BufferSparse
	{
	public:
						BufferSparse();
						~BufferSparse();

		void			setupWithBase (void *startingBlock, u32 sizeOfStartingBlock, Allocator *backingallocator, u32 minSizeOfBlocksWhengrowing);
							/*	utilizza startingBlock come blocco di memoria iniziale. Non ne fa il free.
								Se il buffer dovesse espandersi, allora un nuovo blocco di memoria viene allocata tramite il backingallocator
								In ogni caso, startingBlock non viene mai freed, anche perche' non e' possibile sapere da chi e' stato allocato
								Tutti i buffer allocati internamente invece, vengono freed() alla fine
							*/

		void			setup (Allocator *backingallocator, u32 preallocNumBytes, u32 minSizeOfBlocksWhengrowing);
							/*	alloca un blocco iniziale di dimensione=preallocNumBytes usando il backingallocator.
								Se il buffer dovesse espandersi, allora un nuovo blocco di memoria viene allocata tramite il backingallocator
								Tutti i buffer allocati internamente vengono freed() alla fine
							*/
			
		void			unsetup();


		void			zero();
		bool			read  (void *dest, u32 offset, u32 nBytesToread) const;
		bool			write (const void *src, u32 offset, u32 nBytesTowrite, bool bCangrow=true);
		bool			growIncremental (u32 howManyBytesToAdd);
		bool			growUpTo (u32 finalSize);

		bool 			append (const void *src, u32 *in_out_offset, u32 nBytesTowrite, bool bCangrow=true);
		bool 			appendStr (const char *s, u32 *in_out_offset, bool bCangrow=true);

		u32				getTotalSizeAllocated() const																			{ return totalAvailableBufferSpace; }
		Allocator*	    getAllocator() const																					{ return allocator; }

		void*			getBufferAtOffset (u32 offset, u32 nBytesRequested) const;
							/* ritorna il pt al buffer a partire da offset. Si assicura che offset + nBytesRequested stia tutto all'interno di un singolo blocco
								di memoria.
								Ritorna NULL in caso di errore
							*/


	private:
		struct sBuffer
		{
			sBuffer		*next;		//prossimo buffer
			u32			sizeInByte;	//size disponibile per la memorizzazione

			u8*			getMemPointer(u32 offset) const																			{ assert(offset < sizeInByte); return &((u8*)this)[sizeof(sBuffer) + offset]; }
		};

	private:
		sBuffer*		priv_setupBufferFromMem (void *mem, u32 sizeOfMem) const;

	private:
		Allocator	    *allocator;
		u32				totalAvailableBufferSpace;
		u32				minSizeOfBlocksWhengrowing;
		sBuffer			*bufferList;
		sBuffer			*lastBuffer;
		bool			bFreeFirstMemBlock;
	};
} //namespace gos
#endif //_gosBufferSparse_h_