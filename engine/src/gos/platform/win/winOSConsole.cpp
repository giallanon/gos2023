#ifdef GOS_PLATFORM__WINDOWS
#include "winOS.h"
#include "../../gosEnumAndDefine.h"


struct sWin32ConsoleData
{
	GOS_ConsoleTrap_CTRL_C	trapFN_CTRL_C;
	void					*trapFN_CTRL_C_userParam;
};

static sWin32ConsoleData	win32ConsoleData;

//*****************************************************
bool platform::console_internal_init()
{
	memset (&win32ConsoleData, 0, sizeof(win32ConsoleData));
	return true;
}

//*****************************************************
void platform::console_internal_deinit()
{
}

//*****************************************************
BOOL WINAPI  Win32Internal_consoleHandlerRoutine (DWORD dwCtrlType)
{
	switch (dwCtrlType)
	{
	case CTRL_C_EVENT:
		win32ConsoleData.trapFN_CTRL_C(win32ConsoleData.trapFN_CTRL_C_userParam);
		return TRUE;

	default:
		return FALSE;
	}
}

//*****************************************************
void platform::console_trap_CTRL_C (GOS_ConsoleTrap_CTRL_C trapFn, void *userParam)
{
	win32ConsoleData.trapFN_CTRL_C = trapFn;
	win32ConsoleData.trapFN_CTRL_C_userParam = userParam;
	SetConsoleCtrlHandler(Win32Internal_consoleHandlerRoutine, TRUE);
}

//*****************************************************
void platform::console_setTitle (const char *title)
{
	wchar_t wTitle[512];
	win32::utf8_towchar (reinterpret_cast<const u8*>(title), (u32)strlen(title), wTitle, sizeof(wTitle));
	::SetConsoleTitle (wTitle);
}


#endif //GOS_PLATFORM__WINDOWS
