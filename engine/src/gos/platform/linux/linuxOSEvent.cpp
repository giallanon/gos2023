#ifdef GOS_PLATFORM__LINUX
#include "linuxOS.h"
#include <string.h>
#include <stdio.h>


//*******************************************************
bool platform::eventCreate (OSEvent *out_ev)
{
    out_ev->evfd = eventfd(0, EFD_NONBLOCK);
    if (out_ev->evfd == -1)
        return false;

    out_ev->h = epoll_create1(0);
    if (out_ev->h == -1)
    {
        ::close(out_ev->evfd);
        out_ev->evfd = -1;
        return false;
    }

    memset (&out_ev->eventInfo, 0, sizeof(out_ev->eventInfo));
    out_ev->eventInfo.events = EPOLLHUP | EPOLLERR | EPOLLIN | EPOLLET;
    int ret = epoll_ctl(out_ev->h, EPOLL_CTL_ADD, out_ev->evfd, &out_ev->eventInfo);

    if (ret == 0)
        return true;

    ::close(out_ev->h);
    ::close(out_ev->evfd);
    return false;
}

//*******************************************************
void platform::eventDestroy (OSEvent &ev)
{
    ::close(ev.evfd);
    ::close(ev.h);
}


//*******************************************************
void platform::eventFire (const OSEvent &ev)
{
    uint64_t val = 1;
    while(1)
    {
        int err = write (ev.evfd, &val, sizeof(val));
        if (err > 0)
            return;

        /*if (err < 0)
        {
            //EAGAIN
            int myerrno = errno;
            printf ("%d\n", myerrno);
        }
        */
    }    
}

//*******************************************************
bool platform::eventWait (const OSEvent &ev, size_t timeoutMSec)
{
    /*struct epoll_event events;
    if (epoll_pwait(ev.h, &events, 1, timeoutMSec, NULL) <= 0)
        return false;
    return true;*/
    while(1)
    {
        struct epoll_event events;
        const int ret = epoll_pwait(ev.h, &events, 1, timeoutMSec, NULL);
        if (ret > 0)
            return true;

        if (errno == EINTR)
            continue;
        return false;
    }    
}


#endif //GOS_PLATFORM__LINUX