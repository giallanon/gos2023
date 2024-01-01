#ifndef _gosIProtocol_h_
#define _gosIProtocol_h_
#include "gosIProtocolChannel.h"
#include "gosProtocolBuffer.h"
#include "../logger/gosLogger.h"


namespace gos
{
    /****************************************************************
     *  IProtocol
     *
     *  Interfaccia di base per tutti i protocolli di comunicazione
	 *	Espone dei metodi generici (read, write, close) che le classi derivate devono implementare	
	 *	in modo da astrarre i dettagli implementativi del singolo protocollo da quelle che sono le funzioni di comunicazione
	 *
	 *
     */
    class IProtocol
    {
    public:
						IProtocol (Allocator * allocatorIN, u32 startingSizeOfWriteBuffer);
		virtual         ~IProtocol();


        /* invia l'handshake al server e aspetta la risposta.
         * Ritorna true se tutto ok, false altrimenti*/
		virtual bool	handshake_clientSend (IProtocolChannel *ch, gos::Logger *logger = NULL) = 0;

        /* il server si aspetta che il client inizi la connessione con uno specifico handshake.
         * Se il primi nByte letti dal canale di comunicazione sono un valido handshake, questa fn ritorna il numero
         * di byte consumati da bufferIN e provvede a rispondere al client, altrimenti ritorna 0 */
		virtual bool	handshake_serverAnswer (IProtocolChannel *ch, gos::Logger *logger = NULL) = 0;

        /* Invia eventualmente manda un messaggio di close (se il protocollo lo prevede)*/
		void			close (IProtocolChannel *ch)
						{
							
							if (ch->isOpen())
								virt_sendCloseMessage(ch);
						}

        /* legge dal canale di comunicazione (non bloccante se timeoutMSec==0) ed interpreta il buffer di input.
         * Se c'è almeno un valido messaggi "utente", lo mette in out_result e ritorna il numero di bytes inseriti in out_result.
         * Eventuali messaggi interni di protocollo (ping, close, keepalive..) vengono gestiti internamente e non
         * vengono passati a out_result.
         * La read ritorna solo messaggi "interi" e solo 1 messaggio alla volta.
         * Anche se nel buffer del canale ci fossero pronti n messaggi, read ne ritorna 1 solo alla volta
         * In caso di errore, il valore ritornato è >= protocol::RES_ERROR;
         * Ad esempio, se durante la lettura per qualunque motivo il canale dovesse chiudersi, la fn ritornerebb protocol::RES_CHANNEL_CLOSED.
         * Se durante la lettura, riceve un esplicito messaggio di close (di protocollo), ritorna RES_PROTOCOL_CLOSED
         * Vedi IProtocolChannel::read per ulteriori dettagli */
        u32				read (IProtocolChannel *ch, u32 timeoutMSec, ProtocolBuffer &out_result);

        /* Prova a scrivere [nBytesToWrite], e ci prova per un tempo massimo di [timeoutMSec].
         * Ritorna il numero di bytes scritti sul canale di comunicazione.
         * In caso di errore, il valore ritornato è >= protocol::RES_ERROR;
         * Ad esempio, se durante la scrittura per qualunque motivo il canale dovesse chiudersi, la fn ritornerebb protocol::RES_CHANNEL_CLOSED.
         * Vedi IProtocolChannel::write per ulteriori dettagli */
		u32				write (IProtocolChannel *ch, const u8 *bufferToWrite, u16 nBytesToWrite, u32 timeoutMSec);


	protected:
		virtual	void	virt_sendCloseMessage(IProtocolChannel *ch) = 0;

        /*	Dato un [bufferIN] che contiene almeno [nBytesInBufferIN] letti da qualche device, questa fn prova ad interpretarne
        il significato secondo il protocollo che implementa.
        Se il buffer contiene almeno un valido pacchetto dati:
        ritorna il numero di bytes di bufferIN "consumati"
        mette in [out_result] il payload del msg (a partire dal byte startOffset)
        mette in [out_nBytesInseritiInOutResult] il num di bytes inseriti in out_result (ie: il payloadLen).

        In caso di errore, ritorna >= protocol::RES_ERROR (vedi read)

        Se ritorna 0, vuol dire che in bufferIN non c'era un numero di bytes suff a comporre un intero messaggio, oppure che la funzione
        non è stata in grado di decodificarne il significato.

        ATTENZIONE: la funzione potrebbe ritornare un numero > 0 e contemporaneamente un [out_nBytesInseritiInOutResult]==0. Questo è il caso
        in cui il protocollo implementa dei messaggi interni di comando (per es: ping, keep alive...) per cui di fatto qualcosa di bufferIN è
        stato consumato dalla funzione, ma niente di utile viene inserito in out_result */
		virtual u32		virt_decodeBuffer (IProtocolChannel *ch, const u8 *bufferIN, u32 nBytesInBufferIN, ProtocolBuffer &out_result, u32 *out_nBytesInseritiInOutResult) = 0;

        /* prende [bufferToEncode] e lo codifica secondo il protocollo che implementa. IL risultato viene messo
         * in out_buffer. Ritorna il numero di bytes inseriti in out_buffer */
		virtual u32		virt_encodeBuffer (const u8 *bufferToEncode, u32 nBytesToEncode, ProtocolBuffer &out_buffer) = 0;


	protected:
		ProtocolBuffer	bufferW;	//buffer di appoggio usato per "encodare" i messaggi
	};

} //namespace gos

#endif // _gosIProtocol_h_

