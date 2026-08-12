#ifndef _gosWaitableGrpInterface_h_
#define _gosWaitableGrpInterface_h_
#include "gosEnumAndDefine.h"
#include "gosThreadMsgQ.h"
#include "gosFSWatcher.h"

namespace gos
{
	/***************************************************************
	* WaitableGrpInterface
	* 
	* Classe di comodo alla quale e' possibile aggiungere svariati eventi di sincronizzazione (socket, event, threadMsgQ)
	* e poi attendere che almeno uno di questi venga segnalata (tramite la wait())
	*
	* E' un oggetto che accetta altri oggetti (di tipo socket e/o event) e poi espone una funzione
	* wait() che e' in grado di sospendere l'esecuzione fino a che uno (o piu') qualunque degli oggetti che gli
	* sono stati "addati" non genera un evento.
	*
	* Nel caso degli OSEvent, e' sufficiente chiamare il relativo metodo fire() per far scattare l'evento.
	* Nel caso di OSSocket, l'evento scatta quando ci sono dei dati pronti per essere read(), o quando la socket viene disconnessa.	* 
	*
	*
	* A causa delle sostanziali differenze tra i vari OS, questa e' solo l'interfaccia della class.
	* La specifica implementazione deve essere fatta per ogni OS
	*/
	class WaitableGrpInterface
	{
	public:
		static const u8 MAX_EVENTS_HANDLE_PER_CALL = 16;

	public:
								WaitableGrpInterface()								{ }
		virtual					~WaitableGrpInterface() { }

		virtual bool			clean_all() = 0;

		virtual bool            socket__add (const gos::Socket &sok, void *userParam = NULL) = 0;
		virtual bool            socket__add (const gos::Socket &sok, u32 userParam)	= 0;
		virtual void            socket__remove (const gos::Socket &sok) = 0;

		virtual bool            signal__add(const gos::Signal &evt, void *userParam = NULL)	= 0;
		virtual bool            signal__add(const gos::Signal &evt, u32 userParam) = 0;
		virtual void            signal__remove(const gos::Signal &event) = 0;

		virtual bool            msgQ__add (const HThreadMsgR &hRead, void *userParam = NULL) = 0;
		virtual bool            msgQ__add (const HThreadMsgR &hRead, u32 userParam) = 0;
		virtual void            msgQ__remove (const HThreadMsgR &hRead) = 0;

		/**
		 * @brief	fsWatcher__add
		 * 			Attenzione: l'istanza <fsw> deve rimanere "viva" per tutta la durata di questo WaitableGrp (o fintanto con non viene rimossa).
		 * 			E' reponsabilita' del chiamante assicurare questo fatto */
		virtual bool			fsWatcher__add (gos::FSWatcher *fsw, void *userParam=NULL) = 0;
		virtual bool			fsWatcher__add (gos::FSWatcher *fsw, u32 userParam) = 0;
		virtual void            fsWatcher__remove(gos::FSWatcher *fsw) = 0;

		/* Per specificare un tempo di wait "infinito", usare timeoutMSec=u32MAX
 		 * Per indicare il tempo di wait minimo possibile, usare timeoutMSec=0
		 * Tutti gli altri valori sono comunque validi ma non assumono significati particolari
		 *
		 * La chiamata e' bloccante per almeno [timeoutMSec]
		 * Ritorna il numero di eventi ricevuti oppure 0 se non sono stati ricevuti eventi ed il timeout e' scaduto
		 * Nel caso di eventi ricevuti, usare event__get_origin() per conoscere il tipo di oggetto che ha generato
		 * l'evento i-esimo (es: gos::Socket oppure gos::Signal) e usare la getEventSrc() per ottenere il puntatore all'oggetto
		 *
		 * Il numero massimo di eventi per chiamata e' MAX_EVENTS_PER_CALL
		 * Ad ogni chiamata di wait(), eventuali eventi precedentemente ritornati andranno persi
		 */
		virtual u8					wait (u32 timeoutMSec) = 0;


		/* ritorna il tipo di oggetto che ha generato l'evento i-esimo */
		virtual eWaitEventOrigin	event__get_origin (u8 iEvent) const = 0;

		virtual void*				event__get_user_param_as_ptr(u8 iEvent) const = 0;

		/* ritorna lo "userParam" cosi' come definito durante la chiamana socket__add() e/o signal__add() / addSerialPort / msgQ__add */
		virtual u32					event__get_user_param_as_u32(u8 iEvent) const = 0;
		
		/* se event__get_origin() == eWaitEventOrigin::socket, ritorna la soket che ha scatenato l'evento */
		virtual gos::Socket			event__get_socket_handle(u8 iEvent) const = 0;

		/* come sopra */
		virtual gos::Signal			event__get_signal_handle(u8 iEvent) const = 0;
		
		/* come sopra */
		virtual HThreadMsgR			event__get_msgQ_handle(u8 iEvent) const = 0;

		/* come sopra */
		virtual gos::FSWatcher*		event__get_fsWatcher_handle(u8 iEvent) const = 0;
		
	};

} //namespace gos


#endif //_gosWaitableGrpInterface_h_
