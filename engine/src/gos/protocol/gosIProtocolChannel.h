#ifndef _gosIProtocolChannel_h_
#define _gosIProtocolChannel_h_
#include "gosProtocolEnumAndDefine.h"
#include "gosProtocolBuffer.h"

namespace gos
{
	/****************************************************************
	*  IProtocolChannel
	*
	*	Astrazione di un canale di comunicazione (es: socket, seriale...)
	*	Espone dei metodi generici (read, write, close) che le classi derivate devono implementare
    *	in modo da astrarre la "fisicità" del dispositivo di comunicazione dal suo funzionamento logico.
    *
    *   La read() legge dal dispositivo e bufferizza in [bufferR] il quale può espandersi fino a [maxSizeOfReadBuffer]
	*/
	class IProtocolChannel
	{
	public:
							IProtocolChannel (Allocator *allocatorIN, u32 startingSizeOfReadBufferInBytes, u32 maxSizeOfReadBuffer);
		virtual				~IProtocolChannel();

        /* true se il canale è fisicamente aperto ed è quindi legale usare read/write*/
		bool				isOpen() const														{ return virt_isOpen(); }

        /* chiude fisicamente il canale di comunicazione */
		void				close()																{ virt_close(); }

        /*	legge dal canale di comunicazione (non bloccante se timeoutMSec==0) ed eventualmente filla un buffer interno con i bytes ricevuti.
            Ritorna il numero di bytes letti e messi nel buffer di lettura.
            Attenzione che il numero di byte letti in questa chiamata non è detto che sia == al numero di bytes attualmente presenti nel buffer (vedi getNumBytesInReadBuffer()).
            Supponiamo di chiamare read() una prima volta, questa ritorna 5. In questo momento abbiamo 5 bytes nel buffer interno.
            Supponiamo di chiamarla di nuovo dopo 2 secondi, questa ritorna 2. Nel buffer interno, ora abbiamo 7 bytes

            In caso di errore, il valore ritornato è >= protocol::RES_ERROR
            Ad esempio, se durante la lettura per qualunque motivo il canale dovesse chiudersi, la fn ritorna protocol::RES_CHANNEL_CLOSED */
		u32					read (u32 timeoutMSec);

        /*	Prova a scrivere [nBytesToWrite], e ci prova per un tempo massimo di [timeoutMSec].
            Ritorna il numero di bytes scritti sul canale di comunicazione.
            In caso di errore, il valore ritornato è >= protocol::RES_ERROR
            Ad esempio, se durante la scrittra per qualunque motivo il canale dovesse chiudersi, la fn ritorna protocol::RES_CHANNEL_CLOSED */
		u32					write (const u8 *bufferToWrite, u32 nBytesToWrite, u32 timeoutMSec);


		const u8*			getReadBuffer() const												{ return bufferR._getPointer(0); }
		u32					getNumBytesInReadBuffer() const										{ return bufferR.getCursor(); }
		
        //Shifta il read buffer eliminando i primi nBytes. 
        void				consumeReadBuffer(u32 nBytes)										{ bufferR.removeFirstNByte (nBytes); }
        

	protected:
		virtual bool		virt_isOpen() const = 0;
		virtual void		virt_close() = 0;
		
        // Legge al massimo [nMaxBytesToRead] e li mette in [buffer].								
        // Riguardo al valore ritornato, deve seguire la stessa policy di read()
		virtual u32			virt_read (u8 *buffer, u32 nMaxBytesToRead, u32 timeoutMSec) = 0;

		//riguardo al valore ritornato, deve seguire la stessa policy di write()
        virtual u32			virt_write (const u8 *bufferToWrite, u32 nBytesToWrite) = 0;

	private:
		ProtocolBuffer		bufferR;
		u32					maxSizeOfReadBuffer;

	};
    
} // namespace gos

#endif // _gosIProtocolChannel_h_
