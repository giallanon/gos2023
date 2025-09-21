#include "gosInputWindow.h"
#include "gosInput.h"
#include "gosInputContext.h"
#include "../gos/gos.h"

using namespace gos;
using namespace gos::input;

static void GOSInputWindow_KB_key_callback (GLFWwindow* window, int key, UNUSED_PARAM(int scancode), int action, int mods);
static void GOSInputWindow_mouse_movement_callback (GLFWwindow* window, double xpos, double ypos);
static void GOSInputWindow_mouse_button_callback (GLFWwindow* window, int button, int action, int mods);
static void GOSInputWindow_mouse_wheel_callback (GLFWwindow* window, double xoffset, double yoffset);
static void GOSInputWindow_close_callback (GLFWwindow* window);
void GOSInputWindow_resize_callback (GLFWwindow* window, int w, int h);

//*******************************
Window::Window (GLFWwindow *glfwHandleIN) : evtList1(1024), evtList2(1024)
{
	this->glfwHandle = glfwHandleIN;
	userpt = NULL;

    double dx, dy;
    glfwGetCursorPos (glfwHandle, &dx, &dy);
	lastMouseX = (i32)dx;
    lastMouseY = (i32)dy;
    mouseMode = eMouseMode::absolute;
	curEvtList = &evtList2;
	curBtnModifier.reset();
    
    callback_onResize_fn = NULL;
    callback_onResize_userPt = NULL;
    
    resolving.list = NULL;
    resolving.mouseStatus.reset();

	glfwSetWindowUserPointer (glfwHandle, this);
    glfwSetWindowCloseCallback (glfwHandle, GOSInputWindow_close_callback);
    glfwSetWindowSizeCallback (glfwHandle, GOSInputWindow_resize_callback);

    //gestione kb: vedi https://www.glfw.org/docs/latest/input.html
    glfwSetKeyCallback (glfwHandle, GOSInputWindow_KB_key_callback);
    //glfwSetCharCallback(glfwHandle, GLFW_kb_char_callback);

    //gestione mouse: vedi https://www.glfw.org/docs/latest/input.html#input_mouse
    glfwSetCursorPosCallback (glfwHandle, GOSInputWindow_mouse_movement_callback);
    glfwSetMouseButtonCallback (glfwHandle, GOSInputWindow_mouse_button_callback);
    glfwSetScrollCallback (glfwHandle, GOSInputWindow_mouse_wheel_callback);
}

//*******************************
Window::~Window()
{

}

//*******************************
void Window::setMouseMode (eMouseMode mode)
{
    if (eMouseMode::absolute == mode)
    {
        glfwSetInputMode (glfwHandle, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        mouseMode = eMouseMode::absolute;
    }
    else
    {
        glfwSetInputMode (glfwHandle, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        mouseMode = eMouseMode::relative;
    }
}

//************************************
const input::EvtList* Window::priv_swapEvtList()
{
	if (curEvtList == &evtList1)
	{
		curEvtList = &evtList2;
        evtList2.reset();
		return &evtList1;
	}

	curEvtList = &evtList1;
    evtList1.reset();
	return &evtList2;
}

//************************************
void Window::resolveEvents_begin (const Context *ctx)
{
    resolving.list = priv_swapEvtList();
    resolving.list->toStart (resolving.iter);
    resolving.ctx = ctx;
}

//************************************
u32 Window::resolveEvents_nextActionID (i16 *out_value)
{
	input::EventID eventID;
	while (resolving.list->next (resolving.iter, &eventID))
	{
        //tengo traccia dello stato attuale del mouse
        if (eOrigin::mouse == input::event_getOrigin(eventID))
        {
            switch (input::event_getType(eventID))
            {
            default:
                break;

            case eType::axleABS:
                {
                    sAxleAbsEvent info;
                    input::event_toAxleAbsEvent (eventID, &info);
                    if (eAxle::x == info.axle)
                        resolving.mouseStatus.x = info.pos;
                    else if (eAxle::y == info.axle)
                        resolving.mouseStatus.y = info.pos;
                }
                break;

            case eType::button:
                {
                    sBtnEvent info;
                    input::event_toButtonEvent (eventID, &info);
                    if (eButtonStatus::pressed == info.status)
                        resolving.mouseStatus.btnPressed[info.id] = 1;
                    else
                        resolving.mouseStatus.btnPressed[info.id] = 0;
                }
                break;
            }
        }
        if (eOrigin::keyboard == input::event_getOrigin(eventID))
        {
            //tengo traccia dello stato dei modifier (SHIFT, CTRL...)
            if (input::eType::button == input::event_getType(eventID))
            {
                sBtnEvent info;
                if (input::event_toButtonEvent (eventID, &info))
                    resolving.btnModifier = info.modifier;
            }
        }

        //risolvo rispetto al context
        const u32 actionID = resolving.ctx->resolveEvent (eventID, out_value);
        if (0 != actionID)
            return actionID;
	}

	return 0;
}

//************************************
void Window::_onEvent_resize (int w, int h)
{
    if (NULL == callback_onResize_fn)
        return;
    callback_onResize_fn (w, h, callback_onResize_userPt);
}

//************************************
void GOSInputWindow_close_callback (GLFWwindow* window)
{
    glfwSetWindowShouldClose (window, false);

    input::Window *win = reinterpret_cast<input::Window*> (glfwGetWindowUserPointer(window));
    win->addButtonEvt (input::eOrigin::window, GOS_BUTTON_WINDOW_CLOSE, eButtonStatus::pressed);
}

//************************************
void GOSInputWindow_resize_callback (GLFWwindow* window, int w, int h)
{
    input::Window *win = reinterpret_cast<input::Window*> (glfwGetWindowUserPointer(window));
    win->_onEvent_resize (w, h);
}



//************************************
void GOSInputWindow_mouse_movement_callback (GLFWwindow* window, double xpos, double ypos)
{
    input::Window *win = reinterpret_cast<input::Window*> (glfwGetWindowUserPointer(window));
	const i32 x = (i32)xpos;
	if (x != win->lastMouseX)
	{
        if (eMouseMode::absolute == win->getMouseMode())
        {
            win->addAxleAbsEvt (input::eOrigin::mouse, input::eAxle::x, x);
        }
        else
        {
            if (x > win->lastMouseX)
                win->addAxleRelEvt (input::eOrigin::mouse, input::eAxle::x, input::eAxleDirection::positive, x - win->lastMouseX);
            else
                win->addAxleRelEvt (input::eOrigin::mouse, input::eAxle::x, input::eAxleDirection::negative, win->lastMouseX - x);
        }
        win->lastMouseX = x;
	}

	const i32 y = (i32)ypos;
	if (y != win->lastMouseY)
	{
        if (eMouseMode::absolute == win->getMouseMode())
        {
            win->addAxleAbsEvt (input::eOrigin::mouse, input::eAxle::y, y);
        }
        else
        {
            if (y > win->lastMouseY)
                win->addAxleRelEvt (input::eOrigin::mouse, input::eAxle::y, input::eAxleDirection::positive, y - win->lastMouseY);
            else
                win->addAxleRelEvt (input::eOrigin::mouse, input::eAxle::y, input::eAxleDirection::negative, win->lastMouseY - y);
        }
		win->lastMouseY = y;
	}	
}

//************************************
void GOSInputWindow_mouse_wheel_callback (GLFWwindow* window, double xoffset, double yoffset)
{
    input::Window *win = reinterpret_cast<input::Window*> (glfwGetWindowUserPointer(window));

    if (yoffset<0)
        win->addAxleRelEvt (input::eOrigin::mouse, input::eAxle::z, input::eAxleDirection::negative, 1);
    else if (yoffset>0)
        win->addAxleRelEvt (input::eOrigin::mouse, input::eAxle::z, input::eAxleDirection::positive, 1);
}

//************************************
void GOSInputWindow_mouse_button_callback (GLFWwindow* window, int button, int action, int mods)
{
    assert (button <= 0xFFFF);
	input::Window *win = reinterpret_cast<input::Window*> (glfwGetWindowUserPointer(window));
    if (GLFW_PRESS == action)
        win->addButtonEvt (input::eOrigin::mouse, (u16)button, eButtonStatus::pressed);
    else
        win->addButtonEvt (input::eOrigin::mouse, (u16)button, eButtonStatus::released);
}


//************************************
void GOSInputWindow_KB_key_callback (GLFWwindow* window, int key, UNUSED_PARAM(int scancode), int action, int mods)
{
    assert (key <= 0xFFFF);
    input::Window *win = reinterpret_cast<input::Window*> (glfwGetWindowUserPointer(window));

	//aggiornamento stato dei btn speciali
	switch (key)
	{
		default: break;
		case GLFW_KEY_LEFT_SHIFT: win->curBtnModifier.set (input::eButtonModifier::LSHIFT, (action == GLFW_PRESS)); break;
		case GLFW_KEY_RIGHT_SHIFT: win->curBtnModifier.set (input::eButtonModifier::RSHIFT, (action == GLFW_PRESS)); break;
		case GLFW_KEY_LEFT_ALT: win->curBtnModifier.set (input::eButtonModifier::LALT, (action == GLFW_PRESS)); break;
		case GLFW_KEY_RIGHT_ALT: win->curBtnModifier.set (input::eButtonModifier::RALT, (action == GLFW_PRESS)); break;
		case GLFW_KEY_LEFT_CONTROL: win->curBtnModifier.set (input::eButtonModifier::LCTRL, (action == GLFW_PRESS)); break;
		case GLFW_KEY_RIGHT_CONTROL: win->curBtnModifier.set (input::eButtonModifier::RCTRL, (action == GLFW_PRESS)); break;
	}


    if (GLFW_RELEASE == action)
        win->addButtonEvt (input::eOrigin::keyboard, (u16)key, eButtonStatus::released);
    else if (GLFW_PRESS == action)
        win->addButtonEvt (input::eOrigin::keyboard, (u16)key, eButtonStatus::pressed);



    char kname[32];
#define MACRO_HELPME(glDEFINE_senza_GLFW)\
    case GLFW_##glDEFINE_senza_GLFW: sprintf_s (kname, sizeof(kname), #glDEFINE_senza_GLFW); break;

    switch (key)
    {
    default:
        {
            const char *key_name = glfwGetKeyName (key, scancode);
            if (NULL != key_name)
                strcpy_s (kname, sizeof(kname), key_name);
            else
                sprintf_s (kname, sizeof(kname), "??KEY_NONAME??"); 
        }
        break;

    case GLFW_KEY_ESCAPE:    sprintf_s (kname, sizeof(kname), "KEY_ESCAPE"); break;
    MACRO_HELPME(KEY_ENTER)
    MACRO_HELPME(KEY_TAB)
    MACRO_HELPME(KEY_BACKSPACE)
    MACRO_HELPME(KEY_INSERT)
    MACRO_HELPME(KEY_DELETE)
    MACRO_HELPME(KEY_RIGHT)
    MACRO_HELPME(KEY_LEFT)
    MACRO_HELPME(KEY_DOWN)
    MACRO_HELPME(KEY_UP)
    MACRO_HELPME(KEY_PAGE_UP)
    MACRO_HELPME(KEY_PAGE_DOWN)
    MACRO_HELPME(KEY_HOME)
    MACRO_HELPME(KEY_END)
    MACRO_HELPME(KEY_CAPS_LOCK)
    MACRO_HELPME(KEY_SCROLL_LOCK)
    MACRO_HELPME(KEY_NUM_LOCK)
    MACRO_HELPME(KEY_PRINT_SCREEN)
    MACRO_HELPME(KEY_PAUSE)
    MACRO_HELPME(KEY_F1)
    MACRO_HELPME(KEY_F2)
    MACRO_HELPME(KEY_F3)
    MACRO_HELPME(KEY_F4)
    MACRO_HELPME(KEY_F5)
    MACRO_HELPME(KEY_F6)
    MACRO_HELPME(KEY_F7)
    MACRO_HELPME(KEY_F8)
    MACRO_HELPME(KEY_F9)
    MACRO_HELPME(KEY_F10)
    MACRO_HELPME(KEY_F11)
    MACRO_HELPME(KEY_F12)
    MACRO_HELPME(KEY_KP_0)
    MACRO_HELPME(KEY_KP_1)
    MACRO_HELPME(KEY_KP_2)
    MACRO_HELPME(KEY_KP_3)
    MACRO_HELPME(KEY_KP_4)
    MACRO_HELPME(KEY_KP_5)
    MACRO_HELPME(KEY_KP_6)
    MACRO_HELPME(KEY_KP_7)
    MACRO_HELPME(KEY_KP_8)
    MACRO_HELPME(KEY_KP_9)
    MACRO_HELPME(KEY_KP_DECIMAL)
    MACRO_HELPME(KEY_KP_DIVIDE)
    MACRO_HELPME(KEY_KP_MULTIPLY)
    MACRO_HELPME(KEY_KP_SUBTRACT)
    MACRO_HELPME(KEY_KP_ADD)
    MACRO_HELPME(KEY_KP_ENTER)
    MACRO_HELPME(KEY_KP_EQUAL)
    MACRO_HELPME(KEY_LEFT_SHIFT)
    MACRO_HELPME(KEY_LEFT_CONTROL)
    MACRO_HELPME(KEY_LEFT_ALT)
    MACRO_HELPME(KEY_LEFT_SUPER)
    MACRO_HELPME(KEY_RIGHT_SHIFT)
    MACRO_HELPME(KEY_RIGHT_CONTROL)
    MACRO_HELPME(KEY_RIGHT_ALT)
    MACRO_HELPME(KEY_RIGHT_SUPER)
    MACRO_HELPME(KEY_MENU)
    }
#undef MACRO_HELPME
}
