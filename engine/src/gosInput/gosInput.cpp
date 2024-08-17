#include "gosInput.h"
#include "gosInputWindow.h"
#include "../gos/gos.h"

namespace gos
{
    namespace input
    {
        //****************************************
        class Module
        {
        public:
                Module() : voidEvtList(1)
                {
                }

        public:
            gos::HandleList<GOSWinHandle, input::Window*>   windowList;
            input::EvtList                                  voidEvtList;
        };

    } //namespace input
} //namespace gos


static gos::input::Module *module = NULL;

using namespace gos;

//*****************************************
bool input::init()
{
    gos::logger::log ("INPUT::init\n");
    gos::logger::incIndent();
    
    
    module = GOSNEW(gos::getSysHeapAllocator(), Module)();
    module->windowList.setup (gos::getSysHeapAllocator());
    
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    gos::logger::log ("finished\n");
    gos::logger::decIndent();
    return true;
}

//*****************************************
void input::deinit()
{
    gos::logger::log ("INPUT::deinit\n");
    gos::logger::incIndent();

    if (NULL != module)
    {
        glfwTerminate();

        module->windowList.unsetup();        
        GOSDELETE(gos::getSysHeapAllocator(), module);
        module = NULL;
    }

    gos::logger::log ("finished\n");
    gos::logger::decIndent();
}


//*****************************************
bool input::window_create (int w, int h, const char *title, GOSWinHandle *out_handle)
{
    assert (NULL != out_handle);

    input::Window **ret = module->windowList.reserve (out_handle);
    if (NULL == ret)
    {
        out_handle->setInvalid();
        gos::logger::err ("input::window_create(%d, %d, %s) = failed to reserve handle\n", w, h, title);
        return false;
    }

    GLFWwindow  *glfwHandle = glfwCreateWindow (w, h, title, nullptr, nullptr);
    if (NULL == glfwHandle)
    {
        module->windowList.release (*out_handle);
        out_handle->setInvalid();
        gos::logger::err ("input::window_create(%d, %d, %s) = failed create window\n", w, h, title);
        return false;
    }

    input::Window *win = GOSNEW(gos::getSysHeapAllocator(), input::Window) (glfwHandle);
    *ret = win;
    return true;
}

//*****************************************
void input::pollEvents()
{
    glfwPollEvents();
}

//*****************************************
const char* input::enumToString (input::eType e)
{
    switch (e)
    {
        default:    DBGBREAK; return "???";
        case eType::button: return "button";
        case eType::axle:   return "axle";
    }
}

//*****************************************
const char* input::enumToString (input::eOrigin e)
{
    switch (e)
    {
        default:    DBGBREAK;   return "???";
        case eOrigin::keyboard: return "keyboard";
        case eOrigin::mouse:    return "mouse";
        case eOrigin::window:   return "window";
    }
}

//*****************************************
const char* input::enumToString (input::eAxle e)
{
    switch (e)
    {
        default:    DBGBREAK;   return "???";
        case eAxle::x: return "x";
        case eAxle::y: return "y";
        case eAxle::z: return "z";
    }
}

//*****************************************
input::Window* gos_input_getWindowFromHandle (const GOSWinHandle &handle)
{
    input::Window **win;
    if (module->windowList.fromHandleToPointer (handle, &win))
        return *win;
    return NULL;
}

//*****************************************
void input::window_destroy (GOSWinHandle &handle)
{
    input::Window *win = gos_input_getWindowFromHandle (handle);
    if (NULL == win)
        return; 
    
    glfwDestroyWindow (win->getGLFWHandle());
    GOSDELETE(gos::getSysHeapAllocator(), win);
    module->windowList.release (handle);
    handle.setInvalid();
}

//*****************************************
void input::window_setTitle (const GOSWinHandle &handle, const char *title)
{
    input::Window *win = gos_input_getWindowFromHandle (handle);
    if (NULL != win)
        glfwSetWindowTitle (win->getGLFWHandle(), title);
}

//*****************************************
bool input::window_shouldClose (const GOSWinHandle &handle)
{
    input::Window *win = gos_input_getWindowFromHandle (handle);
    if (NULL == win)
        return false; 
        
    return (glfwWindowShouldClose (win->getGLFWHandle()) != 0);
}

//*****************************************
void input::window_show (const GOSWinHandle &handle)
{
    input::Window *win = gos_input_getWindowFromHandle (handle);
    if (NULL != win)
        glfwShowWindow (win->getGLFWHandle());
}

//*****************************************
void input::window_hide (const GOSWinHandle &handle)
{
    input::Window *win = gos_input_getWindowFromHandle (handle);
    if (NULL != win)
        glfwHideWindow (win->getGLFWHandle());
}

//*****************************************
void input::window_setPos (const GOSWinHandle &handle, int x, int y)
{
    input::Window *win = gos_input_getWindowFromHandle (handle);
    if (NULL != win)
        glfwSetWindowPos (win->getGLFWHandle(), x, y);
}

//*****************************************
void input::window_getPos (const GOSWinHandle &handle, int *out_x, int *out_y)
{
    input::Window *win = gos_input_getWindowFromHandle (handle);
    if (NULL != win)
        glfwGetWindowPos (win->getGLFWHandle(), out_x, out_y);
}

//*****************************************
void input::window_setSize (const GOSWinHandle &handle, int dimX, int dimY)
{
    input::Window *win = gos_input_getWindowFromHandle (handle);
    if (NULL != win)
        glfwSetWindowSize (win->getGLFWHandle(), dimX, dimY);
}

//*****************************************
void input::window_getSize (const GOSWinHandle &handle, int *out_dimX, int *out_dimY)
{
    input::Window *win = gos_input_getWindowFromHandle (handle);
    if (NULL != win)
        glfwGetWindowSize (win->getGLFWHandle(), out_dimX, out_dimY);
}

//*****************************************
bool input::window_getGLF (const GOSWinHandle &handle, GLFWwindow **out_GLFWindow)
{
    input::Window *win = gos_input_getWindowFromHandle (handle);
    if (NULL != win)
    {
        *out_GLFWindow = win->getGLFWHandle();
        return true;
    }
    
    *out_GLFWindow = NULL;
    return false;
}

//*****************************************
void input::window_setUserPointer (const GOSWinHandle &handle, void *userPt)
{
    input::Window *win = gos_input_getWindowFromHandle (handle);
    if (NULL != win)
        win->userpt = userPt;
}

//*****************************************
bool input::window_getUserPointer (const GOSWinHandle &handle, void **out_userPt)
{
    assert (NULL != out_userPt);
    assert (NULL != *out_userPt);

    input::Window *win = gos_input_getWindowFromHandle (handle);
    if (NULL != win)
    {
        *out_userPt = win->userpt;
        return true;
    }

    *out_userPt = NULL;
    return false;
}

//*****************************************
void input::window_setMouseMode (const GOSWinHandle &handle, eMouseMode mode)
{
    input::Window *win = gos_input_getWindowFromHandle (handle);
    if (NULL != win)
        win->setMouseMode (mode);
}

//*****************************************
void input::window_toggleMouseMode (const GOSWinHandle &handle)
{
    input::Window *win = gos_input_getWindowFromHandle (handle);
    if (NULL != win)
        win->toggleMouseMode();
}

//*****************************************
const input::EvtList* input::window_getEventList (const GOSWinHandle &handle)
{
    input::Window *win = gos_input_getWindowFromHandle (handle);
    if (NULL != win)
        return win->swapEvtList();
    
    return &module->voidEvtList;
}

//*****************************************
u32 gos_input_makeBaseEventID (input::eOrigin origin, input::eType type)
{
    //origin 3 bit
    //type   2 bit
    //empty  27bit
    assert (static_cast<u32>(origin) < 8);
    assert (static_cast<u32>(type) < 4);
    
    u32 id = ((u32)origin) << 29;
    id |= (static_cast<u32>(type) << 27);
    return id;
}

//*****************************************
input::eOrigin input::event_getOrigin (const EventID &id)
{
    return static_cast<input::eOrigin>( (id._data >> 29) );
}

//*****************************************
input::eType input::event_getType (const EventID &id)
{
    return static_cast<input::eType>(  (id._data >> 27) & 0x03);
}

//*****************************************
input::EventID input::event_makeID (input::eOrigin origin, u16 btnId, eButtonStatus status, const sButtonModifier &modifier)
{
    u32 id = gos_input_makeBaseEventID (origin, input::eType::button);
    //ci sono 27 bit liberi:
    // empty    2 bit
    // status	1 bit
	// modifier	8 bit
    // btnID	16 bit    

    assert (static_cast<u32>(modifier._status) < 256);
    
    if (eButtonStatus::pressed == status)
        id |= (0x00000001 << 24);
    id |= (static_cast<u32>(modifier._status) << 16);

    id |= btnId;

    input::EventID ret;
    ret._data = id;
    return ret;
}

//*****************************************
input::EventID input::event_makeID (input::eOrigin origin, eAxle axle, i16 pos)
{
    u32 id = gos_input_makeBaseEventID (origin, input::eType::axle);
    //ci sono 27 bit liberi:
    // empty    9 bit
    // axle     2 bit
    // pos      16bit
    assert (static_cast<u32>(axle) < 4);
    
    id |= (static_cast<u32>(axle) << 16);
    id |= (u32)(pos & 0xFFFF);

    input::EventID ret;
    ret._data = id;
    return ret;
}

//*****************************************
bool input::event_toButtonEvent (const EventID &id, sBtnEvent *out)
{
    assert (NULL != out);
    if (event_getType(id) != eType::button)
    {
        DBGBREAK;
        return false;
    }

    out->id = (id._data & 0x0000FFFF);
    out->modifier._status = ((id._data & 0x00FF0000) >> 16);
    if ((id._data & 0x01000000) != 0)
        out->status = eButtonStatus::pressed;
    else
        out->status = eButtonStatus::released;

    return true;
}

//*****************************************
bool input::event_toAxleEvent (const EventID &id, sAxleEvent *out)
{
    assert (NULL != out);
    if (event_getType(id) != eType::axle)
    {
        DBGBREAK;
        return false;
    }

    out->axle = static_cast<input::eAxle>((id._data >> 16) & 0x03);
    out->pos = static_cast<i16>(id._data & 0x0000FFFF);
    return true;
}


//*****************************************
void input::debug_event_printInfo (const EventID &eventID)
{
    printf ("event_info(0x%08X) => origin:%s, ", eventID._data, input::enumToString(input::event_getOrigin(eventID)));
    switch (event_getType(eventID))
    {
    default:
        printf ("unknown type");
        break;

    case eType::button:
        printf ("type=button, ");
        {
            sBtnEvent info;
            input::event_toButtonEvent(eventID, &info);
            printf ("id=%d, modifier=0x%08X, ", info.id, info.modifier._status);
            if (info.status == eButtonStatus::pressed)
                printf ("pressed");
            else
                printf ("released");
        }
        break;

    case eType::axle:
        printf ("type=axle, ");
        {
            sAxleEvent info;
            input::event_toAxleEvent (eventID, &info);
            printf ("axle=%s, pos=%d", input::enumToString(info.axle), info.pos);
        }
        break;    
    }

    printf ("\n");
}

//*****************************************
void gos_input_getKEYname (u32 keyID, char *out_kname, u32 sizeof_outkname)
{
#define MACRO_HELPME(glDEFINE_senza_GLFW)\
    case GLFW_##glDEFINE_senza_GLFW: sprintf_s (out_kname, sizeof_outkname, #glDEFINE_senza_GLFW); break;

    switch (keyID)
    {
    default:
        {
            const char *key_name = glfwGetKeyName (keyID, 0);
            if (NULL != key_name)
                sprintf_s (out_kname, sizeof_outkname, "KEY_%s", key_name);
            else
                sprintf_s (out_kname, sizeof_outkname, "??KEY_NONAME??"); 
        }
        break;

    case GLFW_KEY_ESCAPE:    sprintf_s (out_kname, sizeof_outkname, "KEY_ESCAPE"); break;
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

//*****************************************
void input::event_getEventName (const EventID &eventID, char *out, u32 sizeof_out)
{
    switch (event_getType(eventID))
    {
    default:
        sprintf_s (out, sizeof_out, "???");
        break;

    case eType::button:
        {
            sBtnEvent info;
            input::event_toButtonEvent(eventID, &info);
            switch (input::event_getOrigin(eventID))
            {
            case eOrigin::keyboard:
                gos_input_getKEYname (info.id, out, sizeof_out);
                break;

            case eOrigin::mouse:
                sprintf_s (out, sizeof_out, "mouse.btn%d", info.id);
                break;

            case eOrigin::window:
                sprintf_s (out, sizeof_out, "win.%d", info.id);
                break;
            }
        }
        break;

    case eType::axle:
        {
            sAxleEvent info;
            input::event_toAxleEvent (eventID, &info);
            
            sprintf_s (out, sizeof_out, "%s.%s", input::enumToString(event_getOrigin(eventID)), input::enumToString(info.axle));
        }
        break;
    }    
}