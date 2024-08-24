#include "gosInputMapper.h"
#include "gosInput.h"
#include "../gos/gos.h"
#include "../gos/gosUtils.h"

using namespace gos;
using namespace gos::input;

/***************************************************************************************************************************************
 * 
 *      Mapper::Context
 * 
 ****************************************************************************************************************************************/
Mapper::Context::Context (gos::Allocator *allocator, const char *contextName)
{
    actionList.setup (allocator, 128);
    sprintf_s (name, sizeof(name), "%s", contextName);
    UID = utils::crc32(contextName);

	btnEventList.setup (allocator, 256);
	axleEventList.setup (allocator, 64);    
}

//***************************
Mapper::Context::~Context()
{
    actionList.unsetup();
	btnEventList.unsetup();
	axleEventList.unsetup();    
}

//***************************
void Mapper::Context::action_add (const sAction &action)
{
    assert (NULL == action_exists (action.actionID));
    actionList.append (action);
}

//***************************
const Mapper::sAction* Mapper::Context::action_exists (u32 actionID) const
{
    const u32 n = actionList.getNElem();
    for (u32 i=0; i<n; i++)
    {
        if (actionList(i).actionID == actionID)
            return &actionList(i);
    }
    return NULL;
}

//************************************
bool Mapper::Context::action_bindToBtn (u32 actionID, input::eOrigin origin, u16 btnId, eButtonStatus status, const sButtonModifier &modifier)
{
	return priv_addBind (btnEventList, input::event_button_makeID (origin, btnId, status, modifier), actionID);
}

//************************************
bool Mapper::Context::action_bindToAxleABS (u32 actionID, input::eOrigin origin, input::eAxle axle)
{
	return priv_addBind (axleEventList, input::event_axleAbs_makeID (origin, axle, 0), actionID);
}
//************************************
bool Mapper::Context::action_bindToAxleREL (u32 actionID, input::eOrigin origin, input::eAxle axle, input::eAxleDirection dir)
{
	return priv_addBind (axleEventList, input::event_axleRel_makeID (origin, axle, dir, 0), actionID);
}

//************************************
bool Mapper::Context::priv_addBind (FastArray<sMappedAction> &dst, const EventID &eventID, u32 actionID)
{
	sMappedAction e;
	e.eventID = eventID;
	e.actionID = actionID;
	dst.append (e);
    return true;
}

//************************************
bool Mapper::Context::priv_isBound (const FastArray<sMappedAction> &list, u32 actionID, FastArray<EventID> &out) const
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
bool Mapper::Context::action_isMapped (u32 actionID, FastArray<EventID> &out) const
{
    bool ret = false;
    
    out.reset();
    if (priv_isBound (btnEventList, actionID, out))
        ret = true;
    if (priv_isBound (axleEventList, actionID, out))
        ret = true;
    return ret;
}


//************************************
u32 Mapper::Context::resolveEvent (const input::EventID &eventID, i16 *out_value) const
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



/***************************************************************************************************************************************
 * 
 *      Mapper 
 * 
 ****************************************************************************************************************************************/
Mapper::Mapper()
{
    gos::Allocator *localAllocator = gos::getSysHeapAllocator();
    actionNameList.setup (localAllocator, 65*1024);
    contextList.setup (localAllocator, 32);
    mouseStatus.reset();
}

//***************************
Mapper::~Mapper()
{
    const u32 n = contextList.getNElem();
    for (u32 i=0; i<n; i++)
    {
        Context *ctx = contextList[i];
        GOSDELETE (gos::getSysHeapAllocator(), ctx);
    }    
    contextList.unsetup();

    actionNameList.unsetup();
}

//***************************
u32 Mapper::priv_makeActionID (const char *contextName, const char *actionName, char *out_fullName, u32 sizeof_fullName) const
{
    sprintf_s (out_fullName, sizeof_fullName, "%s.%s", contextName, actionName);
    return utils::crc32 (out_fullName);
}

//***************************
u32 Mapper::priv_context_find (const char *contextName) const
{
    return priv_context_find (utils::crc32(contextName));
}

//***************************
u32 Mapper::priv_context_find (u32 contextUID) const
{
    const u32 n = contextList.getNElem();
    for (u32 i=0; i<n; i++)
    {
        if (contextList(i)->getUID() == contextUID)
            return i;
    }
    return u32MAX;
}

//***************************
Mapper::Context* Mapper::priv_context_add (const char *contextName)
{
    Context *c = GOSNEW(gos::getSysHeapAllocator(), Context) (gos::getSysHeapAllocator(), contextName);
    contextList.append (c);
    return c;
}

//***************************
u32 Mapper::action_add (const char *contextName, const char *actionName)
{
    Context *ctx = NULL;
    {
        u32 iCtx = priv_context_find (contextName);
        if (u32MAX == iCtx)
            ctx = priv_context_add (contextName);
        else
            ctx = contextList[iCtx];
    }


    char s[512];
    const u32 actionID = priv_makeActionID (contextName, actionName, s, sizeof(s));
    if (ctx->action_exists (actionID))
    {
        DBGBREAK;
        return 0;
    }


    sAction action;
    action.actionID = actionID;
    action.offsetToActionName = actionNameList.add (s);
    ctx->action_add (action);

    return actionID;
}

//***************************
bool Mapper::priv_findByActionID (u32 actionID, u32 *out_iCtx, const sAction **out_actionInfo) const
{
    assert (NULL != out_iCtx);
    assert (NULL != out_actionInfo);
    
    const u32 n = contextList.getNElem();
    for (u32 i=0; i<n; i++)
    {
        const sAction *action = contextList(i)->action_exists (actionID);
        if (NULL != action)
        {
            *out_iCtx = i;
            *out_actionInfo = action;
            return true;
        }
    }
    return false;
}

//***************************
bool Mapper::action_existsByActionID (u32 actionID) const
{
    const sAction *action;
    u32 iCtx;
    return priv_findByActionID (actionID, &iCtx, &action);
}

//***************************
const char* Mapper::action_getNameByActionID (u32 actionID) const
{
    const u32 n = contextList.getNElem();
    for (u32 i=0; i<n; i++)
    {
        const sAction *a = contextList(i)->action_exists (actionID);
        if (NULL != a)
        {
            return actionNameList.getStringAtOffset (a->offsetToActionName);
        }
    }
    return NULL;
}

//***************************
const char* Mapper::context_getActionNameByIndex (u32 iCtx, u32 iAction) const
{
    assert(iCtx<context_getNum()); 
    const sAction *a = contextList(iCtx)->action_getByIndex (iAction);
    if (NULL == a)
    {
        DBGBREAK;
        return NULL;
    }

    return actionNameList.getStringAtOffset (a->offsetToActionName);
}

//***************************
u32 Mapper::context_getActionIDByIndex (u32 iCtx, u32 iAction) const
{
    assert(iCtx<context_getNum()); 
    const sAction *a = contextList(iCtx)->action_getByIndex (iAction);
    if (NULL == a)
    {
        DBGBREAK;
        return 0;
    }

    return a->actionID;
}

//***************************
bool Mapper::action_bindToBtn (const char *contextName, const char *actionName, input::eOrigin origin, u16 btnId, eButtonStatus status, const sButtonModifier &modifier)
{
    char s[512];
    const u32 actionID = priv_makeActionID (contextName, actionName, s, sizeof(s));
    
    //cerco il context in cui risiede la [actionID]
    const sAction *action;
    u32 iCtx;
    if (!priv_findByActionID (actionID, &iCtx, &action))
    {
        DBGBREAK;
        return false;
    }

    if (eButtonStatus::both == status)
    {
        if (!contextList[iCtx]->action_bindToBtn (actionID, origin, btnId, eButtonStatus::pressed, modifier))
            return false;
        return contextList[iCtx]->action_bindToBtn (actionID, origin, btnId, eButtonStatus::released, modifier);
    }
    else
        return contextList[iCtx]->action_bindToBtn (actionID, origin, btnId, status, modifier);
}


//***************************
bool Mapper::action_bindToAxleABS (const char *contextName, const char *actionName, input::eOrigin origin, input::eAxle axle)
{
    char s[512];
    const u32 actionID = priv_makeActionID (contextName, actionName, s, sizeof(s));

    //cerco il context in cui risiede la [actionID]
    const sAction *action;
    u32 iCtx;
    if (!priv_findByActionID (actionID, &iCtx, &action))
    {
        DBGBREAK;
        return false;
    }
    return contextList[iCtx]->action_bindToAxleABS (actionID, origin, axle);
}

//***************************
bool Mapper::action_bindToAxleREL (const char *contextName, const char *actionName, input::eOrigin origin, input::eAxle axle, input::eAxleDirection dir)
{
    char s[512];
    const u32 actionID = priv_makeActionID (contextName, actionName, s, sizeof(s));

    //cerco il context in cui risiede la [actionID]
    const sAction *action;
    u32 iCtx;
    if (!priv_findByActionID (actionID, &iCtx, &action))
    {
        DBGBREAK;
        return false;
    }

    if (dir == eAxleDirection::both)
    {
        if (!contextList[iCtx]->action_bindToAxleREL (actionID, origin, axle, eAxleDirection::positive))
            return false;
        return contextList[iCtx]->action_bindToAxleREL (actionID, origin, axle, eAxleDirection::negative);
    }
    else
        return contextList[iCtx]->action_bindToAxleREL (actionID, origin, axle, dir);
}
//***************************
void Mapper::logAllMappedInput() const
{
    u8 buffer[1024];
    FastArray<EventID> eventIDList;
    eventIDList.setupWithBase (buffer, sizeof(buffer), gos::getScrapAllocator());

    gos::logger::log (eTextColor::yellow, "---------------------------------------\n");
    gos::logger::log (eTextColor::yellow, "Mapper::logAllMappedInput()\n");
    gos::logger::log (eTextColor::yellow, "---------------------------------------\n");
    gos::logger::incIndent();
    const u32 n = contextList.getNElem();
    for (u32 i=0; i<n; i++)
    {
        gos::logger::log (eTextColor::yellow, "CONTEXT: %s\n", contextList(i)->getName());
        gos::logger::incIndent();
        const u32 nAction = contextList(i)->action_getNum();
        for (u32 i2=0; i2<nAction; i2++)
        {
            const sAction *action = contextList(i)->action_getByIndex(i2);
            gos::logger::log ("%s\t\t\t\t", actionNameList.getStringAtOffset(action->offsetToActionName));
            
            //mostro il key binding se esiste
            if (contextList(i)->action_isMapped (action->actionID, eventIDList))
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
    gos::logger::decIndent();
}

//***************************
void Mapper::resolve_begin (const EvtList *evtListIN)
{
	evtList = evtListIN;
	evtList->toStart (iter);
}

//***************************
u32 Mapper::resolve_getNextActionID (i16 *out_value)
{
    u32 nCtx = contextList.getNElem();

	input::EventID eventID;
	while (evtList->next (iter, &eventID))
	{
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
                        mouseStatus.x = info.pos;
                    else if (eAxle::y == info.axle)
                        mouseStatus.y = info.pos;
                }
                break;

            case eType::button:
                {
                    sBtnEvent info;
                    input::event_toButtonEvent (eventID, &info);
                    if (eButtonStatus::pressed == info.status)
                        mouseStatus.btnPressed[info.id] = 1;
                    else
                        mouseStatus.btnPressed[info.id] = 0;
                }
                break;
            }
        }
        for (u32 i=0; i<nCtx; i++)
        {
            const u32 actionID = contextList(i)->resolveEvent (eventID, out_value);
            if (0 != actionID)
                return actionID;
        }
	}

	return 0;
}
