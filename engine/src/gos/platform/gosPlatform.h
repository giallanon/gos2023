#ifndef _gosPlatform_h_
#define _gosPlatform_h_

#if defined(GOS_PLATFORM__LINUX)
    #include "linux/linuxOS.h"
#elif defined(GOS_PLATFORM_WINDOWS)
    #include "win/winOS.h"
#else
    WARNING: no define found for platform definition
#endif

#endif //_gosPlatform_h_