#ifdef GOS_PLATFORM__LINUX
#include "linuxOS.h"
#include <limits.h>
#include <errno.h>
#include <string.h>
#include <time.h>


//*******************************************************************
eThreadError platform::createThread (OSThread &out_handle, OSThreadFunction threadFunction, u32 stackSizeInKb, void *userParam)
{
    pthread_attr_t  attr;
    int rc = pthread_attr_init(&attr);
    if (rc != 0)
        return eThreadError::unknown;

    size_t stackSizeInBytes = stackSizeInKb*1024;
    if (stackSizeInBytes < (u32)PTHREAD_STACK_MIN)
        stackSizeInBytes = (u32)PTHREAD_STACK_MIN;
    rc = pthread_attr_setstacksize(&attr, stackSizeInBytes);
    if (rc != 0)
        return eThreadError::invalidStackSize;

    rc = pthread_create(&out_handle, &attr, threadFunction, userParam);
    if (rc == 0)
        return eThreadError::none;

    switch (rc)
    {
        case EAGAIN:    return eThreadError::tooMany;
        default:        return eThreadError::unknown;
    }
}

//*******************************************************************
void platform::destroyThread (UNUSED_PARAM(OSThread &handle))
{
    //qui non c'e' nulla da fare.
    //E' solo ne caso di Windows che alla morte di un thread s'hanno da fare cose
}

//*******************************************************************
void platform::waitThreadEnd (OSThread &handle)
{
    void *ret = 0;
    pthread_join(handle, &ret);
}





#endif //GOS_PLATFORM__LINUX
