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
    clean_all();
}

/***********************************************
 * restituisce true se ha eliminato almeno un elemento
 */
bool OSWaitableGrp::clean_all()
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

	case eWaitEventOrigin::fsWatcher:
		return s->origin.isFSW.fd;

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
OSWaitableGrp::sRecord* OSWaitableGrp::priv_addEvent (const gos::Signal &evt)
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
    gos::thread::msgQ_getHEvent (hRead, &s->origin.ifMsgQ.event);

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
void OSWaitableGrp::msgQ__remove (const HThreadMsgR &hRead)
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
OSWaitableGrp::sRecord* OSWaitableGrp::priv_add_FSWatcher (gos::FSWatcher *fsw)
{
	const int fd = fsw->internal__open_fd();
	if (-1 == fd)
	{
		DBGBREAK;
		return NULL;
	}

    sRecord *s = priv_newRecord (EPOLLIN);
    s->originType = eWaitEventOrigin::fsWatcher;
    s->origin.isFSW.fsWatcher = fsw;
	s->origin.isFSW.fd = fd;
    
    int err = epoll_ctl (hfd, EPOLL_CTL_ADD, fd, &s->eventInfo);
    if (err)
    {
        priv_findAndRemoveRecordByFD (fd);
        return NULL;
    }

    return s;
}

//***********************************************
void OSWaitableGrp::fsWatcher__remove(gos::FSWatcher *fsw)
{
    sRecord *p = base;
    while (p)
    {
        if (eWaitEventOrigin::fsWatcher == p->originType)
        {
            if (p->origin.isFSW.fsWatcher == fsw)
            {
                priv_onRemove (p->origin.isFSW.fd);
				fsw->internal__close_fd();
                return;
            }
        }
        p = p->next;
    }
}

//***********************************************
u8 OSWaitableGrp::wait (u32 timeoutMSec)
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

	for (u8 i=0; i<nEventsReady; i++)
	{
		const sRecord *s = (const sRecord*)events[i].data.ptr;
		if (eWaitEventOrigin::fsWatcher == s->originType)
		{
			s->origin.isFSW.fsWatcher->internal__on_events_fired();
		}
	}

    return nEventsReady;
}

//***********************************************
eWaitEventOrigin OSWaitableGrp::event__get_origin (u8 iEvent) const
{
    assert (iEvent < nEventsReady);

    const sRecord *s = (const sRecord*)events[iEvent].data.ptr;
	return s->originType;
}

//***********************************************
void* OSWaitableGrp::event__get_user_param_as_ptr (u8 iEvent) const
{
    assert (iEvent < nEventsReady);

    const sRecord *s = (const sRecord*)events[iEvent].data.ptr;
    return s->userParam.asPtr;
}

//***********************************************
u32 OSWaitableGrp::event__get_user_param_as_u32 (u8 iEvent) const
{
    assert (iEvent < nEventsReady);

    const sRecord *s = (const sRecord*)events[iEvent].data.ptr;
    return s->userParam.asU32;
}

//***********************************************
gos::Socket OSWaitableGrp::event__get_socket_handle (u8 iEvent) const
{
    assert (event__get_origin(iEvent) == eWaitEventOrigin::socket);

    const sRecord *s = (const sRecord*)events[iEvent].data.ptr;
    return s->origin.socket;
}

//***********************************************
gos::Signal OSWaitableGrp::event__get_signal_handle (u8 iEvent) const
{
    assert (event__get_origin(iEvent) == eWaitEventOrigin::osevent);

    const sRecord *s = (const sRecord*)events[iEvent].data.ptr;
    return s->origin.event;
}

//***********************************************
HThreadMsgR OSWaitableGrp::event__get_msgQ_handle(u8 iEvent) const
{
    assert (event__get_origin(iEvent) == eWaitEventOrigin::msgQ);

    const sRecord *s = (const sRecord*)events[iEvent].data.ptr;
    return s->origin.ifMsgQ.hMsgQRead;
}

//***********************************************
gos::FSWatcher* OSWaitableGrp::event__get_fsWatcher_handle(u8 iEvent) const
{
    assert (event__get_origin(iEvent) == eWaitEventOrigin::fsWatcher);

    const sRecord *s = (const sRecord*)events[iEvent].data.ptr;
    return s->origin.isFSW.fsWatcher;
}

#endif //GOS_PLATFORM__LINUX
