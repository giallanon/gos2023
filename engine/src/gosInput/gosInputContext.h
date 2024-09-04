#ifndef _gosInputContext_h_
#define _gosInputContext_h_
#include "gosInputEnumAndDefine.h"
#include "gosInputEvtList.h"
#include "../gos/string/gosStringList.h"
#include "../gos/gosFastArray.h"
#include "../gos/string/gosCompileTimeHashedString.h"

namespace gos
{
	namespace input
	{
		class Module; //fwd decl


		/*********************************
		 * Context
		 */
		class Context
		{
		public:
								~Context();


								//action_add() 
								//Aggiunge [action] al [context] indicato.
								//Se [action] esiste gia' nel [context] indicato, ritorna 0
								//altrimenti ritorna l'ActionID univoco per la coppia context.action
			u32					action_add (const char *actionName);
			bool 				action_bindToBtn (const char *actionName, input::eOrigin origin, u16 btnId, eButtonStatus status, const sButtonModifier &modifier = sButtonModifier());
			bool				action_bindToAxleABS (const char *actionName, input::eOrigin origin, input::eAxle axle);
			bool 				action_bindToAxleREL (const char *actionName, input::eOrigin origin, input::eAxle axle, input::eAxleDirection dir);

								//resolveEvent
								//ritorna la ActionID associata all'evento [eventID] oppure 0 se l'evento non e'
								//bindato ad alcuna action
			u32					resolveEvent (const input::EventID &eventID, i16 *out_value) const;


			const char*			getName() const 					{ return name;}

			u32					action_getNum() const				{ return actionList.getNElem(); }
			const sAction*		action_getByIndex (u32 i)			{ return &actionList(i); }
			void 				logAllMappedInput() const;

		private:
			static constexpr u8 MAX_NAME_SIZE = 32;

		private:
								Context (gos::Allocator *allocator, const char *contextName);

			u32 				priv_makeActionID (const char *actionName) const;
			bool 				priv_addBind (FastArray<sMappedAction> &dst, const EventID &eventID, u32 actionID);
			const sAction*		priv_action_existsByID (u32 actionID) const;
			bool				priv_action_bindToBtn (u32 actionID, input::eOrigin origin, u16 btnId, eButtonStatus status, const sButtonModifier &modifier = sButtonModifier());
			bool 				priv_action_bindToAxleABS (u32 actionID, input::eOrigin origin, input::eAxle axle);
			bool 				priv_action_bindToAxleREL (u32 actionID, input::eOrigin origin, input::eAxle axle, input::eAxleDirection dir);
			bool 				priv_getAllMappedInputEvent (u32 actionID, FastArray<EventID> &out) const ;
 			bool 				priv_isBound (const FastArray<sMappedAction> &list, u32 actionID, FastArray<EventID> &out) const ;

		private:
			char 						name[MAX_NAME_SIZE];
			FastArray<sAction>			actionList;
			FastArray<sMappedAction>	btnEventList;
			FastArray<sMappedAction>	axleEventList;


		friend class Module;
		}; //Context

	} //namespace input
} //namespace gos
#endif //_gosInputContext_h_