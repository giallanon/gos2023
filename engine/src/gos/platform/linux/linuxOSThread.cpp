#ifdef GOS_PLATFORM__LINUX
#include "linuxOS.h"
#include <limits.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include "../../memory/gosMemory.h"

struct sThreadInfo
{
	GOS_ThreadMainFunction	    fn;
	void						*userData;
};

//*********************************************
void* LinuxInternalThreadProc (void *userParam)
{
    sThreadInfo *info = reinterpret_cast<sThreadInfo*>(userParam);

    GOS_ThreadMainFunction fn = info->fn;
	fn (info->userData);

    return NULL;
}

//*******************************************************************
eThreadError platform::createThread (OSThread *out_handle, gos::Allocator *allocatorTS, GOS_ThreadMainFunction threadFunction, u32 stackSizeInKb, void *userParam)
{
    assert (allocatorTS->isThreadSafe());
    sThreadInfo *info = GOSALLOCSTRUCT(allocatorTS, sThreadInfo);
	info->fn = threadFunction;
	info->userData = userParam;


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

    rc = pthread_create (out_handle, &attr, LinuxInternalThreadProc, info);
    if (rc == 0)
        return eThreadError::none;

    switch (rc)
    {
        case EAGAIN:    return eThreadError::tooMany;
        default:        return eThreadError::unknown;
    }
}

//*******************************************************************
void platform::waitThreadEnd (OSThread &handle)
{
    void *ret = 0;
    pthread_join(handle, &ret);
}





#endif //GOS_PLATFORM__LINUX
