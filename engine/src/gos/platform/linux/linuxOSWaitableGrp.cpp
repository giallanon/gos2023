#ifdef GOS_PLATFORM__LINUX
#include <string.h>
#include "linuxOSWaitableGrp.h"
#include "../../gos.h"

using namespace platform;

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
    gos::Allocator *allocator = gos::getSysHeapAllocator();

    while (base)
    {
        sRecord *p = base;
        base = base->next;

        GOSFREE(allocator, p);
        ret = true;
    }
    ::close(hfd);
    return ret;
}

//***********************************************
OSWaitableGrp::sRecord* OSWaitableGrp::priv_newRecord (u32 flags)
{
    gos::Allocator *allocator = gos::getSysHeapAllocator();
    sRecord *r = GOSALLOCSTRUCT(allocator,sRecord);

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
    case eWaitEventOrigin::socket:
        return s->origin.socket.osSok.socketID;

    case eWaitEventOrigin::osevent:
        return s->origin.event.osEvt.evfd;

    case eWaitEventOrigin::msgQ:
        return s->origin.ifMsgQ.event.osEvt.evfd;

    default:
        DBGBREAK;
        return 0;
    }
}

//***********************************************
void OSWaitableGrp::priv_findAndRemoveRecordByFD (int fd)
{
    gos::Allocator *allocator = gos::getSysHeapAllocator();

    sRecord *q = NULL;
    sRecord *p = base;
    while (p)
    {
        if (priv_getFd(p) == fd)
        {
            if (q == NULL)
            {
                base = base->next;
                GOSFREE(allocator, p);
                return;
            }

            q->next = p->next;
            GOSFREE(allocator, p);
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
OSWaitableGrp::sRecord* OSWaitableGrp::priv_addSocket (const gos::Socket &sok)
{
    sRecord *s = priv_newRecord (EPOLLIN | EPOLLRDHUP | EPOLLPRI | EPOLLERR | EPOLLHUP);
    s->originType = eWaitEventOrigin::socket;
    s->origin.socket = sok;

    int fd = sok.osSok.socketID;
    int err = epoll_ctl (hfd, EPOLL_CTL_ADD, fd, &s->eventInfo);
    if (err)
    {
        priv_findAndRemoveRecordByFD (fd);
        return NULL;
    }

    return s;
}


//***********************************************
OSWaitableGrp::sRecord* OSWaitableGrp::priv_addEvent (const gos::Event &evt)
{
    sRecord *s = priv_newRecord (EPOLLIN | EPOLLPRI | EPOLLERR | EPOLLHUP | EPOLLET);
    s->originType = eWaitEventOrigin::osevent;
    s->origin.event = evt;

    int fd = evt.osEvt.evfd;
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
    s->originType = eWaitEventOrigin::msgQ;
    s->origin.ifMsgQ.hMsgQRead = hRead;
    gos::thread::getMsgQEvent (hRead, &s->origin.ifMsgQ.event);

    int fd = s->origin.ifMsgQ.event.osEvt.evfd;
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
        if (p->originType == eWaitEventOrigin::msgQ)
        {
            if (p->origin.ifMsgQ.hMsgQRead == hRead)
            {
                priv_onRemove (p->origin.ifMsgQ.event.osEvt.evfd);
                return;
            }
        }
        p = p->next;
    }
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
eWaitEventOrigin OSWaitableGrp::getEventOrigin (u8 iEvent) const
{
    assert (iEvent < nEventsReady);

    sRecord *s = (sRecord*)events[iEvent].data.ptr;
    return (eWaitEventOrigin)s->originType;
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
gos::Socket& OSWaitableGrp::getEventSrcAsSocket (u8 iEvent) const
{
    assert (getEventOrigin(iEvent) == eWaitEventOrigin::socket);

    sRecord *s = (sRecord*)events[iEvent].data.ptr;
    return s->origin.socket;
}

//***********************************************
gos::Event& OSWaitableGrp::getEventSrcAsEvent (u8 iEvent) const
{
    assert (getEventOrigin(iEvent) == eWaitEventOrigin::osevent);

    sRecord *s = (sRecord*)events[iEvent].data.ptr;
    return s->origin.event;
}

//***********************************************
HThreadMsgR& OSWaitableGrp::getEventSrcAsMsgQ(u8 iEvent) const
{
    assert (getEventOrigin(iEvent) == eWaitEventOrigin::msgQ);

    sRecord *s = (sRecord*)events[iEvent].data.ptr;
    return s->origin.ifMsgQ.hMsgQRead;
}


#endif //GOS_PLATFORM__LINUX
