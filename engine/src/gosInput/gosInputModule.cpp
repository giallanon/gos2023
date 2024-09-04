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
    contextList.setup (localAllocator, 32);
    actionNameList.setup (localAllocator, 65*1024);
    hashListOfActionNames.setup (localAllocator, 256);
}

//***************************************
Module::~Module()
{
    windowList.unsetup();

    const u32 n = contextList.getNElem();
    for (u32 i=0; i<n; i++)
    {
        Context *ctx = contextList[i];
        GOSDELETE (localAllocator, ctx);
    }    
    contextList.unsetup();

    actionNameList.unsetup();
    hashListOfActionNames.unsetup();
}

//***************************
u32 Module::action_addName (const char *name)
{ 
    const u32 hash = gos::utils::crc32(name);
    u32 offset;

    SortedFastArray<u32, u32>::Position pos;

    if (!hashListOfActionNames.find (hash, &offset, &pos))
    {
        offset = actionNameList.add (name); 
        hashListOfActionNames.insertInPosition (pos, offset);
    }
    return offset;
}

//***************************
Context* Module::context_create (const char *contextName)
{
    for (u32 i=0; i<contextList.getNElem(); i++)
    {
        if (strcmp (contextList(i)->getName(), contextName) == 0)
        {
            gos::logger::err ("input::Module::context_create(%s) => ctx already exists\n", contextName);
            return NULL;
        }
    }

    Context *c = GOSNEW(localAllocator, Context) (localAllocator, contextName);
    contextList.append (c);
    return c;
}


//***************************
const char* Module::context_getActionNameByIndex (u32 iCtx, u32 iAction) const
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
u32 Module::context_getActionIDByIndex (u32 iCtx, u32 iAction) const
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
const char* Module::context_getName (u32 iCtx) const
{
    assert(iCtx<context_getNum()); 
    return contextList(iCtx)->getName();
}

//***************************
u32 Module::context_getNumAction (u32 iCtx) const
{
    assert(iCtx<context_getNum()); 
    return contextList(iCtx)->action_getNum();
}


//***************************
void Module::logAllMappedInput() const
{
    u8 buffer[1024];
    FastArray<EventID> eventIDList;
    eventIDList.setupWithBase (buffer, sizeof(buffer), gos::getScrapAllocator());

    gos::logger::log (eTextColor::yellow, "---------------------------------------\n");
    gos::logger::log (eTextColor::yellow, "input::Module::logAllMappedInput()\n");
    gos::logger::log (eTextColor::yellow, "---------------------------------------\n");

    gos::logger::incIndent();
    for (u32 i=0; i<contextList.getNElem(); i++)
        contextList(i)->logAllMappedInput();
    gos::logger::decIndent();
}

