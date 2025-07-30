#include "gosErr.h"
#include "gos.h"

using namespace gos;


//*************************************************
MultiThreadErrHandler::ThreadErr::ThreadErr()
{ 
    errCount = 0;
    offset = 0;
    buffer.setupWithBase (baseMemBlock, sizeof(baseMemBlock), gos::getSysHeapAllocator()); 
}

//*************************************************
void MultiThreadErrHandler::ThreadErr::vadd (const char *format, va_list argptr)
{
    char msg[1024];
    vsnprintf (msg, sizeof(msg), format, argptr);
    const u16 msgLen = static_cast<u16>(1 + strlen(msg));
    errCount++;
    buffer.append (&msgLen, &offset, 2, true);
    buffer.append (msg, &offset, msgLen, true);
}

//************************************** 
const char* MultiThreadErrHandler::ThreadErr::getErrByIndex (u32 i) const
{
    if (i >= errCount)
    {
        DBGBREAK;
        return NULL;
    }

    u32 pos = 0;
    while (i--)
    {
        u16 msgLen = 0;
        buffer.read (&msgLen, pos, 2);
        pos += 2 + msgLen;
    }

    return reinterpret_cast<const char*>(buffer._getPointer (pos+2));
}


//************************************** 
MultiThreadErrHandler::MultiThreadErrHandler()
{
    sprintf_s (NULLSTR, sizeof(NULLSTR), "NULL");
    memset (handlerList, 0x00, sizeof(handlerList));
    thread::mutexCreate (&mutex);
}

//************************************** 
MultiThreadErrHandler::~MultiThreadErrHandler()
{
    for (u32 i=0; i<NUM_MAX_HANDLER; i++)
    {
        if (handlerList[i].err)
        {
            delete handlerList[i].err;
            handlerList[i].err = NULL;
        }
    }
    thread::mutexDestroy (mutex);
}

//************************************** 
void MultiThreadErrHandler::deleteThisHandlerIfExists (u32 threadID)
{
    MUTEX_LOCK(mutex);
    for (u32 i=0; i<NUM_MAX_HANDLER; i++)
    {
        if (handlerList[i].threadID == threadID)
        {
            delete handlerList[i].err;
            handlerList[i].threadID = 0;
            handlerList[i].err = NULL;
            break;
        }
    }
    MUTEX_UNLOCK(mutex);
}


//**************************************
MultiThreadErrHandler::ThreadErr* MultiThreadErrHandler::exists (u32 threadID) const
{
    for (u32 i=0; i<NUM_MAX_HANDLER; i++)
    {
        if (handlerList[i].threadID == threadID)
            return handlerList[i].err;
    }
    return NULL;
}

//**************************************
MultiThreadErrHandler::ThreadErr* MultiThreadErrHandler::create (u32 threadID)
{
    MUTEX_LOCK(mutex);
    for (u32 i=0; i<NUM_MAX_HANDLER; i++)
    {
        if (NULL == handlerList[i].err)
        {
            handlerList[i].threadID = threadID;
            handlerList[i].err = new ThreadErr();

            MUTEX_UNLOCK(mutex);
            return handlerList[i].err;
        }
    }

    MUTEX_UNLOCK(mutex);
    DBGBREAK;
    return NULL;
}

//************************************** 
void MultiThreadErrHandler::clear (u32 threadID)
{
    ThreadErr *e = exists (threadID);
    if (e)
        e->reset();
}

//************************************** 
u32 MultiThreadErrHandler::getErrCount(u32 threadID)
{
    const ThreadErr *e = exists (threadID);
    if (e)
        return e->getCount();
    return 0;
}


//************************************** 
const char* MultiThreadErrHandler::getErrByIndex (u32 threadID, u32 i)
{
    const char *ret = NULL;

    const ThreadErr *e = exists (threadID);
    if (e)
        ret = e->getErrByIndex(i);

    if (NULL == ret)
        return NULLSTR;
    return ret;
}

//************************************** 
void MultiThreadErrHandler::vadd (u32 threadID, const char *format, va_list argptr)
{
    //questo e' l'unico caso in cui se l'oggetto ERR non esiste per il thread in questione
    //lo vado davero a creare
    ThreadErr *e = exists (threadID);
    if (NULL == e)
        e = create (threadID);

    e->vadd (format, argptr);
}