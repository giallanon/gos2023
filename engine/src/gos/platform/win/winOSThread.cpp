#ifdef GOS_PLATFORM__WINDOWS
#include "winOS.h"

struct sThreadBootstrap
{
	platform::OSThreadFunction	fn;
	void						*userData;
};
sThreadBootstrap	threadBootstrap[32];
u32					nextThreadBootstrap = 0;

//*********************************************
DWORD WINAPI Win32InternalThreadProc (void *lpParameter)
{
	sThreadBootstrap *init = (sThreadBootstrap*)lpParameter;
	platform::OSThreadFunction fn = init->fn;
	void *userData = init->userData;

	//void *ret = fn(userData);
	fn(userData);
	return 0;
}


//*******************************************************************
eThreadError platform::createThread (OSThread &out_handle, OSThreadFunction threadFunction, UNUSED_PARAM(u32 stackSizeInKb), void *userParam)
{
	u32 flag = 0;
	//if (bStartSuspended)	flag |= CREATE_SUSPENDED;


	sThreadBootstrap *init = &threadBootstrap[nextThreadBootstrap++];
	if (nextThreadBootstrap == 32)
		nextThreadBootstrap = 0;

	init->fn = threadFunction;
	init->userData = userParam;
	out_handle = ::CreateThread(NULL, 0, Win32InternalThreadProc, (void*)init, flag, NULL);

	if (INVALID_HANDLE_VALUE == out_handle)
		return eThreadError::unknown;

	return eThreadError::none;
}

//*******************************************************************
void platform::destroyThread (OSThread &handle)
{
	if (NULL == handle)
		return;
	::CloseHandle(handle);
	handle = NULL;
}

//*******************************************************************
void platform::waitThreadEnd (OSThread &handle)
{
	WaitForSingleObject (handle, INFINITE);
}

#endif //GOS_PLATFORM__WINDOWS

