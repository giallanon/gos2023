#ifndef _gosWaitableGrpInterface_h_
#define _gosWaitableGrpInterface_h_
#include "gosEnumAndDefine.h"
#include "gosThreadMsgQ.h"

namespace gos
{
	/***************************************************************
	* WaitableGrpInterface
	* 
	* Classe di comodo alla quale e' possibile aggiungere svariati eventi di sincronizzazione (socket, event, threadMsgQ)
	* e poi attendere che almeno uno di questi venga segnalata (tramite la wait())
	*
	* E' un oggetto che accetta altri oggetti (di tipo socket e/o event) e poi espone una funzione
	* wait() che � in grado di sospendere l'esecuzione fino a che uno (o pi�) qualunque degli oggetti che gli
	* sono stati "addati" non genera un evento.
	*
	* Nel caso degli OSEvent, � sufficiente chiamare il relativo metodo fire() per far scattare l'evento.
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

		virtual bool			cleanAll() = 0;

		virtual bool            addSocket (const gos::Socket &sok, void *userParam = NULL) = 0;
		virtual bool            addSocket (const gos::Socket &sok, u32 userParam)	= 0;
		virtual void            removeSocket(const gos::Socket &sok) = 0;

		virtual bool            addEvent(const gos::Event &evt, void *userParam = NULL)	= 0;
		virtual bool            addEvent(const gos::Event &evt, u32 userParam) = 0;
		virtual void            removeEvent(const gos::Event &event) = 0;

		virtual bool            addMsgQ (const HThreadMsgR &hRead, void *userParam = NULL) = 0;
		virtual bool            addMsgQ (const HThreadMsgR &hRead, u32 userParam) = 0;
		virtual void            removeMsgQ (const HThreadMsgR &hRead) = 0;

		/* Per specificare un tempo di wait "infinito", usare timeoutMSec=u32MAX
 		 * Per indicare il tempo di wait minimo possibile, usare timeoutMSec=0
		 * Tutti gli altri valori sono comunque validi ma non assumono significati particolari
		 *
		 * La chiamata � bloccante per almeno [timeoutMSec]
		 * Ritorna il numero di eventi ricevuti oppure 0 se non sono stati ricevuti eventi ed il timeout � scaduto
		 * Nel caso di eventi ricevuti, usare getEventOrigin() per conoscere il tipo di oggetto che ha generato
		 * l'evento i-esimo (es: gos::Socket oppure gos::Event) e usare la getEventSrc() per ottenere il puntatore all'oggetto
		 *
		 * Il numero massimo di eventi per chiamata � MAX_EVENTS_PER_CALL
		 * Ad ogni chiamata di wait(), eventuali eventi precedentemente ritornati andranno persi
		 */
		virtual u8					wait (u32 timeoutMSec) = 0;


		/* ritorna il tipo di oggetto che ha generato l'evento i-esimo */
		virtual eWaitEventOrigin	getEventOrigin(u8 iEvent) const = 0;

		virtual void*				getEventUserParamAsPtr(u8 iEvent) const = 0;

		/* ritorna lo "userParam" cos� come definito durante la chiamana addSocket() e/o addEvent() / addSerialPort / addMsgQ */
		virtual u32					getEventUserParamAsU32(u8 iEvent) const = 0;
		
		/* se getEventOrigin() == eWaitEventOrigin::socket, ritorna la soket che ha scatenato l'evento */
		virtual gos::Socket&		getEventSrcAsSocket(u8 iEvent) const = 0;

		/* come sopra */
		virtual gos::Event&			getEventSrcAsEvent(u8 iEvent) const = 0;
		
		/* come sopra */
		virtual HThreadMsgR&		getEventSrcAsMsgQ(u8 iEvent) const = 0;
		
	};

} //namespace gos


#endif //_gosWaitableGrpInterface_h_
