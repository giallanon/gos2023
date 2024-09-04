#include "gosInputEvtList.h"
#include "gosInput.h"
#include "../gos/gos.h"

using namespace gos;
using namespace gos::input;

//****************************************
EvtList::EvtList (u32 numStartingElem)
{
	nMaxElem = numStartingElem;
	nElem = 0;
	list = GOSALLOCT (input::EventID*, gos::getSysHeapAllocator(), sizeof(input::EventID) * nMaxElem);
}

//****************************************
EvtList::~EvtList()
{
	GOSFREE(gos::getSysHeapAllocator(), list);
}

//****************************************
void EvtList::reset()
{
	nElem = 0;
}

//****************************************
void EvtList::addButtonEvt (eOrigin origin, u16 buttonId, eButtonStatus status, const sButtonModifier &modifier)
{
	if (eOrigin::window == origin)
		priv_doAddEvent (input::event_button_makeID (origin, buttonId, status, sButtonModifier()));
	else
		priv_doAddEvent (input::event_button_makeID (origin, buttonId, status, modifier));
}

//****************************************
void EvtList::addAxleAbsEvt (eOrigin origin, input::eAxle axle, i16 pos)
{
	priv_doAddEvent (input::event_axleAbs_makeID (origin, axle, pos));
}

//****************************************
void EvtList::addAxleRelEvt (input::eOrigin origin, input::eAxle axle, input::eAxleDirection dir, u16 strength)
{
	priv_doAddEvent (input::event_axleRel_makeID (origin, axle, dir, strength));
}

//****************************************
void EvtList::priv_doAddEvent (const EventID &eventID)
{
	if (nElem >= nMaxElem)
		return;
	list[nElem++] = eventID;
	
	//input::event_printInfo (eventID);
	
	//char debug[64]; event_getEventName (eventID, debug, sizeof(debug)); printf ("%s\n", debug);
}



//****************************************
void EvtList::toStart (Iter &iter) const
{
	iter.i = 0;
	iter.n = nElem;
}

//****************************************
bool EvtList::next (Iter &iter, EventID *out) const
{
	if (iter.i >= iter.n)
		return false;

	*out = list[iter.i];
	iter.i++;
	return true;
}
