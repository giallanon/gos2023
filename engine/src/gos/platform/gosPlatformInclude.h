#ifndef _gosPlatformInclude_h_
#define _gosPlatformInclude_h_

#if defined(GOS_PLATFORM__LINUX)
    #include "linux/linuxOSInclude.h"
#elif defined(GOS_PLATFORM__WINDOWS)
    #include "win/winOSInclude.h"
#else
    WARNING: no define found for platform definition
#endif

#endif //_gosPlatformInclude_h_