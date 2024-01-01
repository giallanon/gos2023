#ifndef _gosWaitableGrp_h_
#define _gosWaitableGrp_h_
#include "gosWaitableGrpInterface.h"

#ifdef GOS_PLATFORM__WINDOWS
	#include "platform/win/winOSWaitableGrp.h"
#endif
#ifdef GOS_PLATFORM__LINUX
	#include "platform/linux/winOSWaitableGrp.h"
#endif


namespace gos
{
	typedef platform::OSWaitableGrp	WaitableGrp;
} //namespace gos

#endif //_gosWaitableGrp_h_