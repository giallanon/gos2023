#ifndef _gosInput_h_
#define _gosInput_h_
#include "gosInputEnumAndDefine.h"
#include "gosInputEvtList.h"
#include "gosInputContext.h"
#include "gosInputResolvedEvtList.h"

namespace gos
{
	namespace input
	{
		bool			init();
		void			deinit();
		Allocator* 		getAllocator();

		const char* 	enumToString (input::eType e);
		const char* 	enumToString (input::eOrigin e);
		const char* 	enumToString (input::eAxle e);
		const char* 	enumToString (input::eAxleDirection e);


		/**
		 * @brief pollEvents()
		 * da chiamare periodicamente, tipicamente nel main loop, per recuperare gli eventi di input
		 */
		void			pollEvents();

		/**
		 * @brief resolveEvents()
		 * dopo una chiamata pollEvent, le [windows] contengono una lista degli eventi di input.
		 * Utilizzare resolveEvents() per recuperare tale lista e tradurla in [actionID]
		 * Questa fn valorizza [out] che poi diventa l'interfaccia principale per il recupero delle actionID
		 * scatenate dagli input recuperati dalla finestra
		 * 
		 * Gli input della finestra vengono confrontati con il [context] per produrre delle [actionID]
		 */
		void 			resolveEvents (const GOSWinHandle &handle, const Context *ctx, ResolvedEvtList *out);


		/************************************************************************************************************
		 *
		 * Windows
		 *
		 */
		bool			window_create (int w, int h, const char *title, GOSWinHandle *out_handle);
		void			window_destroy (GOSWinHandle &handle);
		void			window_setTitle (const GOSWinHandle &handle, const char *title);
		void			window_setUserPointer (const GOSWinHandle &handle, void *userPt);
		bool			window_getUserPointer (const GOSWinHandle &handle, void **out_userPt);
		bool			window_shouldClose (const GOSWinHandle &handle);
		void			window_show (const GOSWinHandle &handle);
		void			window_hide (const GOSWinHandle &handle);
		void			window_setPos (const GOSWinHandle &handle, int x, int y);
		void			window_getPos (const GOSWinHandle &handle, int *out_x, int *out_y);
		void			window_setSize (const GOSWinHandle &handle, int dimX, int dimY);
		void			window_getSize (const GOSWinHandle &handle, int *out_dimX, int *out_dimY);
		bool			window_getGLF (const GOSWinHandle &handle, GLFWwindow **out_GLFWindow);
		void			window_setMouseMode (const GOSWinHandle &handle, eMouseMode mode);
		void			window_toggleMouseMode (const GOSWinHandle &handle);
		void			window_toggleFullscreen(const GOSWinHandle &handle);

		/************************************************************************************************************
		 *
		 * gestione eventi di input "low level"
		 *
		 */
						//event_button_makeID()
						//Nel caso di "bottoni", un [event] e' identificato dall'id bottone, il suo stato di premuto/rilasciato
						//e gli eventuali [modifier] quali ALT, SHIFT e CTRL
        EventID    		event_button_makeID (input::eOrigin origin, u16 btnId, eButtonStatus status, const sButtonModifier &modifier);

						//event_axleAbs_makeID
						//Un eventID nel caso di [axle] assoluto, riporta l'asse stesso e la posizione assoluta dell'asse
		EventID    		event_axleAbs_makeID (input::eOrigin origin, eAxle axle, i16 pos);

						//event_axleRel_makeID
						//Un eventID nel caso di [axle] relativo, riporta l'asse, la direzione dell'asse (positivo/negativo) e la
						//"forza" con cui è stato mosso l'asse
		EventID    		event_axleRel_makeID (input::eOrigin origin, eAxle axle, eAxleDirection direction, u16 strength);
		
		eOrigin 		event_getOrigin (const EventID &eventID);
		eType			event_getType (const EventID &eventID);
		bool			event_toButtonEvent (const EventID &eventID, sBtnEvent *out);
		bool			event_toAxleAbsEvent (const EventID &eventID, sAxleAbsEvent *out);
		bool			event_toAxleRelEvent (const EventID &eventID, sAxleRelEvent *out);
		void			debug_event_printInfo (const EventID &eventID);
		
						//event_getEventName
						//Filla [out] con il nome "umano" della combinazione di tasti/assi (es: ALT + Q)
		void			event_getEventName (const EventID &eventID, char *out, u32 sizeof_out);

	} //namespace input
} //namespace gos
#endif //_gosInput_h_