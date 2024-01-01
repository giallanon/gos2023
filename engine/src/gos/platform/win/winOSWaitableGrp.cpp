#ifdef GOS_PLATFORM__WINDOWS
#include <string.h>
#include "winOSWaitableGrp.h"

using namespace platform;

//***********************************************
OSWaitableGrp::OSWaitableGrp()
{
	debug_bWaiting = 0;
    base = NULL;
	nEventsReady = 0;
	for (int i = 0; i < MAX_EVENTS_HANDLE_PER_CALL; i++)
		eventsHandle[i] = INVALID_HANDLE_VALUE;
}

//***********************************************
OSWaitableGrp::~OSWaitableGrp()
{
	cleanAll();
}

/***********************************************
 * restituisce true se ha eliminato almeno un elemento
 */
bool OSWaitableGrp::cleanAll()
{
	bool	ret = false;

	gos::Allocator *allocator = gos::getSysHeapAllocator();
	while (base)
	{
		sRecord *p = base;
		base = base->next;

		GOSFREE(allocator, p);

		ret = true;
	}

	return ret;
}

//***********************************************
OSWaitableGrp::sRecord* OSWaitableGrp::priv_newRecord ()
{
    gos::Allocator *allocator = gos::getSysHeapAllocator();
    sRecord *r = GOSALLOCSTRUCT(allocator,sRecord);
    r->next = base;
    base = r;
    return r;
}

//***********************************************
void OSWaitableGrp::removeSocket (const gos::Socket &sok)
{ 
	assert (debug_bWaiting == 0);
	gos::Allocator *allocator = gos::getSysHeapAllocator();


	sRecord *q = NULL;
	sRecord *p = base;
	while (p)
	{
		if (p->originType == eWaitEventOrigin::socket)
		{
			if (gos::socket::compare(sok, p->origin.socket.sok))
			{
				WSACloseEvent(p->origin.socket.hEventNotify);

				
				//rimuovo eventuali eventi che sono ancora nel miobuffer di eventi-generati
				for (u32 i = 0; i < nEventsReady; i++)
				{
					if (generatedEventList[i]->originType == eWaitEventOrigin::socket)
					{
						if (gos::socket::compare (sok, generatedEventList[i]->origin.socket.sok))
						{
							generatedEventList[i]->originType = eWaitEventOrigin::deleted;
						}
					}
				}

				if (q == NULL)
					base = base->next;
				else
					q->next = p->next;

				GOSFREE(allocator, p);
				return;
			}
		}

		q = p;
		p = p->next;
	}
}

//***********************************************
OSWaitableGrp::sRecord* OSWaitableGrp::priv_addSocket (const os::Socket &sok)
{
	assert(debug_bWaiting == 0);

	sRecord *s = priv_newRecord();
	s->originType = eWaitEventOrigin::socket;
	s->origin.socket.sok = sok;
	s->origin.socket.hEventNotify = WSACreateEvent();

	//WSAEventSelect(sok.socketID, sok.hEventNotify, FD_READ | FD_WRITE | FD_OOB | FD_ACCEPT | FD_CONNECT | FD_CLOSE | FD_QOS | FD_ROUTING_INTERFACE_CHANGE | FD_ADDRESS_LIST_CHANGE);
	WSAEventSelect (sok.osSok.socketID, s->origin.socket.hEventNotify, FD_READ | FD_OOB | FD_ACCEPT | FD_CONNECT | FD_CLOSE | FD_QOS | FD_ROUTING_INTERFACE_CHANGE | FD_ADDRESS_LIST_CHANGE);

    return s;
}

//***********************************************
void OSWaitableGrp::removeEvent (const gos::Event &evt)
{
	assert(debug_bWaiting == 0);

	gos::Allocator *allocator = gos::getSysHeapAllocator();

	sRecord *q = NULL;
	sRecord *p = base;
	while (p)
	{
		if (p->originType == eWaitEventOrigin::osevent)
		{
			if (gos::thread::eventCompare(p->origin.event.evt, evt))
			{
				//rimuovo eventuali eventi che sono ancora nel miobuffer di eventi-generati
				for (u32 i = 0; i < nEventsReady; i++)
				{
					if (generatedEventList[i]->originType == eWaitEventOrigin::osevent)
					{
						if (gos::thread::eventCompare(evt, generatedEventList[i]->origin.event.evt))
						{
							generatedEventList[i]->originType = eWaitEventOrigin::deleted;
						}
					}
				}

				if (q == NULL)
					base = base->next;
				else
					q->next = p->next;
				GOSFREE(allocator, p);
				return;
			}
		}

		q = p;
		p = p->next;
	}
}

//***********************************************
OSWaitableGrp::sRecord* OSWaitableGrp::priv_addEvent (const gos::Event &evt)
{
	assert(debug_bWaiting == 0);

	sRecord *s = priv_newRecord();
	s->originType = eWaitEventOrigin::osevent;
    s->origin.event.evt = evt;
    return s;
}

//***********************************************
OSWaitableGrp::sRecord* OSWaitableGrp::priv_addMsgQ (const HThreadMsgR &hRead)
{
	assert(debug_bWaiting == 0);
	sRecord *s = priv_newRecord();
	s->originType = eWaitEventOrigin::msgQ;
	s->origin.msgQ.hRead = hRead;
	gos::thread::getMsgQEvent (hRead, &s->origin.msgQ.evt);
	return s;
}

//***********************************************
void OSWaitableGrp::removeMsgQ (const HThreadMsgR &hRead)
{
	assert(debug_bWaiting == 0);
	gos::Allocator *allocator = gos::getSysHeapAllocator();

	gos::Event hMsgQEvent;
	gos::thread::getMsgQEvent (hRead, &hMsgQEvent);
	
	sRecord *q = NULL;
	sRecord *p = base;
	while (p)
	{
		if (p->originType == eWaitEventOrigin::msgQ)
		{
			if (gos::thread::eventCompare (p->origin.msgQ.evt, hMsgQEvent))
			{
				//rimuovo eventuali eventi che sono ancora nel miobuffer di eventi-generati
				for (u32 i = 0; i < nEventsReady; i++)
				{
					if (generatedEventList[i]->originType == eWaitEventOrigin::msgQ)
					{
						if (gos::thread::eventCompare (hMsgQEvent, generatedEventList[i]->origin.msgQ.evt))
							generatedEventList[i]->originType = eWaitEventOrigin::deleted;
					}
				}

				if (q == NULL)
					base = base->next;
				else
					q->next = p->next;
				GOSFREE(allocator, p);
				return;
			}
		}

		q = p;
		p = p->next;
	}
}

//***********************************************
u8 OSWaitableGrp::wait(u32 timeoutMSec)
{
	assert(debug_bWaiting == 0);
	debug_bWaiting = 1;
	u8 ret = priv_wait(timeoutMSec);
	debug_bWaiting = 0;
	return ret;
}

//***********************************************
u8 OSWaitableGrp::priv_wait(u32 timeoutMSec)
{
//per printare un po' di info di debug, definisci la seguente
#undef OSWAITABLE_GRP_DEBUG_TEXT


#ifdef OSWAITABLE_GRP_DEBUG_TEXT
	#define DEBUG_PRINTF(...) printf(__VA_ARGS__)
	#define ADD_EVENT_AND_DEBUG_TEXT(p, debug_text) { generatedEventList[nEventsReady++] = p; printf("  wait::" debug_text "\n"); }
#else
	#define DEBUG_PRINTF(...) {}
	#define ADD_EVENT_AND_DEBUG_TEXT(p, debug_text) { generatedEventList[nEventsReady++] = p;}
#endif

	nEventsReady = 0;
	for (u8 i = 0; i < MAX_EVENTS_HANDLE_PER_CALL; i++)
		eventsHandle[i] = INVALID_HANDLE_VALUE;

	//creo la lista di handle sulla quale fare la WaitForMultipleObjects
	DWORD n = 0;
	sRecord *p = base;
	while (p)
	{
		switch (p->originType)
		{
		default:
			DBGBREAK;
			break;

		case eWaitEventOrigin::socket:
			eventsHandle[n++] = p->origin.socket.hEventNotify;
			break;

		case eWaitEventOrigin::osevent:
			eventsHandle[n++] = p->origin.event.evt.osEvt.h;
			break;

		case eWaitEventOrigin::msgQ:
			eventsHandle[n++] = p->origin.msgQ.evt.osEvt.h;
			break;
		}
		p = p->next;
	}

	if (n > MAX_EVENTS_HANDLE_PER_CALL)
	{
		//posso gestire un numero finito di handle.
		//Eventuali altri handle semplicemente li ignoro
		n = MAX_EVENTS_HANDLE_PER_CALL;
	}
	
	//timeout per la wait
	DWORD dwMilliseconds = (DWORD)timeoutMSec;
	if (timeoutMSec == u32MAX)
		dwMilliseconds = INFINITE;

	if (n == 0)
	{
		assert (dwMilliseconds != INFINITE);
		::Sleep(dwMilliseconds);
		return 0;
	}



	DEBUG_PRINTF("\n\nOSWaitableGrp::wait\n");
	DEBUG_PRINTF("  n=%d\n", n);
	for (int i = 0; i < MAX_EVENTS_HANDLE_PER_CALL; i++)
		DEBUG_PRINTF("  %X", eventsHandle[i]);

	//Attendo pazientemente...
	assert(debug_bWaiting == 1);
	DWORD ret = WaitForMultipleObjects(n, eventsHandle, FALSE, dwMilliseconds);
	assert(debug_bWaiting == 1);

	DEBUG_PRINTF("  ret=%d\n", ret);

	if (ret == WAIT_TIMEOUT)
	{
		DEBUG_PRINTF("  WAIT_TIMEOUT\n", ret);
		return 0;
	}
		

	if (ret == WAIT_FAILED)
	{
		DBGBREAK;
		return 0;
	}

	DWORD index;
	if (ret >= WAIT_ABANDONED_0)
		index = ret - WAIT_ABANDONED_0;
	else
		index = ret - WAIT_OBJECT_0;
		

	//cerco l'handle che ha generato l'interruzione
	if (index < MAX_EVENTS_HANDLE_PER_CALL)
	{
		p = base;
		while (p)
		{
			bool bFound = false;
			switch (p->originType)
			{
			default:
				DBGBREAK;
				break;

			case eWaitEventOrigin::socket:
				if (eventsHandle[index] == p->origin.socket.hEventNotify)
					bFound = true;
				break;

			case eWaitEventOrigin::osevent:
				if (eventsHandle[index] == p->origin.event.evt.osEvt.h)
					bFound = true;
				break;

			case eWaitEventOrigin::msgQ:
				if (eventsHandle[index] == p->origin.msgQ.evt.osEvt.h)
					bFound = true;
				break;
			}

			if (bFound)
			{
				if (p->originType == eWaitEventOrigin::osevent || p->originType == eWaitEventOrigin::msgQ)
				{
					generatedEventList[nEventsReady++] = p;
				}
				else if (p->originType == eWaitEventOrigin::socket)
				{
					WSANETWORKEVENTS networkEvents;
					if (0 != WSAEnumNetworkEvents(p->origin.socket.sok.osSok.socketID, p->origin.socket.hEventNotify, &networkEvents))
					{
						int errCode = WSAGetLastError();
						switch (errCode)
						{
						case WSANOTINITIALISED:
							//A successful WSAStartup call must occur before using this function.
							DBGBREAK;
							break;
						case WSAENETDOWN:
							//The network subsystem has failed.
							DBGBREAK;
							break;
						case WSAEINVAL:
							//One of the specified parameters was invalid.
							DBGBREAK;
							break;
						case WSAEINPROGRESS:
							//A blocking Windows Sockets 1.1 call is in progress, or the service provider is still processing a callback function.
							DBGBREAK;
							break;
						case WSAENOTSOCK:
							//The descriptor is not a socket.
							DBGBREAK;
							break;
						case WSAEFAULT:
							//The lpNetworkEvents parameter is not a valid part of the user address space.
							DBGBREAK;
							break;

						}
					}

					//if (networkEvents.lNetworkEvents == 0) ::ResetEvent(p->origin.socket.hEventNotify);

					DEBUG_PRINTF("  was a socket [userparam=%d], event bits = 0x%X\n", p->userParam.asU32, networkEvents.lNetworkEvents);
					if ((networkEvents.lNetworkEvents & FD_CLOSE) != 0)						ADD_EVENT_AND_DEBUG_TEXT(p, "FD_CLOSE")
					if ((networkEvents.lNetworkEvents & FD_READ) != 0)						ADD_EVENT_AND_DEBUG_TEXT(p, "FD_READ")
					if ((networkEvents.lNetworkEvents & FD_WRITE) != 0)						ADD_EVENT_AND_DEBUG_TEXT(p, "FD_WRITE")
					if ((networkEvents.lNetworkEvents & FD_OOB) != 0)						ADD_EVENT_AND_DEBUG_TEXT(p, "FD_OOB")
					if ((networkEvents.lNetworkEvents & FD_ACCEPT) != 0)					ADD_EVENT_AND_DEBUG_TEXT(p, "FD_ACCEPT")
					if ((networkEvents.lNetworkEvents & FD_CONNECT) != 0)					ADD_EVENT_AND_DEBUG_TEXT(p, "FD_CONNECT")
					if ((networkEvents.lNetworkEvents & FD_QOS) != 0)						ADD_EVENT_AND_DEBUG_TEXT(p, "FD_QOS")
					if ((networkEvents.lNetworkEvents & FD_ROUTING_INTERFACE_CHANGE) != 0)	ADD_EVENT_AND_DEBUG_TEXT(p, "FD_ROUTING_INTERFACE_CHANGE")
					if ((networkEvents.lNetworkEvents & FD_ADDRESS_LIST_CHANGE) != 0)		ADD_EVENT_AND_DEBUG_TEXT(p, "FD_ADDRESS_LIST_CHANGE")
				}

				return nEventsReady;
			}
			p = p->next;
		}
	}

	DEBUG_PRINTF("  handle not found\n", ret);

	return 0;

#undef DEBUG_PRINTF
#undef ADD_EVENT_AND_DEBUG_TEXT
}

//***********************************************
eWaitEventOrigin OSWaitableGrp::getEventOrigin (u8 iEvent) const
{
	assert(debug_bWaiting == 0);
    assert (iEvent < nEventsReady);
	return generatedEventList[iEvent]->originType;
}

//***********************************************
void* OSWaitableGrp::getEventUserParamAsPtr (u8 iEvent) const
{
	assert(debug_bWaiting == 0);
	assert(iEvent < nEventsReady);
	return generatedEventList[iEvent]->userParam.asPtr;
}

//***********************************************
u32 OSWaitableGrp::getEventUserParamAsU32 (u8 iEvent) const
{
	assert(debug_bWaiting == 0);
	assert(iEvent < nEventsReady);
	return generatedEventList[iEvent]->userParam.asU32;
}

//***********************************************
gos::Socket& OSWaitableGrp::getEventSrcAsSocket (u8 iEvent) const
{
	assert(debug_bWaiting == 0);
    assert (getEventOrigin(iEvent) == eWaitEventOrigin::socket);
    return generatedEventList[iEvent]->origin.socket.sok;
}

//***********************************************
gos::Event& OSWaitableGrp::getEventSrcAsEvent (u8 iEvent) const
{
	assert(debug_bWaiting == 0);
    assert (getEventOrigin(iEvent) == eWaitEventOrigin::osevent);
	return generatedEventList[iEvent]->origin.event.evt;
}


//***********************************************
HThreadMsgR& OSWaitableGrp::getEventSrcAsMsgQ(u8 iEvent) const
{
	assert(debug_bWaiting == 0);
	assert (getEventOrigin(iEvent) == eWaitEventOrigin::msgQ);
	return generatedEventList[iEvent]->origin.msgQ.hRead;
}

#endif //GOS_PLATFORM__WINDOWS