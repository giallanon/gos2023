#ifdef GOS_PLATFORM__LINUX
#include "linuxOS.h"
#include "../../gosEnumAndDefine.h"

struct sLinuxConsoleData
{
	ConsoleTrap_CTRL_C	trapFN_CTRL_C;
	void				*trapFN_CTRL_C_userParam;
};

static sLinuxConsoleData	linuxConsoleData;

//*****************************************************
bool platform::console_internal_init()
{
	memset (&linuxConsoleData, 0, sizeof(sLinuxConsoleData));
	return true;
}

//*****************************************************
void platform::console_internal_deinit()
{
}

//*****************************************************
static void LinuxInternal_consoleHandlerRoutine (sig_atomic_t s)
{
	//printf ("SIGNAL %d\n", s);
	if (s == 2) // CTRL C
		linuxConsoleData.trapFN_CTRL_C(linuxConsoleData.trapFN_CTRL_C_userParam);
}

//*****************************************************
void platform::console_trap_CTRL_C (ConsoleTrap_CTRL_C trapFn, void *userParam)
{
	linuxConsoleData.trapFN_CTRL_C = trapFn;
	linuxConsoleData.trapFN_CTRL_C_userParam = userParam;
	signal(SIGINT, LinuxInternal_consoleHandlerRoutine);
}

//*****************************************************
void platform::console_setTitle (const char *title)
{
	printf ("\033]0;%s\007", title);
}
#endif //GOS_PLATFORM__LINUX
