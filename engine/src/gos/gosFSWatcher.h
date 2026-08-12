#ifndef _gosFSWatcher_h_
#define _gosFSWatcher_h_
#include "gosFSWatcherInterface.h"

#ifdef GOS_PLATFORM__WINDOWS
	#include "platform/win/winOSFSWatcher.h"
#endif
#ifdef GOS_PLATFORM__LINUX
	#include "platform/linux/linuxOSFSWatcher.h"
#endif


namespace gos
{
	typedef platform::OSFSWatcher	FSWatcher;
} //namespace gos

#endif //_gosFSWatcher_h_

