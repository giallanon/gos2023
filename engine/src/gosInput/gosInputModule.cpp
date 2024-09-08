#include "gosInputModule.h"
#include "gosInput.h"
#include "../gos/gos.h"
#include "../gos/gosUtils.h"

using namespace gos;
using namespace gos::input;

//***************************************
Module::Module(gos::Allocator *allocatorIN) : voidEvtList(1)
{
    localAllocator = allocatorIN;
    windowList.setup (localAllocator);
}

//***************************************
Module::~Module()
{
    windowList.unsetup();
}


