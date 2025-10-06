#ifndef _gosInputEnumAndDefine_h_
#define _gosInputEnumAndDefine_h_
#include "GLFW/gosGLFWInclude.h"
#include "../gos/gosEnumAndDefine.h"
#include "../gos/dataTypes/gosColorHDR.h"
#include "../gos/gosHandle.h"


//A per "num max di handle", B per "num di chunk", C per "counter"
GOS_DECL_HANDLE(5,3,4, GOSWinHandle);		//2^5=32 => num totale di oggetti, divisi in chunk da 2^3=8


typedef void (*GOSWindowCallbackFN_onResize)(int newW, int newH, void *userpt);


#define GOS_BUTTON_MOUSE_LEFT       0
#define GOS_BUTTON_MOUSE_RIGHT      1
#define GOS_BUTTON_MOUSE_MIDDLE     2

#define GOS_BUTTON_WINDOW_CLOSE     0


namespace gos
{
    namespace input
    {
        enum class eOrigin : u8
        {
            keyboard = 0,
            mouse = 1,
            window = 2
            //max 8 elementi
        };

        enum class eType : u8
        {
            button = 0,
            axleABS = 1,
            axleREL = 2
            //max 8 elementi
        };        

        enum class eButtonStatus : u8
        {
            released = 0,
            pressed = 1,
            both = 2
        };

        enum class eButtonModifier : u8
        {
            NONE =  0x00,
            LCTRL =  0x01,
            RCTRL =  0x02,
            LSHIFT = 0x04,
            RSHIFT = 0x08,
            LALT =   0x10,
            RALT =   0x20
        };

        enum class eAxle : u8
        {
            x = 0,
            y = 1,
            z = 2
        };

        enum class eAxleDirection : u8
        {
            positive = 0,
            negative = 1,
            both = 2
        };        

        enum class eMouseMode : u8
        {
            absolute = 0,
            relative = 1
        };

        struct sButtonModifier
        {
        public:
                    sButtonModifier()                                                               { reset(); }
                    sButtonModifier(eButtonModifier m1)                                             { reset(); set(m1); }
                    sButtonModifier(eButtonModifier m1, eButtonModifier m2)                         { reset(); set(m1); set(m2); }
                    sButtonModifier(eButtonModifier m1, eButtonModifier m2, eButtonModifier m3)     { reset(); set(m1); set(m2); set(m3); }
                    
            bool    operator== (const sButtonModifier &b) const     { return (_status==b._status); }

        public:
            void    reset()                                         { _status = 0; }
            void    set (eButtonModifier b, bool pressed=true)      { if (pressed) _status |= static_cast<u8>(b); else clear(b); }
            void    clear (eButtonModifier b)                       { _status &= ~(static_cast<u8>(b)); }
            bool    isSet (eButtonModifier b) const                 { return (_status & static_cast<u8>(b)) != 0; }

            bool    isCTRL() const                                  { return isSet(eButtonModifier::LCTRL) || isSet(eButtonModifier::RCTRL); }
            bool    isALT() const                                   { return isSet(eButtonModifier::LALT) || isSet(eButtonModifier::RALT); }
            bool    isSHIFT() const                                 { return isSet(eButtonModifier::LSHIFT) || isSet(eButtonModifier::RSHIFT); }

            bool    isLCTRL() const                                 { return isSet(eButtonModifier::LCTRL); }
            bool    isRCTRL() const                                 { return isSet(eButtonModifier::RCTRL); }
            bool    isLALT() const                                  { return isSet(eButtonModifier::LALT); }
            bool    isRALT() const                                  { return isSet(eButtonModifier::RALT); }
            bool    isLSHIFT() const                                { return isSet(eButtonModifier::LSHIFT); }
            bool    isRSHIFT() const                                { return isSet(eButtonModifier::RSHIFT); }

        public:
            u8      _status;
        };

        struct EventID
        {
            struct sAsU32
            {
                u32 data;
            };

            struct sAsU16
            {
                u16 description;
                u16 value;
            };

            union eData
            {
                sAsU32  asU32;
                sAsU16  asU16;
            };

            eData   _data;
        };

        struct sBtnEvent
        {
            u16             id;         //id del btn premuto/rilasciato
            eButtonStatus   status;   
            sButtonModifier modifier;
        };

        struct sAxleAbsEvent
        {
            eAxle   axle;
            i16     pos;
        };

        struct sAxleRelEvent
        {
            eAxle           axle;
            eAxleDirection  direction;
            u16             strength;
        };

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


    } //namespace input
} //namespace gos
#endif //_gosInputEnumAndDefine_h_