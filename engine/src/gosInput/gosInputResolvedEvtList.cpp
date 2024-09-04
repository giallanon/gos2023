#include "gosInputResolvedEvtList.h"
#include "gosInput.h"
#include "gosInputWindow.h"
#include "gosInputContext.h"

using namespace gos;
using namespace gos::input;


//**********************************************
ResolvedEvtList::ResolvedEvtList()
{
	win = NULL;
}

void ResolvedEvtList::setup (Window *winIN)
{
	win = winIN;
}

//**********************************************
u32  ResolvedEvtList::nextActionID (i16 *out_value)
{ 
	if (NULL == win)
		return 0;
	return win->resolveEvents_nextActionID (out_value);
}

//**********************************************
const MouseStatus& ResolvedEvtList::getMouseStatus() const
{
	return win->resolveEvents_getMouse();
}

//**********************************************
const sButtonModifier& ResolvedEvtList::getBtnModifier() const
{
	return win->resolveEvents_getBtnModifier();
}