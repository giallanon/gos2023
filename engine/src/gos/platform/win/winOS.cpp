#ifdef GOS_PLATFORM__WINDOWS
#include "winOS.h"
#include <mbstring.h>
#include <shlobj.h>
#include "../../gos.h"

#pragma comment(lib, "Ws2_32.lib")

struct	sWin32PlatformData
{
	HINSTANCE			hInst;
	u8					applicationPathNoSlash[256];
	u8					writableFolderPathNoSlash[256];
	wchar_t				chromeFullPathAndName[256];
	u64					hiresTimerFreqMSec;
	u64					hiresTimerFreqUSec;
	uint64_t			timeStarted;
	WSADATA				wsaData;
};

static sWin32PlatformData	win32PlatformData;



//********************************************* 
bool win32::utf8_towchar (const u8 *utf8_string, u32 nBytesToUse, wchar_t *out, u32 sizeInBytesOfOut)
{
	int n;
	if (nBytesToUse == u32MAX)
		n = -1;
	else
		n = (int)nBytesToUse;

	int result = MultiByteToWideChar (CP_UTF8, 0, (const char*)utf8_string, n, out, (sizeInBytesOfOut >> 1));
	if (0 == result)
	{
		DBGBREAK;
		return false;
	}

	if (out[result] != 0x00)
		out[result] = 0x00;

	return true;
}

//********************************************* 
bool win32::wchar_to_utf8 (const wchar_t *wstring, u32 nBytesToUse, u8 *out, u32 sizeInBytesOfOut)
{
	int n;
	if (nBytesToUse == u32MAX)
		n = -1;
	else
		n = (int)nBytesToUse/2;

	int bytesWriten = WideCharToMultiByte  (CP_UTF8, 0, wstring, n, (char*)out, sizeInBytesOfOut, 0, 0);
	if (0 != bytesWriten)
	{
		if (n != -1)
			out[bytesWriten] = 0;
		return true;
	}

	DBGBREAK;
	return false;
}

//**********************************************
bool platform::internal_init (const char *appName)
{
	memset(&win32PlatformData, 0, sizeof(win32PlatformData));

	win32PlatformData.hInst = GetModuleHandle(NULL);

	//timer
	u64 timerFreq;
	QueryPerformanceFrequency((LARGE_INTEGER*)&timerFreq);
	win32PlatformData.hiresTimerFreqUSec = timerFreq / 1000000; //per avere il time in microsec
	win32PlatformData.hiresTimerFreqMSec = timerFreq / 1000; //per avere il time in msec
	
	QueryPerformanceCounter((LARGE_INTEGER*)&win32PlatformData.timeStarted);

	//application path
	wchar_t wcString[MAX_PATH];
	GetModuleFileName (win32PlatformData.hInst, wcString, MAX_PATH);
	
	u8 temp_utf8[512];
	win32::wchar_to_utf8 (wcString, u32MAX, temp_utf8, sizeof(temp_utf8));
	gos::fs::pathSanitizeInPlace (temp_utf8);
	gos::fs::extractFilePathWithOutSlash (temp_utf8, win32PlatformData.applicationPathNoSlash, sizeof(win32PlatformData.applicationPathNoSlash));


	//writable folder path
	SHGetFolderPath(NULL, CSIDL_APPDATA | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, wcString);
	{
		size_t	n = wcslen(wcString);
		for (size_t t = 0; t < n; t++)
		{
			if (wcString[t] == '\\')
				wcString[t] = '/';
		}

		win32::wchar_to_utf8 (wcString, u32MAX, win32PlatformData.writableFolderPathNoSlash, sizeof(win32PlatformData.writableFolderPathNoSlash));
		gos::fs::pathSanitizeInPlace (win32PlatformData.writableFolderPathNoSlash); 
	}

	strcat_s((char*)win32PlatformData.writableFolderPathNoSlash, sizeof(win32PlatformData.writableFolderPathNoSlash), "/");
	strcat_s((char*)win32PlatformData.writableFolderPathNoSlash, sizeof(win32PlatformData.writableFolderPathNoSlash), appName);
	gos::fs::folderCreate(win32PlatformData.writableFolderPathNoSlash);


	//initialize Winsock
	if (0 != WSAStartup(MAKEWORD(2, 2), &win32PlatformData.wsaData))
		return false;

	//init console stuff
	if (!console_internal_init())
		return false;

	return true;
}

//**********************************************
void platform::internal_deinit()
{
	console_internal_deinit();
	
	WSACleanup();
}

//**********************************************
u32 platform::systeminfo_getNumOfCPUCore()
{
	SYSTEM_INFO si;
	GetSystemInfo (&si);
	return si.dwNumberOfProcessors;
}

//**********************************************
u32 platform::memory_getPageSizeInByte()
{
	SYSTEM_INFO si;
	GetSystemInfo (&si);
	return si.dwPageSize;
}

//**********************************************
void* platform::memory_alignedAlloc (size_t size, size_t alignmentPowerOfTwo)
{
	void *p = _aligned_malloc(size, alignmentPowerOfTwo);
	assert(NULL != p);
	return p;
}

//**********************************************
void platform::memory_alignedFree (void *p)
{
	_aligned_free(p);
}


//**********************************************
const u8 *platform::getAppPathNoSlash()						{ return win32PlatformData.applicationPathNoSlash; }
const u8 *platform::getPhysicalPathToWritableFolder()		{ return win32PlatformData.writableFolderPathNoSlash; }
void platform::sleep_msec(size_t msec)						{ ::Sleep((u32)msec); }

//**********************************************
u64 platform::getTimeNow_usec()
{
	uint64_t	timeNow;
	QueryPerformanceCounter((LARGE_INTEGER*)&timeNow);
	timeNow -= win32PlatformData.timeStarted;
	timeNow /= win32PlatformData.hiresTimerFreqUSec; //time in msec
	return timeNow;
}
//*******************************************************************
void platform::getDateNow(u16 *out_year, u16 *out_month, u16 *out_day)
{
	SYSTEMTIME s;
	GetLocalTime(&s);

	(*out_year) = (u16)s.wYear;
	(*out_month) = (u16)s.wMonth;
	(*out_day) = (u16)s.wDay;
}

//*******************************************************************
void platform::getTimeNow(u8 *out_hour, u8 *out_min, u8 *out_sec)
{
	SYSTEMTIME s;
	GetLocalTime(&s);

	(*out_hour) = (u8)s.wHour;
	(*out_min) = (u8)s.wMinute;
	(*out_sec) = (u8)s.wSecond;
}

//*******************************************************************
#include "Iphlpapi.h"
#include "ws2tcpip.h"
#pragma comment(lib, "IPHLPAPI.lib")
gos::NetworkAdapterInfo* platform::NET_getListOfAllNerworkAdpaterIPAndNetmask (gos::Allocator *allocator, u32 *out_nRecordFound)
{
	*out_nRecordFound = 0;

	gos::Allocator *tempAllocator = gos::getScrapAllocator();

	// Before calling AddIPAddress we use GetIpAddrTable to get an adapter to which we can add the IP.
	DWORD dwTableSize = sizeof(MIB_IPADDRTABLE);
	PMIB_IPADDRTABLE pIPAddrTable = (MIB_IPADDRTABLE *)RHEAALLOC(tempAllocator,dwTableSize);
	if (GetIpAddrTable(pIPAddrTable, &dwTableSize, 0) == ERROR_INSUFFICIENT_BUFFER)
	{
		RHEAFREE(tempAllocator, pIPAddrTable);
		pIPAddrTable = (MIB_IPADDRTABLE *)RHEAALLOC(tempAllocator, dwTableSize);
	}

	// Make a second call to GetIpAddrTable to get the actual data we want
	DWORD err = GetIpAddrTable(pIPAddrTable, &dwTableSize, 0);
	if (err != NO_ERROR) 
	{
		/*
		LPVOID lpMsgBuf;
		if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)& lpMsgBuf, 0, NULL))
		{
			printf("\tError: %s", (const char*)lpMsgBuf);
			LocalFree(lpMsgBuf);
		}
		*/
		return NULL;
	}

	if (pIPAddrTable->dwNumEntries == 0)
		return NULL;
	if (pIPAddrTable->dwNumEntries > 250)
		*out_nRecordFound = 250;
	else
		*out_nRecordFound = (u8)pIPAddrTable->dwNumEntries;
	
	gos::NetworkAdapterInfo *ret = (gos::NetworkAdapterInfo*)RHEAALLOC(allocator, sizeof(gos::NetworkAdapterInfo) * (*out_nRecordFound));
	memset(ret, 0, sizeof(gos::NetworkAdapterInfo) * (*out_nRecordFound));
	for (u8 i = 0; i < (*out_nRecordFound); i++)
	{
		IN_ADDR IPAddr;

		//printf("\n\tInterface Index[%d]:\t%ld\n", i, pIPAddrTable->table[i].dwIndex);
		sprintf_s (ret[i].name, sizeof(ret->name), "ip%d", i);
		
		IPAddr.S_un.S_addr = (u_long)pIPAddrTable->table[i].dwAddr;
		InetNtopA(AF_INET, &IPAddr, ret[i].ip, sizeof(ret[i].ip));
		//printf("\tIP Address[%d]:     \t%s\n", i, ret[i].ip);
		
		IPAddr.S_un.S_addr = (u_long)pIPAddrTable->table[i].dwMask;
		InetNtopA(AF_INET, &IPAddr, ret[i].subnetMask, sizeof(ret[i].subnetMask));
		//printf("\tSubnet Mask[%d]:    \t%s\n", i, ret[i].subnetMask);
		
		/*IPAddr.S_un.S_addr = (u_long)pIPAddrTable->table[i].dwBCastAddr;
		printf("\tBroadCast[%d]:      \t%s (%ld)\n", i, InetNtop(AF_INET, &IPAddr, sAddr, sizeof(sAddr)), pIPAddrTable->table[i].dwBCastAddr);
		
		printf("\tReassembly size[%d]:\t%ld\n", i, pIPAddrTable->table[i].dwReasmSize);
		
		printf("\tType and State[%d]:", i);
		if (pIPAddrTable->table[i].wType & MIB_IPADDR_PRIMARY)
			printf("\tPrimary IP Address");
		
		if (pIPAddrTable->table[i].wType & MIB_IPADDR_DYNAMIC)
			printf("\tDynamic IP Address");
		
		if (pIPAddrTable->table[i].wType & MIB_IPADDR_DISCONNECTED)
			printf("\tAddress is on disconnected interface");
		
		if (pIPAddrTable->table[i].wType & MIB_IPADDR_DELETED)
			printf("\tAddress is being deleted");
		
		if (pIPAddrTable->table[i].wType & MIB_IPADDR_TRANSIENT)
			printf("\tTransient address");*/
		//printf("\n");
	}

	if (pIPAddrTable) 
		RHEAFREE(tempAllocator, pIPAddrTable);

	return ret;
}

//*******************************************************************
bool platform::NET_getMACAddress (gos::MacAddress *outMAC, gos::IPv4 *outIP)
{
	assert (NULL != outMAC);
	assert (NULL != outIP);
	memset (outMAC, 0, sizeof(gos::MacAddress));
	memset (outIP, 0, sizeof(gos::IPv4));

	gos::Allocator *allocator = gos::getScrapAllocator();

	PIP_ADAPTER_INFO AdapterInfo;
	AdapterInfo = (IP_ADAPTER_INFO *)RHEAALLOC(allocator, sizeof(IP_ADAPTER_INFO));
	if (AdapterInfo == NULL)
	{
		DBGBREAK;
		return false;
	}

	// Make an initial call to GetAdaptersInfo to get the necessary size into the dwBufLen variable
	DWORD dwBufLen = sizeof(IP_ADAPTER_INFO);
	if (ERROR_BUFFER_OVERFLOW == GetAdaptersInfo(AdapterInfo, &dwBufLen))
	{
		RHEAFREE(allocator, AdapterInfo);
		AdapterInfo = (IP_ADAPTER_INFO *)RHEAALLOC(allocator, dwBufLen);
		if (AdapterInfo == NULL)
		{
			DBGBREAK;
			return false;
		}
	}

	u32 whichAdapterTypeWasFound = 0;
	if (GetAdaptersInfo(AdapterInfo, &dwBufLen) == NO_ERROR)
	{
		// Contains pointer to current adapter info
		PIP_ADAPTER_INFO pAdapterInfo = AdapterInfo;
		while (pAdapterInfo)
		{
			if (strcmp(pAdapterInfo->IpAddressList.IpAddress.String, "0.0.0.0") != 0)
			{
				if ((pAdapterInfo->Type == 6 || pAdapterInfo->Type == 71) && pAdapterInfo->DhcpEnabled == 1)
				{
					if (strcmp(pAdapterInfo->GatewayList.IpAddress.String, "0.0.0.0") != 0)
					{
						//sembra un buon candidato.
						//Se il Type==6 vuol dire che � una interfaccia di rete via cavo, se Type==71 vuol dire che � un wifi
						//Nel caso io abbia entrambe le connessioni, favorisco quella di rete via cavo
						bool bUseThis = false;
						switch (whichAdapterTypeWasFound)
						{
						case 0:
							bUseThis = true;
							break;

						case 6: //ho gi� trovato un interfaccia di rete via cavo, mi tengo quella
							bUseThis = false;
							break;

						case 71: //l'adapter precedente era un wifi, quello nuovo � una LAN, uso il nuovo
							if (pAdapterInfo->Type == 6)
								bUseThis = true;
							break;
						}
						
						if (bUseThis)
						{
							whichAdapterTypeWasFound = pAdapterInfo->Type;
							outMAC->b[0] = pAdapterInfo->Address[0];
							outMAC->b[1] = pAdapterInfo->Address[1];
							outMAC->b[2] = pAdapterInfo->Address[2];
							outMAC->b[3] = pAdapterInfo->Address[3];
							if (pAdapterInfo->AddressLength == 6)
							{
								outMAC->b[4] = pAdapterInfo->Address[4];
								outMAC->b[5] = pAdapterInfo->Address[5];
							}
							else
							{
								outMAC->b[4] = 0;
								outMAC->b[5] = 0;
							}

							gos::netaddr::ipstrToIPv4 (pAdapterInfo->IpAddressList.IpAddress.String, outIP);
						}
					}
				}
			}

			pAdapterInfo = pAdapterInfo->Next;
		}
	}
	RHEAFREE(allocator, AdapterInfo);
	return (whichAdapterTypeWasFound != 0);
}

#endif //GOS_PLATFORM__WINDOWS

