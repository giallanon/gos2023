#include "gosServerTCP.h"
#include "gos.h"
#include "gosString.h"
#include "gosUtils.h"
#include "protocol/gosProtocolChSocketTCP.h"
#include "protocol/gosProtocolConsole.h"
#include "protocol/gosProtocolWebsocket.h"

using namespace gos;

//****************************************************
ProtocolSocketServer::ProtocolSocketServer (u8 maxClientAllowed, gos::Allocator *allocatorIN)
{
    logger = &nullLogger;
    allocator = allocatorIN;
	gos::socket::init (&sok);
    clientList.setup (allocator, maxClientAllowed);
}

//****************************************************
ProtocolSocketServer::~ProtocolSocketServer()
{
    this->close();
    clientList.unsetup();
}

//****************************************************
eSocketError ProtocolSocketServer::start (u16 portNumber)
{
    logger->log ("ProtocolServer::start() on port %d... ", portNumber);
    logger->incIndent();

    eSocketError err = gos::socket::openAsTCPServer(&sok, portNumber);
    if (err != eSocketError::none)
    {
        logger->log ("FAIL, error code=%d\n", err);
        logger->decIndent();
        return err;
    }
    logger->log ("OK\n");

    gos::socket::setReadTimeoutMSec  (sok, 0);
    gos::socket::setWriteTimeoutMSec (sok, 10000);

    logger->log ("listen... ");
    if (!gos::socket::listen(sok))
    {
        logger->log ("FAIL\n");
        logger->decIndent();
        gos::socket::close(sok);
        return eSocketError::errorListening;
    }
    logger->log ("OK\n");


    //aggiungo la socket al gruppo di oggetti in osservazione
    waitableGrp.addSocket (sok, u32MAX);

    //setup dell'handle array
    clientList.reset();
    handleArray.setup (allocator);


    logger->log ("Done!\n");
    logger->decIndent();
    return eSocketError::none;
}

//****************************************************
void ProtocolSocketServer::close()
{
    if (!gos::socket::isOpen(sok))
        return;

    logger->log ("ProtocolServer::close()\n");
    logger->incIndent();

    //free dei client ancora connessi
    logger->log ("free of client list\n");
    while (clientList.getNElem())
    {
        HSokServerClient h = clientList(0);
        client_sendClose(h);
    }
    clientList.reset();

    waitableGrp.removeSocket(sok);

    logger->log ("closing socket\n");
    gos::socket::close(sok);

    logger->log ("handleArray.unsetup");
    handleArray.unsetup();

    logger->log ("Done!\n");
    logger->decIndent();
}


//****************************************************
bool ProtocolSocketServer::addExternalSocket1ToWaitList (gos::Socket &sok)
{ 
    return waitableGrp.addSocket (sok, EVENT_USER_PARAM_IS_EXTERNAL_SOCKET1);
}

/****************************************************
 * wait
 *
 * resta in attesa per un massimo di timeoutMSec e si sveglia quando
 * la socket riceve qualcosa, oppure quando uno qualunque degli oggetti
 * della waitableGrp si sveglia
 */
u8 ProtocolSocketServer::wait (u32 timeoutMSec)
{
    nEvents = 0;
    const u8 n = waitableGrp.wait (timeoutMSec);
    if (n == 0)
        return 0;

    //scanno gli eventi perch� non tutti vanno ritornati, ce ne sono alcuni che devo gesire da me
    for (u8 i=0; i<n; i++)
    {
        if (waitableGrp.getEventOrigin(i) == eWaitEventOrigin::osevent)
        {
            //un gos::Event e' stato fired(), lo segnalo negli eventi che ritorno
            eventList[nEvents].evtType = ProtocolSocketServer::eEventType::osevent_fired;
			eventList[nEvents].data.if_event.osEvent = &waitableGrp.getEventSrcAsEvent(i);
			eventList[nEvents++].data.if_event.userParam = waitableGrp.getEventUserParamAsU32(i);
            continue;
        }

        else if (waitableGrp.getEventOrigin(i) == eWaitEventOrigin::msgQ)
        {
            //una msgQ ha dei dati in arrivo
            eventList[nEvents].evtType = ProtocolSocketServer::eEventType::msgQ_has_data_avail;
			eventList[nEvents].data.if_msgQ.hReadAsU32 = waitableGrp.getEventSrcAsMsgQ(i).hRead.viewAsU32();
			eventList[nEvents++].data.if_msgQ.userParam = waitableGrp.getEventUserParamAsU32(i);
            continue;
        }

        else if (waitableGrp.getEventOrigin(i) == eWaitEventOrigin::socket)
        {
            //l'evento e' stato generato da una socket
            if (waitableGrp.getEventUserParamAsU32(i) == u32MAX)
            {
                //se la socket e' quella del server, allora gestiamo l'eventuale incoming connection
                HSokServerClient clientHandle;
                if (priv_checkIncomingConnection (&clientHandle))
                {
                    eventList[nEvents].evtType = ProtocolSocketServer::eEventType::new_client_connected;
                    eventList[nEvents++].data.if_socket.clientHandleAsU32 = clientHandle.handle.viewAsU32();
                }
            }
            else if (waitableGrp.getEventUserParamAsU32(i) == EVENT_USER_PARAM_IS_EXTERNAL_SOCKET1)
            {
                eventList[nEvents].evtType = ProtocolSocketServer::eEventType::external_socket1_has_data_avail;
                eventList[nEvents++].data.if_externalSok1.sok = waitableGrp.getEventSrcAsSocket(i);
            }
			else
            {
                //altimenti la socket che si e' svegliata deve essere una dei miei client gia' connessi, segnalo
                //e ritorno l'evento
                const u32 clientHandleAsU32 = waitableGrp.getEventUserParamAsU32(i);
                eventList[nEvents].evtType = ProtocolSocketServer::eEventType::client_has_data_avail;
                eventList[nEvents++].data.if_socket.clientHandleAsU32 = clientHandleAsU32;
            }
            continue;
        }
    }

    return nEvents;
}


//****************************************************
void ProtocolSocketServer::client_sendClose (const HSokServerClient hClient)
{
    sRecord *r = NULL;
    if (!handleArray.fromHandleToPointer(hClient.handle, &r))
    {
        //l'handle e' invalido!
        return;
    }

	r->protocol->close(r->ch);
    priv_onClientDeath (hClient, r);
}

//****************************************************
void ProtocolSocketServer::priv_onClientDeath (HSokServerClient hClient, sRecord *r)
{
	//logger->log("ProtocolServer::priv_onClientDeath()\n");
	//logger->incIndent();
	//logger->log("client [0x%02X]\n", hClient.asU32());

	//marco come "ignora" eventuali eventi in lista che sono relativi a questo client
	for (u8 i = 0; i < nEvents; i++)
	{
		const u16 ev = (u16)eventList[i].evtType;
		if (ev >= (u8)eEventType::new_client_connected && ev <= (u8)eEventType::client_max)
		{
			if (eventList[i].data.if_socket.clientHandleAsU32 == hClient.handle.viewAsU32())
				eventList[i].evtType = eEventType::ignore;
		}
	}

	//logger->log("removing socket\n");
	waitableGrp.removeSocket (r->ch->getSocket());

	//logger->log("closing channel\n");
	r->ch->close();

	//logger->log("removing from clientlist\n");
    clientList.findAndRemove(hClient);

	//logger->log("deleting protocol\n");
    GOSDELETE(allocator, r->protocol);

	//logger->log("deleting channel\n");
	GOSDELETE(allocator, r->ch);

	//logger->log("dealloc handle\n");
    handleArray.release(hClient.handle);

	//logger->decIndent();
}

//****************************************************
ProtocolSocketServer::eEventType ProtocolSocketServer::getEventType (u8 iEvent) const
{
    assert (iEvent < nEvents);
    return eventList[iEvent].evtType;
}

//****************************************************
u32 ProtocolSocketServer::getEventSrcAsOSEventUserParam(u8 iEvent) const
{
	assert(iEvent < nEvents);
    assert (eventList[iEvent].evtType == ProtocolSocketServer::eEventType::osevent_fired);
	return eventList[iEvent].data.if_event.userParam;
}

//****************************************************
gos::Event* ProtocolSocketServer::getEventSrcAsOSEvent(u8 iEvent) const
{
    assert (iEvent < nEvents);
    assert (eventList[iEvent].evtType == ProtocolSocketServer::eEventType::osevent_fired);
    return eventList[iEvent].data.if_event.osEvent;
}

//****************************************************
HThreadMsgR ProtocolSocketServer::getEventSrcAsMsgQHandle(u8 iEvent) const
{
    assert (iEvent < nEvents);
    assert (eventList[iEvent].evtType == ProtocolSocketServer::eEventType::msgQ_has_data_avail);

    HThreadMsgR ret;
    ret.hRead.setFromU32 (eventList[iEvent].data.if_msgQ.hReadAsU32);    
    return ret;
}

//****************************************************
gos::Socket ProtocolSocketServer::getEventSrcAsExternalSocket1(u8 iEvent) const
{
    assert (iEvent < nEvents);
    assert (eventList[iEvent].evtType == ProtocolSocketServer::eEventType::external_socket1_has_data_avail);

    return eventList[iEvent].data.if_externalSok1.sok;
}

//****************************************************
u32 ProtocolSocketServer::getEventSrcAsMsgQUserParam (u8 iEvent) const
{
	assert(iEvent < nEvents);
    assert (eventList[iEvent].evtType == ProtocolSocketServer::eEventType::msgQ_has_data_avail);
	return eventList[iEvent].data.if_msgQ.userParam;
}

//****************************************************
HSokServerClient ProtocolSocketServer::getEventSrcAsClientHandle(u8 iEvent) const
{
    assert (iEvent < nEvents);
    assert ((u8)eventList[iEvent].evtType >= 100);

    HSokServerClient h;
    h.handle.setFromU32 (eventList[iEvent].data.if_socket.clientHandleAsU32);
    return h;
}

//****************************************************
bool ProtocolSocketServer::priv_checkIncomingConnection (HSokServerClient *out_clientHandle)
{
    //logger->log ("ProtocolServer::priv_checkIncomingConnection()\n");
    //logger->incIndent();
    bool ret = priv_checkIncomingConnection2(out_clientHandle);
    //logger->decIndent();
    return ret;
}
bool ProtocolSocketServer::priv_checkIncomingConnection2 (HSokServerClient *out_clientHandle)
{
    gos::Socket acceptedSok;
	if (!gos::socket::accept(sok, &acceptedSok))
    {
        logger->log("ProtocolServer::priv_checkIncomingConnection() => accept failed\n");
		return false;
    }

	ProtocolChSocketTCP	*ch = GOSNEW(allocator, ProtocolChSocketTCP) (allocator, u16MAX, 2*u16MAX);
	ch->bindSocket(acceptedSok);



    //ok, ho un client che si vuole connettere, dovrei ricevere l'handshake
    //attendo un tot in modo da ricevere una comunicazione dalla socket appena connessa.
    //I dati che leggo devono essere un valido handshake per uno dei protocolli supportati
	const u32 nBytesRead = ch->read(5000);

    if (nBytesRead == 0 || nBytesRead >= protocol::RES_ERROR)
    {
        logger->log("ProtocolServer::priv_checkIncomingConnection() => timeout waiting for handshake. Closing connection to the client\n");
		ch->close();
		GOSDELETE(allocator, ch);
        return false;
    }

	IProtocol *protocol = NULL;
    while (1)
    {
        //Se � un client websocket, gestiamo il suo handshake
        if (ProtocolWebsocket::server_isAValidHandshake(ch->getReadBuffer(), ch->getNumBytesInReadBuffer()))
        {
            //logger->log ("it's a websocket\n");
			protocol = GOSNEW(allocator, ProtocolWebsocket) (allocator, 1024 * 8);
            break;
        }

        //Se � un client console, abbiamo un semplice handshake
        if (ProtocolConsole::server_isAValidHandshake(ch->getReadBuffer(), ch->getNumBytesInReadBuffer()))
        {
            //logger->log ("it's a console\n");
			protocol = GOSNEW(allocator, ProtocolConsole) (allocator, 512);
            break;
        }

        //errore
        logger->log ("ProtocolServer::priv_checkIncomingConnection() => client handsake is invalid, closing connection\n");
		ch->close();
		GOSDELETE(allocator, ch);
        return false;
    }

	assert(protocol != NULL);

	//provo a portare a termine l'handshake
	if (!protocol->handshake_serverAnswer(ch, logger))
	{
		logger->log("ProtocolServer::priv_checkIncomingConnection() =>  client handsake failed, closing connection\n");
		GOSDELETE(allocator, protocol);
		ch->close();
		GOSDELETE(allocator, ch);
		return false;
	}


    //tuttok ok. Genero un handle per il client
    HSokServerClient newClientHandle;
    sRecord *r = handleArray.reserve (&newClientHandle.handle);
    if (NULL == r)
    {
        logger->log ("ProtocolServer::priv_checkIncomingConnection() => connection was accepted but we have no more free handle, closing client socket\n");
		
		protocol->close(ch);
		GOSDELETE(allocator, protocol);
		
		ch->close();
		GOSDELETE(allocator, ch);
		return false;
	}
	
    *out_clientHandle = newClientHandle;
	r->protocol = protocol;
	r->ch = ch;

	waitableGrp.addSocket (acceptedSok, newClientHandle.handle.viewAsU32());
    clientList.append (newClientHandle);

    //logger->log ("Done!\n");
    return true;
}

//****************************************************
u32 ProtocolSocketServer::client_read (const HSokServerClient hClient, gos::ProtocolBuffer &out_buffer)
{
    sRecord *r = NULL;
    if (!handleArray.fromHandleToPointer(hClient.handle, &r))
    {
        //l'handle e' invalido!
        return 0;
    }

    const u32 ret = r->protocol->read (r->ch, 0, out_buffer);
	if (ret >= protocol::RES_ERROR)
    {
        priv_onClientDeath(hClient, r);
        return u32MAX;
    }
	
    return ret;
}

//****************************************************
u32 ProtocolSocketServer::client_writeBuffer (const HSokServerClient hClient, const u8 *bufferIN, u16 nBytesToWrite)
{
    sRecord *r = NULL;
    if (!handleArray.fromHandleToPointer(hClient.handle, &r))
    {
        //l'handle � invalido!
        return 0;
    }
    
	const u32 ret = r->protocol->write (r->ch, bufferIN, nBytesToWrite, 1000);
	if (ret >= protocol::RES_ERROR)
	{
		priv_onClientDeath(hClient, r);
		return u32MAX;
	}

	return ret;
}
