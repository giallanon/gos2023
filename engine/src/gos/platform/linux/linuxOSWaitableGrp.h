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

        bool                clean_all();

        bool                socket__add (const gos::Socket &sok, void *userParam=NULL)				{ sRecord *s=priv_addSocket(sok); if(s) s->userParam.asPtr=userParam; return (s != NULL); }
        bool                socket__add (const gos::Socket &sok, u32 userParam)						{ sRecord *s=priv_addSocket(sok); if(s) s->userParam.asU32=userParam; return (s != NULL); }
        void                socket__remove (const gos::Socket &sok)									{ priv_onRemove (sok.osSok.socketID); }

        bool                signal__add (const gos::Signal &evt, void *userParam=NULL)					{ sRecord *s=priv_addEvent(evt); if(s) s->userParam.asPtr=userParam; return (s != NULL); }
        bool                signal__add (const gos::Signal &evt, u32 userParam)						{ sRecord *s=priv_addEvent(evt); if(s) s->userParam.asU32=userParam; return (s != NULL); }
        void                signal__remove (const gos::Signal &event)									{ priv_onRemove (event.osEvt.evfd); }

        bool                msgQ__add (const HThreadMsgR &hRead, void *userParam = NULL)				{ sRecord *s = priv_addMsgQ(hRead); if (s) s->userParam.asPtr = userParam; return (s != NULL); }
        bool                msgQ__add (const HThreadMsgR &hRead, u32 userParam)						{ sRecord *s = priv_addMsgQ(hRead); if (s) s->userParam.asU32 = userParam; return (s != NULL); }
        void                msgQ__remove (const HThreadMsgR &hRead);

		bool				fsWatcher__add (gos::FSWatcher *fsw, void *userParam=NULL)				{ sRecord *s=priv_add_FSWatcher(fsw); if(s) s->userParam.asPtr=userParam; return (s != NULL); }
        bool                fsWatcher__add (gos::FSWatcher *fsw, u32 userParam)						{ sRecord *s=priv_add_FSWatcher(fsw); if(s) s->userParam.asU32=userParam; return (s != NULL); }
		void            	fsWatcher__remove(gos::FSWatcher *fsw);

        u8                  wait (u32 timeoutMSec);

        eWaitEventOrigin    event__get_origin (u8 iEvent) const;

        void*               event__get_user_param_as_ptr (u8 iEvent) const;
        u32                 event__get_user_param_as_u32 (u8 iEvent) const;

        gos::Socket        	event__get_socket_handle (u8 iEvent) const;

        gos::Signal         	event__get_signal_handle (u8 iEvent) const;

        HThreadMsgR	    	event__get_msgQ_handle(u8 iEvent) const;

		gos::FSWatcher*		event__get_fsWatcher_handle(u8 iEvent) const;

    private:
        static const u8 WHATIS_SOCKET = 1;
        static const u8 WHATIS_EVENT = 2;

        struct sIfMsgQ
        {
            HThreadMsgR     hMsgQRead;
            gos::Signal		event;
        };

        struct sIfFSWatcher
        {
            gos::FSWatcher	*fsWatcher;
            int 			fd;
        };		

        union sOrigin
        {
            gos::Socket		socket;
            gos::Signal		event;
            sIfMsgQ         ifMsgQ;
			sIfFSWatcher	isFSW;
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
        sRecord*        priv_addEvent (const gos::Signal &evt);
        sRecord*        priv_addMsgQ (const HThreadMsgR &hRead);
		sRecord*        priv_add_FSWatcher (gos::FSWatcher *fsw);

    private:
        sRecord         *base;
        int             hfd;
        epoll_event     events[MAX_EVENTS_HANDLE_PER_CALL];
        u8              nEventsReady;
    };

} //namespace platform
#endif // _linuxOSWaitableGrp_h_

#endif //GOS_PLATFORM__LINUX
