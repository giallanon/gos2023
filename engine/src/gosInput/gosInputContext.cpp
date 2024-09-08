#include "gosInputContext.h"
#include "gosInput.h"
#include "../gos/gos.h"
#include "../gos/gosUtils.h"

using namespace gos;
using namespace gos::input;

//***************************
void Context::priv_init (const char *contextName)
{
    setContextName (contextName);
    actionNameList.setup (input::getAllocator(), 16*1204);
    actionList.setup (input::getAllocator(), 128);
	btnEventList.setup (input::getAllocator(), 256);
	axleEventList.setup (input::getAllocator(), 64);    
}

//***************************
Context::~Context()
{
    actionNameList.unsetup();
    actionList.unsetup();
	btnEventList.unsetup();
	axleEventList.unsetup();    
}

//***************************
void Context::setContextName (const char *contextName)
{
    sprintf_s (name, sizeof(name), "%s", contextName);
}

//***************************
u32 Context::priv_makeActionID (const char *actionName) const
{
    return utils::crc32 (actionName);
}

//***************************
const sAction* Context::priv_action_existsByID (u32 actionID) const
{
    const u32 n = actionList.getNElem();
    for (u32 i=0; i<n; i++)
    {
        if (actionList(i).actionID == actionID)
            return &actionList(i);
    }
    return NULL;
}

//***************************
Context& Context::action_add (const char *actionName)
{
    const u32 actionID = priv_makeActionID (actionName); 
    if (!priv_action_existsByID (actionID))
    {
        sAction action;
        action.actionID = actionID;
        action.offsetToActionName = actionNameList.add (actionName);
        
        actionList.append (action);
    }
    return *this;
}

//************************************
bool Context::priv_action_bindToBtn (u32 actionID, input::eOrigin origin, u16 btnId, eButtonStatus status, const sButtonModifier &modifier)
{
	return priv_addBind (btnEventList, input::event_button_makeID (origin, btnId, status, modifier), actionID);
}

bool Context::action_bindToBtn (const char *actionName, input::eOrigin origin, u16 btnId, eButtonStatus status, const sButtonModifier &modifier)
{
    const u32 actionID = priv_makeActionID (actionName);
    
    if (NULL == priv_action_existsByID (actionID))
    {
        DBGBREAK;
        return false;
    }

    if (eButtonStatus::both == status)
    {
        if (!priv_action_bindToBtn (actionID, origin, btnId, eButtonStatus::pressed, modifier))
            return false;
        return priv_action_bindToBtn(actionID, origin, btnId, eButtonStatus::released, modifier);
    }
    else
        return priv_action_bindToBtn(actionID, origin, btnId, status, modifier);
}

//***************************
bool Context::priv_action_bindToAxleABS (u32 actionID, input::eOrigin origin, input::eAxle axle)
{
	return priv_addBind (axleEventList, input::event_axleAbs_makeID (origin, axle, 0), actionID);
}

bool Context::action_bindToAxleABS (const char *actionName, input::eOrigin origin, input::eAxle axle)
{
    const u32 actionID = priv_makeActionID (actionName);

    if (NULL == priv_action_existsByID (actionID))
    {
        DBGBREAK;
        return false;
    }
    return priv_action_bindToAxleABS (actionID, origin, axle);
}

//***************************
bool Context::priv_action_bindToAxleREL (u32 actionID, input::eOrigin origin, input::eAxle axle, input::eAxleDirection dir)
{
	return priv_addBind (axleEventList, input::event_axleRel_makeID (origin, axle, dir, 0), actionID);
}

bool Context::action_bindToAxleREL (const char *actionName, input::eOrigin origin, input::eAxle axle, input::eAxleDirection dir)
{
    const u32 actionID = priv_makeActionID (actionName);

    if (NULL == priv_action_existsByID (actionID))
    {
        DBGBREAK;
        return false;
    }

    if (dir == eAxleDirection::both)
    {
        if (!priv_action_bindToAxleREL (actionID, origin, axle, eAxleDirection::positive))
            return false;
        return priv_action_bindToAxleREL (actionID, origin, axle, eAxleDirection::negative);
    }
    else
        return priv_action_bindToAxleREL (actionID, origin, axle, dir);
}

//************************************
bool Context::priv_addBind (FastArray<sMappedAction> &dst, const EventID &eventID, u32 actionID)
{
	sMappedAction e;
	e.eventID = eventID;
	e.actionID = actionID;
	dst.append (e);
    return true;
}

//************************************ 
bool Context::priv_isBound (const FastArray<sMappedAction> &list, u32 actionID, FastArray<EventID> &out) const 
{ 
    bool ret = false; 
	const u32 n = list.getNElem(); 
	for (u32 i=0; i<n; i++) 
	{ 
        if (list(i).actionID == actionID) 
        { 
            out.append (list(i).eventID); 
            ret = true; 
        } 
    } 
    return ret; 
} 
 
//************************************ 
bool Context::priv_getAllMappedInputEvent (u32 actionID, FastArray<EventID> &out) const 
{ 
    bool ret = false; 
     
    out.reset(); 
    if (priv_isBound (btnEventList, actionID, out)) 
        ret = true; 
    if (priv_isBound (axleEventList, actionID, out)) 
        ret = true; 
    return ret; 
} 

//***************************
void Context::logAllMappedInput() const
{
    u8 buffer[1024];
    FastArray<EventID> eventIDList;
    eventIDList.setupWithBase (buffer, sizeof(buffer), gos::getScrapAllocator());

    gos::logger::log (eTextColor::yellow, "CONTEXT: %s\n", getName());
    gos::logger::incIndent();
    
    for (u32 i2=0; i2<actionList.getNElem(); i2++)
    {
        const sAction *action = &actionList(i2);
        gos::logger::log ("%s\t\t\t\t", actionNameList.getStringAtOffset (action->offsetToActionName));
        
        //mostro il key binding se esiste
        if (priv_getAllMappedInputEvent (action->actionID, eventIDList))
        {
            const u32 n3 = eventIDList.getNElem();
            for (u32 i3=0; i3<n3; i3++)
            {
                char s[512];
                input::event_getEventName (eventIDList(i3), s, sizeof(s));
                gos::logger::log ("[%s]  ", s);
            }
        }
        gos::logger::log ("\n");            
    }
    gos::logger::decIndent();

}

//************************************
u32 Context::resolveEvent (const input::EventID &eventID, i16 *out_value) const
{
    assert (NULL != out_value);
    *out_value = 0;

    switch (input::event_getType(eventID))
    {
    default:
        return 0;

    case input::eType::button:
        {
        	const u32 n = btnEventList.getNElem();
            for (u32 i=0; i<n; i++)
            {
                if (btnEventList(i).eventID._data.asU32.data == eventID._data.asU32.data)
                {
                    sBtnEvent info;
                    input::event_toButtonEvent(eventID, &info);
                    if (input::eButtonStatus::pressed == info.status)
                        *out_value = 1;
                    return btnEventList(i).actionID;
                }
            }
        }
	    return 0;
    
    case input::eType::axleREL:
        {
        	const u32 n = axleEventList.getNElem();
            for (u32 i=0; i<n; i++)
            {
                if (axleEventList(i).eventID._data.asU16.description == eventID._data.asU16.description)
                {
                    sAxleRelEvent info;
                    input::event_toAxleRelEvent (eventID, &info);
                    if (input::eAxleDirection::positive == info.direction)
                        *out_value = info.strength;
                    else
                        *out_value = -info.strength;
                    return axleEventList(i).actionID;
                }
            }
        }
	    return 0; 

    case input::eType::axleABS:
        {
        	const u32 n = axleEventList.getNElem();
            for (u32 i=0; i<n; i++)
            {
                if (axleEventList(i).eventID._data.asU16.description == eventID._data.asU16.description)
                {
                    sAxleAbsEvent info;
                    input::event_toAxleAbsEvent (eventID, &info);
                    *out_value = info.pos;
                    return axleEventList(i).actionID;
                }
            }
        }
	    return 0;    
    }
}
