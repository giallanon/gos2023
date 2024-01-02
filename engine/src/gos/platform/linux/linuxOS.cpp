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
#include <pwd.h>
#include <linux/wireless.h>
#include "../../gosEnumAndDefine.h"
#include "../../gos.h"


struct	sLinuxPlatformData
{
    u32         memory_pageSize;
    u8	        *appPathNoSlash;
    u8	        *userFolderPathNoSlash;
};
static sLinuxPlatformData	linuxPlatformData;

//*******************************************************************
bool platform::internal_init (const char *appName)
{
	memset (&linuxPlatformData, 0, sizeof(linuxPlatformData));

    //info sul sistema
    linuxPlatformData.memory_pageSize = sysconf(_SC_PAGE_SIZE);

	//get_current_dir_name() usa la malloc per allocare il path
    linuxPlatformData.appPathNoSlash = (u8*)get_current_dir_name();
	gos::fs::pathSanitizeInPlace (linuxPlatformData.appPathNoSlash);

    //recupero il path della user folder
    const char *homedir = getenv("HOME");
    if (NULL == homedir)
        homedir = getpwuid(getuid())->pw_dir;

    {
        u32 n = strlen(homedir);
        char *temp = (char*)malloc(n + 1);
        sprintf_s (temp, n+1, "%s", homedir);
        linuxPlatformData.userFolderPathNoSlash = reinterpret_cast<u8*>(temp);
    }
	gos::fs::pathSanitizeInPlace (linuxPlatformData.userFolderPathNoSlash);

    //console stuff
	if (!console_internal_init())
		return false;
	return true;
}

//*******************************************************************
void platform::internal_deinit()
{
	console_internal_deinit();
	
	if (linuxPlatformData.userFolderPathNoSlash)
		::free(linuxPlatformData.userFolderPathNoSlash);
	if (linuxPlatformData.appPathNoSlash)
		::free(linuxPlatformData.appPathNoSlash);
}

//**********************************************
const u8 *platform::getAppPathNoSlash()                                         { return linuxPlatformData.appPathNoSlash; }
const u8 *platform::getPhysicalPathToUserFolder()				                { return linuxPlatformData.userFolderPathNoSlash; }
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


//*******************************************************************
gos::NetworkAdapterInfo* platform::NET_getListOfAllNerworkAdpaterIPAndNetmask (gos::Allocator *allocator, u32 *out_nRecordFound)
{
    struct ifaddrs * ifAddrStruct=NULL;
    struct ifaddrs * ifa=NULL;

    *out_nRecordFound = 0;
    getifaddrs(&ifAddrStruct);
    for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next)
    {
        if (!ifa->ifa_addr)
            continue;
        if (ifa->ifa_addr->sa_family == AF_INET)
            (*out_nRecordFound)++;
    }


    if (0 == (*out_nRecordFound))
        return NULL;

    u32 ct = 0;
	gos::NetworkAdapterInfo *ret = (gos::NetworkAdapterInfo*)GOSALLOC(allocator, sizeof(gos::NetworkAdapterInfo) * (*out_nRecordFound));
	memset(ret, 0, sizeof(gos::NetworkAdapterInfo) * (*out_nRecordFound));

    for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next)
    {
        if (!ifa->ifa_addr)
            continue;

        if (ifa->ifa_addr->sa_family == AF_INET)
        {
            sprintf_s (ret[ct].name, sizeof(ret[ct].name), "%s", ifa->ifa_name);
            void *tmpAddrPtr = &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
            inet_ntop(AF_INET, tmpAddrPtr, ret[ct].ip, INET_ADDRSTRLEN);

            tmpAddrPtr = &((struct sockaddr_in *)ifa->ifa_netmask)->sin_addr;
            inet_ntop(AF_INET, tmpAddrPtr, ret[ct].subnetMask, INET_ADDRSTRLEN);

            //printf("%s IP Address %s %s\n", ifa->ifa_name, ip, netmask);
            ct++;
        }
        /*else if (ifa->ifa_addr->sa_family == AF_INET6)
        { 	// check it is IP6
            // is a valid IP6 Address
            tmpAddrPtr=&((struct sockaddr_in6 *)ifa->ifa_addr)->sin6_addr;
            char addressBuffer[INET6_ADDRSTRLEN];
            inet_ntop(AF_INET6, tmpAddrPtr, addressBuffer, INET6_ADDRSTRLEN);
            printf("%s IP Address %s\n", ifa->ifa_name, addressBuffer);
        }
        */
    }
    if (ifAddrStruct!=NULL)
        freeifaddrs(ifAddrStruct);
    return ret;
}

//*******************************************************************
bool linuxPlatorm_check_if_wireless (const char* ifname)
{
    struct iwreq pwrq;
    memset(&pwrq, 0, sizeof(pwrq));
    strncpy(pwrq.ifr_name, ifname, IFNAMSIZ);

    int sock = -1;
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        return false;

    if (ioctl(sock, SIOCGIWNAME, &pwrq) != -1) 
    {
        close(sock);
        return true;
    }

    close (sock);
    return false;
}

//*******************************************************************
bool platform::NET_getMACAddress (gos::MacAddress *outMAC, gos::IPv4 *outIP)
{
    //scanno tutte le interfacce di rete disponibili e scelgo preferibilmente quella ethernet rispetto a quella via cavo
    char  nomeInterfaccia[64];
    memset (nomeInterfaccia,0, sizeof(nomeInterfaccia));

    struct ifaddrs *addrList;
    getifaddrs(&addrList);
    ifaddrs *addr = addrList;
    while (addr)
    {
        if (addr->ifa_addr) 
        {
            if (AF_PACKET == addr->ifa_addr->sa_family)
            {
                //printf ("found %s [is wifi? %c]\n", addr->ifa_name, bIsWiFi ? 'Y':'N');
                
                //skippo loopback
                if (strcasecmp(addr->ifa_name, "lo") != 0)
                {
                    sprintf_s (nomeInterfaccia, sizeof(nomeInterfaccia), "%s", addr->ifa_name);
                    const bool bIsWiFi = linuxPlatorm_check_if_wireless (addr->ifa_name);
                    if (!bIsWiFi)
                        break;
                }
            }
        }

        addr = addr->ifa_next;
    }
    freeifaddrs(addrList);


    //recpuero info sull'interfaccia selezionata
    assert (NULL != outMAC);
    assert (NULL != outIP);
    memset (outMAC, 0, sizeof(gos::MacAddress));
    memset (outIP, 0, sizeof(gos::IPv4));

	int fd = socket (PF_INET, SOCK_DGRAM, IPPROTO_IP);

	struct ifreq s;
    strcpy(s.ifr_name, nomeInterfaccia);
    if (0 == ioctl(fd, SIOCGIFHWADDR, &s))
    {
        outMAC->b[0] = (unsigned char)s.ifr_addr.sa_data[0];
        outMAC->b[1] = (unsigned char)s.ifr_addr.sa_data[1];
        outMAC->b[2] = (unsigned char)s.ifr_addr.sa_data[2];
        outMAC->b[3] = (unsigned char)s.ifr_addr.sa_data[3];
        outMAC->b[4] = (unsigned char)s.ifr_addr.sa_data[4];
        outMAC->b[5] = (unsigned char)s.ifr_addr.sa_data[5];

        if (0 == ioctl(fd, SIOCGIFADDR, &s))
        {
            sockaddr_in *sin = reinterpret_cast<sockaddr_in*>(&s.ifr_addr);

            char ip[32];
            inet_ntop(AF_INET, &sin->sin_addr, ip, INET_ADDRSTRLEN);

            gos::netaddr::ipstrToIPv4  (ip, outIP);
            return true;
        }
    }
	return false;
}

#endif //GOS_PLATFORM__LINUX
