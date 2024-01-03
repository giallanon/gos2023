#ifdef GOS_PLATFORM__LINUX
#include "linuxOS.h"
#include <limits.h>
#include <errno.h>
#include <string.h>
#include <time.h>

struct sThreadBootstrap
{
	GOS_ThreadMainFunction	    fn;
	void						*userData;
};
static sThreadBootstrap	threadBootstrap[32];
static u32				nextThreadBootstrap = 0;


//*********************************************
void* LinuxInternalThreadProc (void *userParam)
{
	sThreadBootstrap *init = reinterpret_cast<sThreadBootstrap*>(userParam);
	
    GOS_ThreadMainFunction fn = init->fn;
	fn (init->userData);

    return NULL;
}

//*******************************************************************
eThreadError platform::createThread (OSThread *out_handle, GOS_ThreadMainFunction threadFunction, u32 stackSizeInKb, void *userParam)
{
	sThreadBootstrap *init = &threadBootstrap[nextThreadBootstrap++];
	if (nextThreadBootstrap == 32)
		nextThreadBootstrap = 0;
	init->fn = threadFunction;
	init->userData = userParam;


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

    rc = pthread_create (out_handle, &attr, LinuxInternalThreadProc, init);
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
