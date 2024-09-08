#ifndef _gosInputContext_h_
#define _gosInputContext_h_
#include "gosInputEnumAndDefine.h"
#include "gosInputEvtList.h"
#include "../gos/string/gosStringList.h"
#include "../gos/gosFastArray.h"
#include "../gos/string/gosCompileTimeHashedString.h"
#include "../gos/string/gosUniqueStringList.h"

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
								Context ()									{ priv_init("noname"); }
								Context (const char *contextName)			{ priv_init("contextName"); }
								~Context();

			void 				setContextName (const char *contextName);

								/**
								 * @brief action_add() 
								 * Aggiunge [actionName] all'elenco di [actionName] esistenti in questo Context.
								 * 
								 * @return false se [actionName] esiste gia'
								 */
			Context&			action_add (const char *actionName);

								/**
								 * @brief data una [actionName] preventivamente inserita tramite action_add(), la
								 * binda ad uno specifico evento di input.
								 * 
								 * @return false solo nel caso in cui [actionName] non esiste, ovvero se non e' stata registrata
								 * tramite action_add()
								 */
			bool 				action_bindToBtn (const char *actionName, input::eOrigin origin, u16 btnId, eButtonStatus status, const sButtonModifier &modifier = sButtonModifier());
			bool				action_bindToAxleABS (const char *actionName, input::eOrigin origin, input::eAxle axle);
			bool 				action_bindToAxleREL (const char *actionName, input::eOrigin origin, input::eAxle axle, input::eAxleDirection dir);

								/**
								 * @brief resolveEvent
								 * 
								 * @return 0 oppure la ActionID associata all'evento [eventID]
								 */
			u32					resolveEvent (const input::EventID &eventID, i16 *out_value) const;


			const char*			getName() const 					{ return name;}
			u32					action_getNum() const				{ return actionList.getNElem(); }
			const sAction*		action_getByIndex (u32 i)			{ return &actionList(i); }
			void 				logAllMappedInput() const;

		private:
			void 				priv_init (const char *name);
			u32 				priv_makeActionID (const char *actionName) const;
			bool 				priv_addBind (FastArray<sMappedAction> &dst, const EventID &eventID, u32 actionID);
			const sAction*		priv_action_existsByID (u32 actionID) const;
			bool				priv_action_bindToBtn (u32 actionID, input::eOrigin origin, u16 btnId, eButtonStatus status, const sButtonModifier &modifier = sButtonModifier());
			bool 				priv_action_bindToAxleABS (u32 actionID, input::eOrigin origin, input::eAxle axle);
			bool 				priv_action_bindToAxleREL (u32 actionID, input::eOrigin origin, input::eAxle axle, input::eAxleDirection dir);
			bool 				priv_getAllMappedInputEvent (u32 actionID, FastArray<EventID> &out) const ;
 			bool 				priv_isBound (const FastArray<sMappedAction> &list, u32 actionID, FastArray<EventID> &out) const ;

		private:
			char 						name[32];
			UniqueStringList			actionNameList;
			FastArray<sAction>			actionList;
			FastArray<sMappedAction>	btnEventList;
			FastArray<sMappedAction>	axleEventList;


		friend class Module;
		}; //Context

	} //namespace input
} //namespace gos
#endif //_gosInputContext_h_