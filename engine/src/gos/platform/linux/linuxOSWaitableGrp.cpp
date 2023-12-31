#ifdef aaaGOS_PLATFORM__LINUX
#include <string.h>
#include "linuxOSWaitableGrp.h"
#include "../../gos.h"


//***********************************************
OSWaitableGrp::OSWaitableGrp()
{
    base = NULL;
    hfd = epoll_create1(0);
    assert (hfd!=-1);

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
    bool ret = false;
    rhea::Allocator *allocator = rhea::getSysHeapAllocator();

    while (base)
    {
        sRecord *p = base;
        base = base->next;

        allocator->dealloc(p);
        ret = true;
    }
    ::close(hfd);
    return ret;
}
//***********************************************
OSWaitableGrp::sRecord* OSWaitableGrp::priv_newRecord (u32 flags)
{
    rhea::Allocator *allocator = rhea::getSysHeapAllocator();
    sRecord *r = RHEAALLOCSTRUCT(allocator,sRecord);

    r->next = base;
    r->eventInfo.events = flags;
    r->eventInfo.data.ptr = r;

    base = r;
    return r;
}

//***********************************************
int OSWaitableGrp::priv_getFd (const sRecord *s) const
{
    switch (s->originType)
    {
    case eEventOrigin::socket:
        return s->origin.osSocket.socketID;

    case eEventOrigin::osevent:
        return s->origin.osEvent.evfd;

    case eEventOrigin::serialPort:
        return s->origin.osSerialPort.fd;

    case eEventOrigin::msgQ:
        return s->origin.ifMsgQ.hEvent.evfd;

    default:
        DBGBREAK;
        return 0;
    }
}

//***********************************************
void OSWaitableGrp::priv_findAndRemoveRecordByFD (int fd)
{
    rhea::Allocator *allocator = rhea::getSysHeapAllocator();

    sRecord *q = NULL;
    sRecord *p = base;
    while (p)
    {
        if (priv_getFd(p) == fd)
        {
            if (q == NULL)
            {
                base = base->next;
                allocator->dealloc(p);
                return;
            }

            q->next = p->next;
            allocator->dealloc(p);
            return;
        }

        q = p;
        p = p->next;
    }
}

//***********************************************
void OSWaitableGrp::priv_onRemove (int fd)
{
    priv_findAndRemoveRecordByFD(fd);

    /*  in teoria potrei passare NULL al posto di &eventInfo, ma pare ci sia un BUG in certe versioni di linux:
            In kernel versions before 2.6.9, the EPOLL_CTL_DEL operation required
            a non-null pointer in event, even though this argument is ignored.
    */
    epoll_event eventInfo;
    memset (&eventInfo, 0, sizeof(eventInfo));
    eventInfo.data.fd = fd;

    epoll_ctl(hfd, EPOLL_CTL_DEL, fd, &eventInfo);
}

//***********************************************
OSWaitableGrp::sRecord* OSWaitableGrp::priv_addSocket (const OSSocket &sok)
{
    sRecord *s = priv_newRecord (EPOLLIN | EPOLLRDHUP | EPOLLPRI | EPOLLERR | EPOLLHUP);
    s->originType = eEventOrigin::socket;
    s->origin.osSocket = sok;

    int fd = sok.socketID;
    int err = epoll_ctl (hfd, EPOLL_CTL_ADD, fd, &s->eventInfo);
    if (err)
    {
        priv_findAndRemoveRecordByFD (fd);
        return NULL;
    }

    return s;
}


//***********************************************
OSWaitableGrp::sRecord* OSWaitableGrp::priv_addEvent (const OSEvent &evt)
{
    sRecord *s = priv_newRecord (EPOLLIN | EPOLLPRI | EPOLLERR | EPOLLHUP | EPOLLET);
    s->originType = eEventOrigin::osevent;
    s->origin.osEvent = evt;

    int fd = evt.evfd;
    int err = epoll_ctl (hfd, EPOLL_CTL_ADD, fd, &s->eventInfo);
    if (err)
    {
        priv_findAndRemoveRecordByFD(fd);
        return NULL;
    }

    return s;
}

//***********************************************
OSWaitableGrp::sRecord* OSWaitableGrp::priv_addMsgQ (const HThreadMsgR &hRead)
{
    sRecord *s = priv_newRecord (EPOLLIN | EPOLLPRI | EPOLLERR | EPOLLHUP | EPOLLET);
    s->originType = eEventOrigin::msgQ;
    s->origin.ifMsgQ.hMsgQRead = hRead;
    rhea::thread::getMsgQEvent (hRead, &s->origin.ifMsgQ.hEvent);

    int fd = s->origin.ifMsgQ.hEvent.evfd;
    int err = epoll_ctl (hfd, EPOLL_CTL_ADD, fd, &s->eventInfo);
    if (err)
    {
        priv_findAndRemoveRecordByFD(fd);
        return NULL;
    }

    return s;
}

//***********************************************
void OSWaitableGrp::removeMsgQ (const HThreadMsgR &hRead)
{
    sRecord *p = base;
    while (p)
    {
        if (p->originType == eEventOrigin::msgQ)
        {
            if (p->origin.ifMsgQ.hMsgQRead == hRead)
            {
                priv_onRemove (p->origin.ifMsgQ.hEvent.evfd);
                return;
            }
        }
        p = p->next;
    }
}


//***********************************************
OSWaitableGrp::sRecord* OSWaitableGrp::priv_addSerialPort (const OSSerialPort &sp)
{
    sRecord *s = priv_newRecord (EPOLLIN | EPOLLPRI | EPOLLERR | EPOLLHUP);
    s->originType = eEventOrigin::serialPort;
    s->origin.osSerialPort = sp;

    int fd = sp.fd;
    int err = epoll_ctl (hfd, EPOLL_CTL_ADD, fd, &s->eventInfo);
    if (err)
    {
        priv_findAndRemoveRecordByFD(fd);
        return NULL;
    }


    return s;
}


//***********************************************
u8 OSWaitableGrp::wait(u32 timeoutMSec)
{
    int epollTimeout;
    if (timeoutMSec == u32MAX)
        epollTimeout = -1;
    if (timeoutMSec == 0)
        epollTimeout = 0;
    else
        epollTimeout = (int)timeoutMSec;

    nEventsReady = 0;
    int n = epoll_wait(hfd, events, MAX_EVENTS_HANDLE_PER_CALL, epollTimeout);
    if (n <= 0)
        return 0;

    nEventsReady = (u8)n;
    return nEventsReady;
}

//***********************************************
OSWaitableGrp::eEventOrigin OSWaitableGrp::getEventOrigin (u8 iEvent) const
{
    assert (iEvent < nEventsReady);

    sRecord *s = (sRecord*)events[iEvent].data.ptr;
    return (eEventOrigin)s->originType;
}

//***********************************************
void* OSWaitableGrp::getEventUserParamAsPtr (u8 iEvent) const
{
    assert (iEvent < nEventsReady);

    sRecord *s = (sRecord*)events[iEvent].data.ptr;
    return s->userParam.asPtr;
}

//***********************************************
u32 OSWaitableGrp::getEventUserParamAsU32 (u8 iEvent) const
{
    assert (iEvent < nEventsReady);

    sRecord *s = (sRecord*)events[iEvent].data.ptr;
    return s->userParam.asU32;
}

//***********************************************
OSSocket& OSWaitableGrp::getEventSrcAsOSSocket (u8 iEvent) const
{
    assert (getEventOrigin(iEvent) == eEventOrigin::socket);

    sRecord *s = (sRecord*)events[iEvent].data.ptr;
    return s->origin.osSocket;
}

//***********************************************
OSEvent& OSWaitableGrp::getEventSrcAsOSEvent (u8 iEvent) const
{
    assert (getEventOrigin(iEvent) == eEventOrigin::osevent);

    sRecord *s = (sRecord*)events[iEvent].data.ptr;
    return s->origin.osEvent;
}

//***********************************************
OSSerialPort& OSWaitableGrp::getEventSrcAsOSSerialPort (u8 iEvent) const
{
    assert (getEventOrigin(iEvent) == eEventOrigin::serialPort);

    sRecord *s = (sRecord*)events[iEvent].data.ptr;
    return s->origin.osSerialPort;
}

//***********************************************
HThreadMsgR& OSWaitableGrp::getEventSrcAsMsgQ(u8 iEvent) const
{
    assert (getEventOrigin(iEvent) == eEventOrigin::msgQ);

    sRecord *s = (sRecord*)events[iEvent].data.ptr;
    return s->origin.ifMsgQ.hMsgQRead;
}


#endif //GOS_PLATFORM__LINUX
