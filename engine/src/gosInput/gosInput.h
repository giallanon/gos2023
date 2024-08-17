#ifndef _gosInput_h_
#define _gosInput_h_
#include "gosInputEnumAndDefine.h"
#include "gosInputEvtList.h"
#include "gosInputController.h"

namespace gos
{
	namespace input
	{
		bool			init();
		void			deinit();

		void			pollEvents();
						//da chiamare periodicamente, tipicamente nel main loop, per recuperare gli eventi di input

		const char* 	enumToString (input::eType e);
		const char* 	enumToString (input::eOrigin e);
		const char* 	enumToString (input::eAxle e);

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
		const EvtList*	window_getEventList (const GOSWinHandle &handle);

		/************************************************************************************************************
		 *
		 * gestione eventi di input "low level"
		 *
		 */
        EventID    		event_makeID (input::eOrigin origin, u16 btnId, eButtonStatus status, const sButtonModifier &modifier);
		EventID    		event_makeID (input::eOrigin origin, eAxle axle, i16 pos);
		eOrigin 		event_getOrigin (const EventID &id);
		eType			event_getType (const EventID &id);
		bool			event_toButtonEvent (const EventID &id, sBtnEvent *out);
		bool			event_toAxleEvent (const EventID &id, sAxleEvent *out);
		void			debug_event_printInfo (const EventID &id);
		void			event_getEventName (const EventID &id, char *out, u32 sizeof_out); 

	} //namespace input
} //namespace gos
#endif //_gosInput_h_