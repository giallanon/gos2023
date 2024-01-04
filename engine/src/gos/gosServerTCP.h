#ifndef _gosServerTCP_h_
#define _gosServerTCP_h_
#include "gos.h"
#include "gosFastArray.h"
#include "gosThreadMsgQ.h"
#include "gosWaitableGrp.h"
#include "logger/gosLoggerNull.h"
#include "protocol/gosIProtocol.h"
#include "protocol/gosProtocolChSocketTCP.h"
#include "protocol/gosProtocolBuffer.h"

//A per "num max di handle", B per "num di chunk", C per "counter"
typedef gos::HandleT<10,6,16, 0,0>	HSokServerClientHandle;		//2^10=1024 => num totale di oggetti, divisi in chunk da 2^6=64

//handle per la gestione dei client
struct HSokServerClient
{
    HSokServerClientHandle handle;

    bool    operator== (const HSokServerClient &b) const                        { return (handle==b.handle); }
    bool    operator!= (const HSokServerClient &b) const                        { return (handle!=b.handle); }
};

namespace gos
{
    /**************************************************************************
     * ServerTCP
     *
	 *	Apre una socket in listen sulla porta [portNumber] (vedi start) e attende connessioni.
	 *	I client che si connettono possono usare uno qualunque dei protocolli che implementano IProtocol
     */
    class ServerTCP
    {
    public:
        enum class eEventType: u8
        {
            osevent_fired                       = 1,        //un gos::Event è stato fired
            msgQ_has_data_avail                 = 2,        //una msgQ ha dei dati disponibili
            external_socket1_has_data_avail     = 3,        //la socket esterna "soket1" ha dei dati disponibili
            
            //eventi da 100 a 199 sono relativi a socket di client connessi
            new_client_connected = 100,
            client_has_data_avail = 101,
			client_max = 199,

			ignore  = 0xfe,
            unknown = 0xff
        };

    public:
        // AllocatorIN è usato internamente da questa classe per allocare tutto quello che le serve (es: i client, i buffer interni e via dicendo).
        // Considerando che questa classe non è thread safe, potrebbe valere la pena utilizzare un allocator non thread safe se si vuole guadagnare qualcosina
        // in termini di performace
                            ServerTCP (u8 maxClientAllowed, gos::Allocator *allocatorIN);

        virtual             ~ServerTCP();

        void                useLogger (Logger *loggerIN)                                                                { if (NULL==loggerIN) logger=&nullLogger; else logger=loggerIN; }
		

        eSocketError        start (u16 portNumber);
        void                close ();
        
        // aggiunge/rimuove un gos::Event all'elenco degli oggetti osservati dalla wait()
        bool                addEventToWaitList (const gos::Event evt, u32 userParam=0)                                  { return waitableGrp.addEvent (evt, userParam); }
        void                removeEventFromWaitList (const gos::Event evt)                                              { waitableGrp.removeEvent (evt); }

        bool                addMsgQToWaitList (const HThreadMsgR &hRead, u32 userParam)                                 { return waitableGrp.addMsgQ (hRead, userParam); }
        void                removeMsgQFromWaitList (const HThreadMsgR &hRead)                                           { waitableGrp.removeMsgQ (hRead); }

        //this può avere al max 1 external socket
	    bool                addExternalSocket1ToWaitList (gos::Socket &sok);
        void                removeExternalSocket1FromWaitList (gos::Socket &sokIN)                                      { waitableGrp.removeSocket (sokIN); }
                                
        // Chiamata bloccante per un max di timeoutMSec:
        // per specificare un tempo di wait "infinito" (ie: socket sempre bloccante), usare timeoutMSec=u32MAX
        // per indicare il tempo di wait minimo possibile, usare timeoutMSec=0
        // tutti gli altri valori sono comunque validi ma non assumono significati particolari.
        //
        // La funzione termina se il timeout scade oppure se uno o più eventi sono occorsi.
        // Ritorna il numero di eventi disponibili.
        // Per conoscere il tipo di evento e recuperare il "chi" ha generato l'evento, vedi le fn getEvent...()
        u8                  wait (u32 timeoutMSec);

        eEventType          getEventType (u8 iEvent) const;
        
        gos::Event*         getEventSrcAsEvent (u8 iEvent) const;
		u32					getEventSrcAsEventUserParam (u8 iEvent) const;
        
        HThreadMsgR         getEventSrcAsMsgQHandle (u8 iEvent) const;
        u32                 getEventSrcAsMsgQUserParam (u8 iEvent) const;
        
        HSokServerClient    getEventSrcAsClientHandle(u8 iEvent) const;

        gos::Socket         getEventSrcAsExternalSocket1(u8 iEvent) const;

        //A seguito di un evento "evt_client_has_data_avail" (vedi wait()), chiamare questa fn per flushare i dati.
        //Nel caso in cui tra i dati letti ci fossero dei messaggi utili, il loro payload viene messi in out_buffer.
        //
        //Eventuali messaggi di controllo (come previsti per es dal protocollo websocket), vengono gestiti internamente in totale
        //autonomia (ping, pong, close..)
        //
        //Ritorna il numero di bytes inseriti in out_buffer oppure u32MAX per indicare che la connessione è stata chiusa
        u32                 client_read (const HSokServerClient hClient, gos::ProtocolBuffer &out_buffer);

		//ritorna il num di byte scritti, oppure u32MAX in caso di disconnessione del client
        u32                 client_writeBuffer(const HSokServerClient hClient, const u8 *bufferIN, u16 nBytesToWrite);
		
		void                client_sendClose (const HSokServerClient hClient);
			   
        u32                 client_getNumConnected() const                              { return clientList.getNElem(); }
        HSokServerClient    client_getByIndex (u32 i) const                             { return clientList(i); }

    private:
        static const u32    EVENT_USER_PARAM_IS_EXTERNAL_SOCKET1   = 0xFFFFFFFD;

        enum eClientType
        {
            eClientType_unknown = 0,
            eClientType_websocket = 1,
            eClientType_console = 2
        };

    private:
		struct sDataForEvent_event
		{
			gos::Event         *osEvent;
			u32				userParam;
		};

        struct sDataForEvent_msgQ
        {
            u32         hReadAsU32;
            u32			userParam;
        };

		struct sDataForEvent_socket
		{
			u32             clientHandleAsU32;
		};

		struct sDataForEvent_externalSocket1
		{
			gos::Socket        sok;
		};

        union uEventData
        {
			sDataForEvent_event		        if_event;
            sDataForEvent_msgQ              if_msgQ;
			sDataForEvent_socket	        if_socket;
            sDataForEvent_externalSocket1   if_externalSok1;
        };

        struct sEvent
        {
            eEventType  evtType;
            uEventData  data;
        };

        struct sRecord
        {
            IProtocol			*protocol;
			ProtocolChSocketTCP	*ch;
        };

        struct sUDPClient
        {
            gos::NetAddr        addr;
            gos::BufferLinear   bufferR;
            u16                 numByteInBufferR;
        };

    private:
        bool                priv_checkIncomingConnection (HSokServerClient *out_clientHandle);
        bool                priv_checkIncomingConnection2 (HSokServerClient *out_clientHandle);
        void                priv_onClientDeath (HSokServerClient handle, sRecord *r);

    private:
        Allocator                               *allocator;
        Logger                                  *logger;
        LoggerNull                              nullLogger;
        sEvent                                  eventList[WaitableGrp::MAX_EVENTS_HANDLE_PER_CALL];
        WaitableGrp                             waitableGrp;
        gos::Socket                             sok;
        u8                                      nEvents;

        HandleList<HSokServerClientHandle,sRecord>   handleArray;
        FastArray<HSokServerClient>             clientList;
    };
} //namespace gos


#endif // _gosServerTCP_h_
