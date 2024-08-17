#include "gosInputController.h"
#include "gosInput.h"
#include "../gos/gos.h"
#include "../gos/gosUtils.h"

using namespace gos;
using namespace gos::input;

//************************************
Controller::Controller ()
{
	btnEventList.setup (gos::getSysHeapAllocator(), 256);
	axleEventList.setup (gos::getSysHeapAllocator(), 64);
}

//************************************
Controller::~Controller()
{
	btnEventList.unsetup();
	axleEventList.unsetup();
}

//************************************
void Controller::priv_addEvent (FastArray<sEvent> &dst, const EventID &eventID, const char *action)
{
	sEvent e;
	e.eventID = eventID;
	e.actionID = utils::crc32(action);
	assert (e.actionID != 0);
	dst.append (e);
}


//************************************
void Controller::bindBtnPress (const char *action, input::eOrigin origin, u16 btnId, const sButtonModifier &modifier)
{
	priv_addEvent (btnEventList, input::event_makeID (origin, btnId, eButtonStatus::pressed, modifier), action);
}

//************************************
void Controller::bindBtnRelease (const char *action, input::eOrigin origin, u16 btnId, const sButtonModifier &modifier)
{
	priv_addEvent (axleEventList, input::event_makeID (origin, btnId, eButtonStatus::released, modifier), action);
}

//************************************
u32 Controller::priv_resolveEvent (const FastArray<sEvent> &list, const EventID &eventID) const
{
	const u32 n = list.getNElem();
	for (u32 i=0; i<n; i++)
	{
		if (list(i).eventID == eventID)
			return list(i).actionID;
	}
	return 0;
}

//************************************
void Controller::beginParse (const EvtList *evtListIN)
{
	evtList = evtListIN;
	evtList->toStart (iter);
}

//************************************
u32	Controller::nextEvent()
{
	u32 ret;
	input::EventID eventID;
	while (evtList->next (iter, &eventID))
	{
		switch (input::event_getType(eventID))
		{
		case input::eType::button:
			ret = priv_resolveEvent (btnEventList, eventID);
			if (ret)
				return ret;
			break;

		case input::eType::axle:
			ret = priv_resolveEvent (axleEventList, eventID);
			if (ret)
				return ret;
			break;
		}
	}

	return 0;
}

