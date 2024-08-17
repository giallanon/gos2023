#ifndef _gosInputController_h_
#define _gosInputController_h_
#include "gosInputEnumAndDefine.h"
#include "gosInputEvtList.h"
#include "../gos/gosFastArray.h"

namespace gos
{
	namespace input
	{
		/**************************************************
		 * Controller
		 * 
		 * Prende in input una lista di eventi di input low level (solitamente recuperata da window_getEventList())
		 * e li trasforma in "action"
		 * 
		 * Una action e' un intero 32bit che nasce come CRC di una stringa
		 */
		class Controller
		{
		public:
						Controller ();
						~Controller();

			void		bindBtnPress 	(const char *action, input::eOrigin origin, u16 btnId, const sButtonModifier &modifier = sButtonModifier());
			void		bindBtnRelease 	(const char *action, input::eOrigin origin, u16 btnId, const sButtonModifier &modifier = sButtonModifier());

			void		beginParse (const EvtList *evtList);
			u32			nextEvent();

		private:
			struct sEvent
			{
				EventID		eventID;
				u32			actionID;
			};

		private:
			void		priv_addEvent (FastArray<sEvent> &dst, const EventID &eventID, const char *action);
			u32			priv_resolveEvent (const FastArray<sEvent> &list, const EventID &eventID) const;

		private:
			FastArray<sEvent>	btnEventList;
			FastArray<sEvent>	axleEventList;
			
			const EvtList 	*evtList;
			EvtList::Iter	iter;
		};
		
	} //namespace input
} //namespace gos
#endif //_gosInputController_h_