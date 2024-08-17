#ifndef _gosInputWindow_h_
#define _gosInputWindow_h_
#include "gosInputEnumAndDefine.h"
#include "gosInputEvtList.h"

namespace gos
{
	namespace input
	{
		/**************************************************
		 * Window
		 */
		struct Window
		{
		public:
							Window (GLFWwindow *glfwHandle);
							~Window();

			void			addButtonPressed (input::eOrigin dev, int buttonId) 					{ curEvtList->addButtonPressed (dev, buttonId, curBtnModifier); }
			void			addButtonReleased (input::eOrigin dev, int buttonId) 					{ curEvtList->addButtonReleased (dev, buttonId, curBtnModifier); }
			void			addAxleEvt (input::eOrigin dev, input::eAxle axle, i16 pos)				{ curEvtList->addAxleEvt (dev, axle, pos); }

			void			setMouseMode (eMouseMode mode);
			void			toggleMouseMode()														{ if (mouseMode == eMouseMode::absolute) setMouseMode(eMouseMode::relative); else setMouseMode(eMouseMode::absolute); }
			eMouseMode		getMouseMode() const	 												{ return mouseMode; }
			GLFWwindow*		getGLFWHandle() const 													{ return glfwHandle; }
			const EvtList*	swapEvtList();

		public:
            void        	*userpt;
			i32				lastMouseX;
			i32				lastMouseY;
			sButtonModifier	curBtnModifier;
			

		private:
            GLFWwindow  	*glfwHandle;
			EvtList			*curEvtList;
			eMouseMode		mouseMode;
			EvtList			evtList1;
			EvtList			evtList2;
			
		};
		
	} //namespace input
} //namespace gos
#endif //_gosInputWindow_h_