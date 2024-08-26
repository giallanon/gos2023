#ifndef _gosInputMapper_h_
#define _gosInputMapper_h_
#include "gosInputEnumAndDefine.h"
#include "gosInputEvtList.h"
#include "../gos/string/gosStringList.h"
#include "../gos/gosFastArray.h"
#include "../gos/string/gosCompileTimeHashedString.h"

namespace gos
{
	namespace input
	{
		/*********************************
		 */
		class Mapper
		{
		private:
			static const u8 CONTEX_MAX_NAME_SIZE = 32;

		private:
			struct sAction
			{
				u32	actionID;
				u32 offsetToActionName;
			};

			struct sMappedAction
			{
				EventID		eventID;
				u32			actionID;
			};				

		public:
			/*********************************
			 * Context
			 */
			class Context
			{
			public:
									Context (gos::Allocator *allocator, const char *contextName);
									~Context();

				void 				action_add (const sAction &action);
				u32					action_getNum() const				{ return actionList.getNElem(); }
				const sAction*		action_getByIndex (u32 i)			{ return &actionList(i); }
				const sAction*		action_exists (u32 actionID) const;
				bool				action_isMapped (u32 actionID, FastArray<EventID> &out) const;
				
				bool				action_bindToBtn (u32 actionID, input::eOrigin origin, u16 btnId, eButtonStatus status, const sButtonModifier &modifier = sButtonModifier());
				bool 				action_bindToAxleABS (u32 actionID, input::eOrigin origin, input::eAxle axle);
				bool 				action_bindToAxleREL (u32 actionID, input::eOrigin origin, input::eAxle axle, input::eAxleDirection dir);
				

									//resolveEvent
									//ritorna la ActionID associata all'evento [eventID] oppure 0 se l'evento non e'
									//bindato ad alcuna action
				u32					resolveEvent (const input::EventID &eventID, i16 *out_value) const;

				const char*			getName() const 					{ return name;}
				u32					getUID() const						{ return UID; }

			private:
				bool 				priv_addBind (FastArray<sMappedAction> &dst, const EventID &eventID, u32 actionID);
				bool 				priv_isBound (const FastArray<sMappedAction> &list, u32 actionID, FastArray<EventID> &out) const; 

			private:
				char 				name[CONTEX_MAX_NAME_SIZE];
				u32					UID;
				FastArray<sAction>	actionList;
				FastArray<sMappedAction>	btnEventList;
				FastArray<sMappedAction>	axleEventList;
			}; //Context

		public:
			struct MouseStatus
			{
				i16 	x;
				i16 	y;
				u8 		btnPressed[16];

				void 	reset()							{ x=y=0; memset(btnPressed,0,sizeof(btnPressed)); }

				bool 	isLMBPressed() const 			{ return (btnPressed[0] != 0); }
				bool 	isRMBPressed() const 			{ return (btnPressed[1] != 0); }
				bool 	isMMBPressed() const 			{ return (btnPressed[2] != 0); }
				bool 	isPressed (u8 btnNum) const 	{ assert(btnNum<16); return (btnPressed[btnNum]!=0); }
			};

		public:
							Mapper();
							~Mapper();

							//action_add() 
							//Aggiunge [action] al [context] indicato.
							//Se [context] non esiste, lo crea
							//Se [action] esiste gia' nel [context] indicato, ritorna 0
							//altrimenti ritorna l'ActionID univoco per la coppia context.action
			u32				action_add (const char *contextName, const char *actionName);
			bool			action_existsByActionID (u32 actionID) const;
			const char*		action_getNameByActionID (u32 actionID) const;
			bool			action_bindToBtn 	(const char *contextName, const char *actionName, input::eOrigin origin, u16 btnId, eButtonStatus status, const sButtonModifier &modifier = sButtonModifier());
			bool			action_bindToAxleABS (const char *contextName, const char *actionName, input::eOrigin origin, input::eAxle axle);
			bool 			action_bindToAxleREL (const char *contextName, const char *actionName, input::eOrigin origin, input::eAxle axle, input::eAxleDirection dir);

			//============== gestione dei context
			u32				context_getNum () const											{ return contextList.getNElem(); }
			const char*		context_getName (u32 iCtx) const								{ assert(iCtx<context_getNum()); return contextList(iCtx)->getName(); }
			u32				context_getNumAction (u32 iCtx) const							{ assert(iCtx<context_getNum()); return contextList(iCtx)->action_getNum(); }
			const char*		context_getActionNameByIndex (u32 iCtx, u32 iAction) const;
			u32				context_getActionIDByIndex (u32 iCtx, u32 iAction) const;

			//============== resolve dell'input
			void			resolve_begin (const EvtList *evtList);
			
							//resolve_getNextActionID()
							//ritorna una l'[actionID] oppure 0
							//Se ritorna una valida action ID, allora
							// in caso di action mappata su un BTN:
							//	[out_value] vale 0 o 1 a seconda che il btn sia RELEASED o PRESSE
							// in caso di action mappata su un AXLE:
							//	[out_value] riporta il valore dell'asse
							//
							// Ad ogni chiamata di resolve_getNextActionID() lo stato interno
							// del mouse viene aggiornato in modo che una chiamata a resolve_getMouse()
							// ritorna lo stato del mouse in quel preciso momento, avendo elaborato
							// solo gli input macinati fino ad ora
			u32				resolve_getNextActionID (i16 *out_value);
			const MouseStatus&			resolve_getMouse() const						{ return mouseStatus; }
			const sButtonModifier&		resolve_getBtnModifier() const 					{ return btnModifier; }

			//============== utils
			void 			logAllMappedInput() const;

		private:
			u32				priv_makeActionID (const char *contextName, const char *actionName, char *out_fullName, u32 sizeof_fullName) const;
			u32				priv_context_find (const char *contextName) const;
			u32				priv_context_find (u32 contextUID) const;
			Context*		priv_context_add (const char *contextName);
			bool			priv_findByActionID (u32 actionID, u32 *out_iCtx, const sAction **out_actionInfo) const;

		private:
			StringList			actionNameList;
			FastArray<Context*>	contextList;
			const EvtList 		*evtList;
			EvtList::Iter		iter;			
			MouseStatus			mouseStatus;
			sButtonModifier		btnModifier;
		};


	} //namespace input
} //namespace gos
#endif //_gosInputMapper_h_