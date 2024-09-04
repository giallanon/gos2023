#ifndef _gosInputEvtList_h_
#define _gosInputEvtList_h_
#include "gosInputEnumAndDefine.h"

namespace gos
{
	namespace input
	{
		class Context; //fwd

		/**
		 * @brief EvtList
		 */
		class EvtList
		{
		public:
			struct Iter
			{
				u32 i;
				u32 n;
			};

		public:
					EvtList (u32 numStartingElem);
					~EvtList();

			void	reset();
			void	addButtonEvt (input::eOrigin origin, u16 buttonId, eButtonStatus status, const sButtonModifier &modifier);
			void	addAxleAbsEvt (input::eOrigin origin, input::eAxle axle, i16 pos);
			void	addAxleRelEvt (input::eOrigin origin, input::eAxle axle, input::eAxleDirection dir, u16 strength);

			void	toStart (Iter &iter) const;
			bool	next (Iter &iter, input::EventID *out) const; 

		private:
			void	priv_doAddEvent (const EventID &eventID);
		
		private:
			EventID		*list;
			u32			nMaxElem;
			u32			nElem;
		};
		
	} //namespace input
} //namespace gos
#endif //_gosInputEvtList_h_