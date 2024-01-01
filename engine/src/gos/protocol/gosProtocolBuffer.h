#ifndef _gosProtocolBuffer_h_
#define _gosProtocolBuffer_h_
#include "../memory/gosMemory.h"


namespace gos
{
    /******************************************************************************
     * gosProtocolBuffer
     *
     * Gestisce un blocco di memoria, che viene mantenuto contiguo anche a seguito di eventuale crescita del buffer.
     * L'utilizzo classico e' nella gestione di buffer provenienti da rs232 e/o socket.
     * Tipicamente, si legge tutto il possibile dal canale di ingresso e si "appende" quanto letto a questo buffer il quale e' in grado di crescere in
     * autonomia.
     * Successivamente, si analizza il contenuto di questo buffer alla ricerca di messaggi completi e li si estraggono uno per uno.
    *******************************************************************************/
    class ProtocolBuffer
    {
    public:
                        ProtocolBuffer();
                        ~ProtocolBuffer()                                                                                   { priv_freeCurBuffer (); }

        //utilizza startingBlock come blocco di memoria iniziale. Non ne fa il free.
        //Se il buffer dovesse espandersi, allora nuova memoria viene allocata tramite il backingallocator
        //e il contenuto di startingBlock viene memcpiato nel nuovo buffer.
        //In ogni caso, startingBlock non viene mai freed, anche perchÃ¨ non Ã¨ possibile sapere da chi e' stato allocato
        void			setupWithBase (void *startingBlock, u32 sizeOfStartingBlock, gos::Allocator *backingallocator);
                            
        //alloca un blocco iniziale di dimensione=preallocNumBytes usando il backingallocator.
        //Se il buffer dovessere crescere, nuova memoria viene allocata tramite il backingallocator e il contenuto
        //del precedente buffer viene memcpiato nel nuovo.
        //Il buffer precedente viene poi freed()
        void			setup (gos::Allocator *backingallocator, u32 preallocNumBytes=0);

        void			unsetup ()																								{ priv_freeCurBuffer(); mem = NULL; allocatedSize = 0; bFreeMemBlock = 0; allocator = NULL; }

        //sposta [cursor] all'inizio del buffer
        void            reset()                                                                                                 { cursor = 0; }

        //espande la memoria allocata fino a [newSize] (solo se newSize > allocatedSize).
        //[cursor] rimane invariato
        bool			growUpTo (u32 newSize);

        //espande la memoria allocata aggiungendo [howManyBytesToAdd] al totale giÃ  allocato
        //[cursor] rimane invariato
        bool			growIncremental (u32 howManyBytesToAdd);

        //raddoppia la memoria allocata
        //[cursor] rimane invariato
        bool			growDouble ()                                                                                           { return growUpTo (allocatedSize*2); }

        //riduce la dimensione del buffer allocato portandola a newSize e memcpyando tutti i dati fino a newSize.
        //Se viene indicato un newAllocator, allora si usa quello, altrimenti si usa il solito allocator
        //se [cursor] puntava ad una locazione > [newSize], allora [cursor] = [newSize]
        void			shrink (u32 newSize, gos::Allocator *newAllocator = NULL);


        //Appende [nBytesToAppend] di [src] a partire dall'attuale posizione di [cursor]
        //Eventualmente il buffer interno viene riallocato per ospitare i nuovi dati
        bool            append (const u8 *src, u32 nBytesToAppend);

        //elimina i primi nBytesToRemove dal buffer e shifta tutto indietro.
        //[cursor] viene aggiornato di conseguenza
        void            removeFirstNByte (u32 nBytesToRemove);

        void            moveCursorTo (u32 dest)                                                                                 { if (dest > allocatedSize) cursor = allocatedSize; else cursor = dest; }
        void            advanceCursor (u32 howMuch)                                                                             { cursor += howMuch; if (cursor > allocatedSize) cursor=allocatedSize; }

        //ritorna l'offset attuale a cui punta il buffer. Vuol dire che il prossimo append
        //avverra' a partiure da mem[cursor]
        u32				getCursor() const																			            { return cursor; }

        u32				getTotalSizeAllocated() const																			{ return allocatedSize; }
        u8*				_getPointer (u32 pos) const																				{ assert(pos<allocatedSize); return &mem[pos]; }
        gos::Allocator*	getAllocator() const                                                                                { return allocator; }

    private:
        void			priv_freeCurBuffer ();

    private:
        Allocator	    *allocator;
        u8				*mem;
        u32				allocatedSize;
        u32             cursor;
        u8				bFreeMemBlock;
    };

}; //namespace gos
#endif // _gosProtocolBuffer_h_
