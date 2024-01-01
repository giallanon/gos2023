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

		bool			cleanAll();

		bool            addSocket (const gos::Socket &sok, void *userParam = NULL) 			{ sRecord *s = priv_addSocket(sok); if (s) s->userParam.asPtr = userParam; return (s != NULL); }
		bool            addSocket (const gos::Socket &sok, u32 userParam) 						{ sRecord *s = priv_addSocket(sok); if (s) s->userParam.asU32 = userParam; return (s != NULL); }
		void            removeSocket (const gos::Socket &sok);

		bool            addEvent (const gos::Event &evt, void *userParam = NULL) 		{ sRecord *s = priv_addEvent(evt); if (s) s->userParam.asPtr = userParam; return (s != NULL); }
		bool            addEvent (const gos::Event &evt, u32 userParam) 					{ sRecord *s = priv_addEvent(evt); if (s) s->userParam.asU32 = userParam; return (s != NULL); }
		void            removeEvent (const gos::Event &event);

		bool            addMsgQ (const HThreadMsgR &hRead, void *userParam = NULL) 		{ sRecord *s = priv_addMsgQ(hRead); if (s) s->userParam.asPtr = userParam; return (s != NULL); }
		bool            addMsgQ (const HThreadMsgR &hRead, u32 userParam) 				{ sRecord *s = priv_addMsgQ(hRead); if (s) s->userParam.asU32 = userParam; return (s != NULL); }
		void            removeMsgQ (const HThreadMsgR &hRead);

		u8              wait (u32 timeoutMSec);

		eWaitEventOrigin getEventOrigin(u8 iEvent) const;

		void*           getEventUserParamAsPtr(u8 iEvent) const;
		u32             getEventUserParamAsU32(u8 iEvent) const;

		gos::Socket&	getEventSrcAsSocket(u8 iEvent) const;

		gos::Event&		getEventSrcAsEvent(u8 iEvent) const;

		HThreadMsgR&	getEventSrcAsMsgQ(u8 iEvent) const;

	private:
		static const u8	MAX_EVENTS_RETURNED = 32;

		struct sIfSocket
		{
			gos::Socket	sok;
			HANDLE		hEventNotify;
		};

		struct sIfEvent
		{
			gos::Event		evt;
		};

		struct sIfMsgQ
		{
			HThreadMsgR		hRead;
			gos::Event		evt;
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
		sRecord*        priv_addEvent (const gos::Event &evt);
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


