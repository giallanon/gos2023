#ifdef GOS_PLATFORM__WINDOWS
#ifndef _winOSWaitableGrp_h_
#define _winOSWaitableGrp_h_
#include "winOS.h"
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

		bool			clean_all();

		bool            socket__add (const gos::Socket &sok, void *userParam = NULL) 		{ sRecord *s = priv_addSocket(sok); if (s) s->userParam.asPtr = userParam; return (s != NULL); }
		bool            socket__add (const gos::Socket &sok, u32 userParam)					{ sRecord *s = priv_addSocket(sok); if (s) s->userParam.asU32 = userParam; return (s != NULL); }
		void            socket__remove (const gos::Socket &sok);

		bool            signal__add (const gos::Signal &evt, void *userParam = NULL) 			{ sRecord *s = priv_addEvent(evt); if (s) s->userParam.asPtr = userParam; return (s != NULL); }
		bool            signal__add (const gos::Signal &evt, u32 userParam) 					{ sRecord *s = priv_addEvent(evt); if (s) s->userParam.asU32 = userParam; return (s != NULL); }
		void            signal__remove (const gos::Signal &event);

		bool            msgQ__add (const HThreadMsgR &hRead, void *userParam = NULL) 		{ sRecord *s = priv_addMsgQ(hRead); if (s) s->userParam.asPtr = userParam; return (s != NULL); }
		bool            msgQ__add (const HThreadMsgR &hRead, u32 userParam) 					{ sRecord *s = priv_addMsgQ(hRead); if (s) s->userParam.asU32 = userParam; return (s != NULL); }
		void            msgQ__remove (const HThreadMsgR &hRead);

		bool			fsWatcher__add (gos::FSWatcher *fsw, void *userParam=NULL)			{ sRecord *s=priv_add_FSWatcher(fsw); if(s) s->userParam.asPtr=userParam; return (s != NULL); }
        bool            fsWatcher__add (gos::FSWatcher *fsw, u32 userParam)					{ sRecord *s=priv_add_FSWatcher(fsw); if(s) s->userParam.asU32=userParam; return (s != NULL); }
		void            fsWatcher__remove(gos::FSWatcher *fsw);

		u8              wait (u32 timeoutMSec);

		eWaitEventOrigin event__get_origin(u8 iEvent) const;

		void*           event__get_user_param_as_ptr(u8 iEvent) const;
		u32             event__get_user_param_as_u32(u8 iEvent) const;

		gos::Socket		event__get_socket_handle(u8 iEvent) const;

		gos::Signal		event__get_signal_handle(u8 iEvent) const;

		HThreadMsgR		event__get_msgQ_handle(u8 iEvent) const;

		gos::FSWatcher*	event__get_fsWatcher_handle(u8 iEvent) const;

	private:
		static const u8	MAX_EVENTS_RETURNED = 32;

		struct sIfSocket
		{
			gos::Socket	sok;
			HANDLE		hEventNotify;
		};

		struct sIfEvent
		{
			gos::Signal		evt;
		};

		struct sIfMsgQ
		{
			HThreadMsgR		hRead;
			gos::Signal		evt;
		};

		union sOrigin
		{
			sIfSocket		socket;
			sIfEvent		event;
			sIfMsgQ			msgQ;
		};

		union uUserParam
		{
			void    *asPtr;
			u32     asU32;
		};

		struct sRecord
		{
			sOrigin			origin;
			eWaitEventOrigin originType;
			uUserParam		userParam;
			sRecord			*next;
		};

	private:
		sRecord*        priv_newRecord();
		sRecord*        priv_addSocket (const gos::Socket &sok);
		sRecord*        priv_addEvent (const gos::Signal &evt);
		sRecord*        priv_addMsgQ (const HThreadMsgR &hRead);
		u8              priv_wait (u32 timeoutMSec);

	private:
		sRecord         *base;
		HANDLE			eventsHandle[MAX_EVENTS_HANDLE_PER_CALL];
		u8				nEventsReady;
		sRecord*		generatedEventList[MAX_EVENTS_RETURNED];

		u8				debug_bWaiting;
	};

}//namespace platform


#endif // _winOSWaitableGrp_h_
#endif //GOS_PLATFORM__WINDOWS


