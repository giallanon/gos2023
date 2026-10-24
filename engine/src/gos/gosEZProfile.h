#ifndef _gosEZProfile_h_
#define _gosEZProfile_h_
#include "dataTypes/gosTimer.h"


#define GOS_PROFILE_BEGIN(name)\
		static gos::TimerFPS gos_profiler__##name;\
		gos_profiler__##name.onFrameBegin();\


#define GOS_PROFILE_END(name)\
	if (gos_profiler__##name.onFrameEnd())\
	{\
		logger::log (eTextColor::blue, "GOSPROFILE[" #name "]: %.2f ms\n", gos_profiler__##name.getAvgFrameTime_ms());\
	}\



#endif