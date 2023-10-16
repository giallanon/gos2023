#ifdef GOS_PLATFORM__WINDOWS
#ifndef _winOSConsole_h_
#define _winOSConsole_h_
#include "winOSInclude.h"
#include "../../rheaEnumAndDefine.h"

namespace platform
{
	bool            console_internal_init();
	void			console_internal_deinit();

	void			console_trap_CTRL_C (ConsoleTrap_CTRL_C trapFn, void *userParam);
	void			console_setTitle (const char *title);
}


#endif // _winOSConsole_h_
#endif // GOS_PLATFORM__WINDOWS
