#ifdef GOS_PLATFORM__WINDOWS
#include "winOS.h"
#include "../../memory/gosMemory.h"

struct sThreadInfo
{
	HANDLE 					winHandle;
	GOS_ThreadMainFunction	fn;
	void					*userData;
	gos::Allocator			*allocatorTS;
};

//*********************************************
DWORD WINAPI Win32InternalThreadProc (void *lpParameter)
{
	sThreadInfo *info = reinterpret_cast<sThreadInfo*>(lpParameter);

	GOS_ThreadMainFunction fn = info->fn;
	fn(info->userData);

	::CloseHandle(info->winHandle);
	GOSFREE(info->allocatorTS, info);
	return 0;
}


//*******************************************************************
eThreadError platform::createThread (OSThread *out_handle, gos::Allocator *allocatorTS, GOS_ThreadMainFunction threadFunction, UNUSED_PARAM(u32 stackSizeInKb), void *userParam)
{
	assert (allocatorTS->isThreadSafe());
	
	//info sul thread
	sThreadInfo *info = GOSALLOCSTRUCT(allocatorTS, sThreadInfo);
	info->fn = threadFunction;
	info->userData = userParam;
	info->allocatorTS = allocatorTS;
	


	u32 flag = 0;
	//if (bStartSuspended)	flag |= CREATE_SUSPENDED;
	info->winHandle = *out_handle = ::CreateThread (NULL, 0, Win32InternalThreadProc, info, flag, NULL);

	if (INVALID_HANDLE_VALUE == *out_handle)
		return eThreadError::unknown;

	return eThreadError::none;
}

/*******************************************************************
void platform::destroyThread (OSThread &handle)
{
	if (NULL == handle)
		return;
	::CloseHandle(handle);
	handle = NULL;
}*/

//*******************************************************************
void platform::waitThreadEnd (OSThread &handle)
{
	WaitForSingleObject (handle, INFINITE);
}

#endif //GOS_PLATFORM__WINDOWS

