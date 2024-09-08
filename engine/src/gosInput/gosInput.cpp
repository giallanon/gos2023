#include "gosInput.h"
#include "gosInputModule.h"
#include "../gos/gos.h"
#include "../gos/memory/gosAllocatorHeap.h"

typedef gos::AllocatorHeap<gos::AllocPolicy_Track_simple, gos::AllocPolicy_Thread_Unsafe>		GOSInputMemAllocatorTS;

static GOSInputMemAllocatorTS *localAllocator;
static gos::input::Module *module = NULL;

using namespace gos;

//*****************************************
bool input::init()
{
    gos::logger::log ("INPUT::init\n");
    gos::logger::incIndent();
    
    localAllocator = GOSNEW(gos::getSysHeapAllocator(), GOSInputMemAllocatorTS)("INPUT");
    localAllocator->setup (1024 * 1024);
    
    module = GOSNEW (localAllocator, Module)(localAllocator);
    

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

        GOSDELETE(localAllocator, module);
        module = NULL;

        GOSDELETE(gos::getSysHeapAllocator(), localAllocator);
    }

    gos::logger::log ("finished\n");
    gos::logger::decIndent();
}

//*****************************************
gos::Allocator* input::getAllocator()
{
    return module->getAllocator();
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
        case eType::axleABS:   return "axleABS";
        case eType::axleREL:   return "axleREL";
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
const char* input::enumToString (input::eAxleDirection e)
{
    switch (e)
    {
        default:    DBGBREAK;   return "???";
        case eAxleDirection::positive: return "pos";
        case eAxleDirection::negative: return "neg";
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
void input::window_toggleFullscreen(const GOSWinHandle &handle)
{
    input::Window *win = gos_input_getWindowFromHandle (handle);
    if (NULL == win)
        return;

    GLFWwindow *glfWin = win->getGLFWHandle();
    GLFWmonitor *monitor = glfwGetWindowMonitor(glfWin);
    if (NULL == monitor)
    {
        //andiamo in full
        win->storeCurrentPosAndSize();
        gos::logger::log ("going full screen, current win pos and size (%d,%d) (%d,%d)\n", win->storedX, win->storedY, win->storedW, win->storedH);

        monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode *mode = glfwGetVideoMode(monitor);
        glfwSetWindowMonitor (glfWin, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
    }    
    else
    {
        //torniamo in windowed
        gos::logger::log ("going in windowed mode, current win pos and size (%d,%d) (%d,%d)\n", win->storedX, win->storedY, win->storedW, win->storedH);
        glfwSetWindowMonitor(glfWin, NULL, win->storedX, win->storedY, win->storedW, win->storedH, 0);
    }
}

//*****************************************
void input::resolveEvents (const GOSWinHandle &handle, const Context *ctx, ResolvedEvtList *out)
{
    input::Window *win = gos_input_getWindowFromHandle (handle);
    if (NULL != win)
    {
        win->resolveEvents_begin (ctx);
        out->setup (win);
    }
    else
        out->setup (NULL);
}

//*****************************************
input::eOrigin input::event_getOrigin (const EventID &eventID)
{
    //description
    //type      3bit
    //origin    3bit    
    return static_cast<input::eOrigin>( (eventID._data.asU16.description >> 10)  & 0x07);
}

//*****************************************
input::eType input::event_getType (const EventID &eventID)
{
    //description
    //type      3bit
    //origin    3bit    
    return static_cast<input::eType>(  (eventID._data.asU16.description >> 13) & 0x07);
}

//*****************************************
input::EventID input::event_button_makeID (input::eOrigin origin, u16 btnId, eButtonStatus status, const sButtonModifier &modifier)
{
    assert (static_cast<u32>(origin) < 8);
    assert (static_cast<u32>(status) < 2);
    assert (static_cast<u32>(modifier._status) < 256);
    //description
    //type      3bit
    //origin    3bit
    //status    1bit
    //empty     1bit
    //modifier  8bit
    EventID eventID;
    eventID._data.asU32.data = 0;
    eventID._data.asU16.description |= (static_cast<u32>(eType::button) << 13);
    eventID._data.asU16.description |= (static_cast<u32>(origin) << 10);
    eventID._data.asU16.description |= (static_cast<u32>(status) << 9);
    eventID._data.asU16.description |= modifier._status;

    eventID._data.asU16.value = btnId;
    return eventID;
}

//*****************************************
bool input::event_toButtonEvent (const EventID &eventID, sBtnEvent *out)
{
    assert (NULL != out);
    if (event_getType(eventID) != eType::button)
    {
        DBGBREAK;
        return false;
    }

    out->id = eventID._data.asU16.value;
    out->modifier._status = static_cast<u8>(eventID._data.asU16.description & 0x00ff);
    out->status = static_cast<input::eButtonStatus>( (eventID._data.asU16.description >> 9) & 0x01 );

    return true;
}

//*****************************************
input::EventID input::event_axleAbs_makeID (input::eOrigin origin, eAxle axle, i16 pos)
{
    assert (static_cast<u32>(origin) < 8);
    assert (static_cast<u32>(axle) < 8);
    //description
    //type      3bit
    //origin    3bit
    //axle      3bit
    //empty     7bit
    EventID eventID;
    eventID._data.asU32.data = 0;
    eventID._data.asU16.description |= (static_cast<u32>(eType::axleABS) << 13);
    eventID._data.asU16.description |= (static_cast<u32>(origin) << 10);
    eventID._data.asU16.description |= (static_cast<u32>(axle) << 7);

    eventID._data.asU16.value = static_cast<u16>(pos & 0xFFFF);

    return eventID;
}

//*****************************************
bool input::event_toAxleAbsEvent (const EventID &eventID, sAxleAbsEvent *out)
{
    assert (NULL != out);
    if (event_getType(eventID) != eType::axleABS)
    {
        DBGBREAK;
        return false;
    }

    out->axle = static_cast<input::eAxle>((eventID._data.asU16.description >> 7) & 0x07);
    out->pos = static_cast<i16>(eventID._data.asU16.value);
    return true;
}

//*****************************************
input::EventID input::event_axleRel_makeID (input::eOrigin origin, eAxle axle, eAxleDirection direction, u16 strength)
{
    assert (static_cast<u32>(origin) < 8);
    assert (static_cast<u32>(axle) < 8);
    assert (static_cast<u32>(direction) < 2);
    //description
    //type      3bit
    //origin    3bit
    //axle      3bit
    //direction 1bit
    //empty     6bit

    EventID eventID;
    eventID._data.asU32.data = 0;
    eventID._data.asU16.description |= (static_cast<u32>(eType::axleREL) << 13);
    eventID._data.asU16.description |= (static_cast<u32>(origin) << 10);
    eventID._data.asU16.description |= (static_cast<u32>(axle) << 7);
    eventID._data.asU16.description |= (static_cast<u32>(direction) << 6);

    eventID._data.asU16.value = strength;

    return eventID;
}

//*****************************************
bool input::event_toAxleRelEvent (const EventID &eventID, sAxleRelEvent *out)
{
    assert (NULL != out);
    if (event_getType(eventID) != eType::axleREL)
    {
        DBGBREAK;
        return false;
    }

    out->axle = static_cast<input::eAxle>((eventID._data.asU16.description >> 7) & 0x07);
    out->direction = static_cast<input::eAxleDirection>((eventID._data.asU16.description >> 6) & 0x01);
    out->strength = eventID._data.asU16.value;
    return true;
}

//*****************************************
void input::debug_event_printInfo (const EventID &eventID)
{
    printf ("event_info(0x%08X) => origin:%s, ", eventID._data.asU32.data, input::enumToString(input::event_getOrigin(eventID)));
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

    case eType::axleABS:
        printf ("type=axleABS, ");
        {
            sAxleAbsEvent info;
            input::event_toAxleAbsEvent (eventID, &info);
            printf ("axle=%s, pos=%d", input::enumToString(info.axle), info.pos);
        }
        break;

    case eType::axleREL:
        printf ("type=axleREL, ");
        {
            sAxleRelEvent info;
            input::event_toAxleRelEvent (eventID, &info);
            printf ("axle=%s, dir=%s, str=%d", input::enumToString(info.axle), input::enumToString(info.direction), info.strength);
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

            //stringhizza gli eventuali modifier
            char mod[512];
            memset (mod, 0, sizeof(mod));
            if (info.modifier.isLSHIFT())       strcat_s (mod, sizeof(mod), "LSHIFT+");
            if (info.modifier.isRSHIFT())       strcat_s (mod, sizeof(mod), "RSHIFT+");
            if (info.modifier.isLCTRL())        strcat_s (mod, sizeof(mod), "LCTRL+");
            if (info.modifier.isRCTRL())        strcat_s (mod, sizeof(mod), "RCTRL+");
            if (info.modifier.isLALT())         strcat_s (mod, sizeof(mod), "LALT+");
            if (info.modifier.isRALT())         strcat_s (mod, sizeof(mod), "RALT+");

            switch (input::event_getOrigin(eventID))
            {
            case eOrigin::keyboard:
                {
                    char keyName[32];
                    gos_input_getKEYname (info.id, keyName, sizeof(keyName));
                    sprintf_s (out, sizeof_out, "%s%s", mod, keyName);
                }
                break;

            case eOrigin::mouse:
                sprintf_s (out, sizeof_out, "%smouse.btn%d", mod, info.id);
                break;

            case eOrigin::window:
                switch (info.id)
                {
                default:
                    sprintf_s (out, sizeof_out, "%swin.%d", mod, info.id);
                    break;

                case GOS_BUTTON_WINDOW_CLOSE:
                    sprintf_s (out, sizeof_out, "%swin.CLOSE", mod);
                    break;
                }
            }

            if (input::eButtonStatus::pressed == info.status)
                strcat_s (out, sizeof_out, ".pressed");
            else
                strcat_s (out, sizeof_out, ".released");
        }
        break;

    case eType::axleABS:
        {
            sAxleAbsEvent info;
            input::event_toAxleAbsEvent (eventID, &info);
            
            sprintf_s (out, sizeof_out, "%s.%s", input::enumToString(event_getOrigin(eventID)), input::enumToString(info.axle));
        }
        break;

    case eType::axleREL:
        {
            sAxleRelEvent info;
            input::event_toAxleRelEvent (eventID, &info);
            
            sprintf_s (out, sizeof_out, "%s.%s.%s", input::enumToString(event_getOrigin(eventID)), input::enumToString(info.axle), input::enumToString(info.direction));
        }
        break;
    }    
}