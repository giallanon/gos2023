#ifdef GOS_PLATFORM__LINUX
#ifndef _linuxOSWaitableGrp_h_
#define _linuxOSWaitableGrp_h_
#include <sys/eventfd.h>
#include <sys/epoll.h>
#include <unistd.h>
#include "linuxOS.h"
#include "../../gosWaitableGrpInterface.h"


namespace platform
{
	/********************************************************************
	 * OSWaitableGrp
	 *
	 * Implementazione dell'interfaccia gos::WaitableGrpInterface
	 */
	class OSWaitableGrp : public gos::WaitableGrpInterface
    {
    public:
                            OSWaitableGrp();
                            ~OSWaitableGrp();

        bool                cleanAll();

        bool                addSocket (const gos::Socket &sok, void *userParam=NULL)                        { sRecord *s=priv_addSocket(sok); if(s) s->userParam.asPtr=userParam; return (s != NULL); }
        bool                addSocket (const gos::Socket &sok, u32 userParam)                               { sRecord *s=priv_addSocket(sok); if(s) s->userParam.asU32=userParam; return (s != NULL); }
        void                removeSocket (const gos::Socket &sok)                                           { priv_onRemove (sok.osSok.socketID); }

        bool                addEvent (const gos::Event &evt, void *userParam=NULL)                          { sRecord *s=priv_addEvent(evt); if(s) s->userParam.asPtr=userParam; return (s != NULL); }
        bool                addEvent (const gos::Event &evt, u32 userParam)                                 { sRecord *s=priv_addEvent(evt); if(s) s->userParam.asU32=userParam; return (s != NULL); }
        void                removeEvent (const gos::Event &event)                                           { priv_onRemove (event.osEvt.evfd); }

        bool                addMsgQ (const HThreadMsgR &hRead, void *userParam = NULL)					    { sRecord *s = priv_addMsgQ(hRead); if (s) s->userParam.asPtr = userParam; return (s != NULL); }
        bool                addMsgQ (const HThreadMsgR &hRead, u32 userParam)							    { sRecord *s = priv_addMsgQ(hRead); if (s) s->userParam.asU32 = userParam; return (s != NULL); }
        void                removeMsgQ (const HThreadMsgR &hRead);

        u8                  wait (u32 timeoutMSec);

        eWaitEventOrigin    getEventOrigin (u8 iEvent) const;

        void*               getEventUserParamAsPtr (u8 iEvent) const;
        u32                 getEventUserParamAsU32 (u8 iEvent) const;

        gos::Socket&        getEventSrcAsSocket (u8 iEvent) const;

        gos::Event&         getEventSrcAsEvent (u8 iEvent) const;

        HThreadMsgR&	    getEventSrcAsMsgQ(u8 iEvent) const;

    private:
        static const u8 WHATIS_SOCKET = 1;
        static const u8 WHATIS_EVENT = 2;

        struct sIfMsgQ
        {
            HThreadMsgR     hMsgQRead;
            gos::Event		event;
        };

        union sOrigin
        {
            gos::Socket		socket;
            gos::Event		event;
            sIfMsgQ         ifMsgQ;
        };

        union uUserParam
        {
            void    *asPtr;
            u32     asU32;
        };

        struct sRecord
        {
            epoll_event     eventInfo;
            sOrigin         origin;
            eWaitEventOrigin originType;
            uUserParam      userParam;
            sRecord         *next;
        };

    private:
        sRecord*        priv_newRecord (u32 flags);
        void            priv_findAndRemoveRecordByFD (int fd);
        void            priv_onRemove (int fd);
        int             priv_getFd (const sRecord *s) const;
        sRecord*        priv_addSocket (const gos::Socket &sok);
        sRecord*        priv_addEvent (const gos::Event &evt);
        sRecord*        priv_addMsgQ (const HThreadMsgR &hRead);

    private:
        sRecord         *base;
        int             hfd;
        epoll_event     events[MAX_EVENTS_HANDLE_PER_CALL];
        u8              nEventsReady;
    };

} //namespace platform
#endif // _linuxOSWaitableGrp_h_

#endif //GOS_PLATFORM__LINUX
