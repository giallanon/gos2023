#ifdef GOS_PLATFORM__LINUX
#include "linuxOS.h"
#include <limits.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <sys/time.h>
#include <sys/reboot.h>
#include <malloc.h>
#include "../../gosEnumAndDefine.h"
#include "../../gos.h"


struct	sLinuxPlatformData
{
    u32         memory_pageSize;
    u8	        *appPathNoSlash;
    u8	        *writableFolderPathNoSlash;
};
static sLinuxPlatformData	linuxPlatformData;

//*******************************************************************
bool platform::internal_init (const char *appName)
{
	memset (&linuxPlatformData, 0, sizeof(linuxPlatformData));

    //info sul sistema
    linuxPlatformData.memory_pageSize = sysconf(_SC_PAGE_SIZE);

	//usa la malloc per allocare il path
    linuxPlatformData.appPathNoSlash = (u8*)get_current_dir_name();
	gos::fs::pathSanitizeInPlace (linuxPlatformData.appPathNoSlash);

    {
        u32 n = strlen(reinterpret_cast<const char*>(linuxPlatformData.appPathNoSlash)) +2 + strlen(appName);
        char *temp = (char*)malloc(n);
        sprintf_s(temp, n, "%s/%s", linuxPlatformData.appPathNoSlash, appName);
        linuxPlatformData.writableFolderPathNoSlash = reinterpret_cast<u8*>(temp);
    }
	gos::fs::pathSanitizeInPlace (linuxPlatformData.writableFolderPathNoSlash);

    //console stuff
	if (!console_internal_init())
		return false;
	return true;
}

//*******************************************************************
void platform::internal_deinit()
{
	console_internal_deinit();
	
	if (linuxPlatformData.writableFolderPathNoSlash)
		::free(linuxPlatformData.writableFolderPathNoSlash);
	if (linuxPlatformData.appPathNoSlash)
		::free(linuxPlatformData.appPathNoSlash);
}

//**********************************************
const u8 *platform::getAppPathNoSlash()                                         { return linuxPlatformData.appPathNoSlash; }
const u8 *platform::getPhysicalPathToWritableFolder()				            { return linuxPlatformData.writableFolderPathNoSlash; }
void* platform::memory_alignedAlloc (size_t sizeInByte, size_t alignmentPowerOfTwo)
{ 
    assert(GOS_IS_POWER_OF_TWO(alignmentPowerOfTwo));
    return aligned_alloc(alignmentPowerOfTwo, sizeInByte); 
}
void platform::memory_alignedFree (void *p)								        { ::free(p); }
u32 platform::memory_getPageSizeInByte()                                        { return linuxPlatformData.memory_pageSize; }
u32 platform::systeminfo_getNumOfCPUCore()                                      { return sysconf(_SC_NPROCESSORS_ONLN); }

//**********************************************
u64 platform::getTimeNow_usec()
{
    struct timespec now;
    clock_gettime (CLOCK_MONOTONIC_RAW, &now);
    return (now.tv_sec * 1000000 + now.tv_nsec / 1000);
}


//*******************************************************************
void platform::sleep_msec (size_t msec)
{
    #define OSSLEEP_MSEC_TO_NANOSEC             1000000  // 1 millisec = 1.000.000 nanosec
    #define OSSLEEP_ONE_SECOND_IN_NANOSEC       1000000000

    //The value of the nanoseconds field must be in the range 0 to 999.999.999
    timespec sleepValue;
    sleepValue.tv_sec = 0;
    sleepValue.tv_nsec = msec * OSSLEEP_MSEC_TO_NANOSEC;
    while (sleepValue.tv_nsec >= OSSLEEP_ONE_SECOND_IN_NANOSEC)
    {
        sleepValue.tv_nsec -= OSSLEEP_ONE_SECOND_IN_NANOSEC;
        sleepValue.tv_sec++;
    }
    nanosleep (&sleepValue, NULL);
}


//*******************************************************************
void platform::getDateNow (u16 *out_year, u16 *out_month, u16 *out_day)
{
    time_t T = time(NULL);
    struct  tm tm = *localtime(&T);

    *out_year = tm.tm_year + 1900;
    *out_month = tm.tm_mon + 1;
    *out_day = tm.tm_mday;
}

//*******************************************************************
void platform::getTimeNow (u8 *out_hour, u8 *out_min, u8 *out_sec)
{
    time_t T = time(NULL);
    struct  tm tm = *localtime(&T);

    *out_hour = tm.tm_hour;
    *out_min = tm.tm_min;
    *out_sec = tm.tm_sec;
}


#endif //GOS_PLATFORM__LINUX
