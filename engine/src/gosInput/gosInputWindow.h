#ifndef _gosInputWindow_h_
#define _gosInputWindow_h_
#include "gosInputEnumAndDefine.h"
#include "gosInputEvtList.h"

namespace gos
{
	namespace input
	{
		class Context; //fwd

		/**************************************************
		 * Window
		 */
		struct Window
		{
		public:
							Window (GLFWwindow *glfwHandle);
							~Window();

			void 					resolveEvents_begin (const Context *ctx);
			u32 					resolveEvents_nextActionID (i16 *out_value);
			const MouseStatus&		resolveEvents_getMouse() const																{ return resolving.mouseStatus; }
			const sButtonModifier&	resolveEvents_getBtnModifier() const														{ return resolving.btnModifier; }


			void			addButtonEvt (input::eOrigin origin, int buttonId, eButtonStatus status)							{ curEvtList->addButtonEvt (origin, buttonId, status, curBtnModifier); }
			void			addAxleAbsEvt (input::eOrigin origin, input::eAxle axle, i16 pos)									{ curEvtList->addAxleAbsEvt (origin, axle, pos); }
			void			addAxleRelEvt (input::eOrigin origin, input::eAxle axle, input::eAxleDirection dir, u16 strength)	{ curEvtList->addAxleRelEvt (origin, axle, dir, strength); }

			void			setMouseMode (eMouseMode mode);
			void			toggleMouseMode()														{ if (mouseMode == eMouseMode::absolute) setMouseMode(eMouseMode::relative); else setMouseMode(eMouseMode::absolute); }
			eMouseMode		getMouseMode() const	 												{ return mouseMode; }

			GLFWwindow*		getGLFWHandle() const 													{ return glfwHandle; }
			
            void            storeCurrentPosAndSize()
                            {
								glfwGetWindowPos (glfwHandle, &storedX, &storedY);
								glfwGetWindowSize (glfwHandle, &storedW, &storedH);
                            }

		public:
            void        	*userpt;
			i32				lastMouseX;
			i32				lastMouseY;
			sButtonModifier	curBtnModifier;
            int 			storedX;
            int 			storedY;
            int 			storedW;
            int 			storedH;

		private:
			struct sResolving
			{
				const EvtList 	*list;
				EvtList::Iter 	iter;
				const Context 	*ctx;
				MouseStatus		mouseStatus;
				sButtonModifier	btnModifier;
			};

		private:
			const EvtList*	priv_swapEvtList();

		private:
            GLFWwindow  	*glfwHandle;
			EvtList			*curEvtList;
			eMouseMode		mouseMode;
			EvtList			evtList1;
			EvtList			evtList2;
			sResolving		resolving;
		};
		
	} //namespace input
} //namespace gos
#endif //_gosInputWindow_h_