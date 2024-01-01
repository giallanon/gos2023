#ifdef GOS_PLATFORM__WINDOWS
#ifndef _winOSInclude_h_
#define _winOSInclude_h_

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <assert.h>
#include <Winsock2.h>
#include <ws2tcpip.h>
#include <shellapi.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <inttypes.h>
#include <conio.h>

//typedef dei dati di base
typedef int8_t      i8;         //8 bit signed
typedef uint8_t     u8;         //8 bit unsigned
typedef int16_t     i16;        //16 bit signed
typedef uint16_t    u16;        //16 bit unsigned
typedef int32_t     i32;        //....
typedef uint32_t    u32;
typedef int64_t     i64;
typedef uint64_t    u64;
typedef float       f32;        //32 bit floating point
typedef double      f64;        //64 bit floating point

typedef uintptr_t   uiPtr;      //un "intero" la cui dimensione in byte dipende dalla piattaforma, ma che è sempre in grado di ospitare un puntatore

#define	u8MAX		0xFF
#define i8MIN		(-127 - 1)
#define i8MAX       127

#define	u16MAX      0xFFFF
#define i16MIN		(-32767 - 1)
#define i16MAX      32767

#define	u32MAX      0xFFFFFFFF
#define i32MIN		(-2147483647 - 1)
#define i32MAX      2147483647

#define	u64MAX      0xFFFFFFFFFFFFFFFF
#define i64MIN		(-9223372036854775807 - 1)
#define i64MAX      9223372036854775807


//#pragma warning(disable:26495)	//Variable 'sOSFileFind::findData' is uninitialized.Always initialize a member variable(type.6)
//#pragma warning(disable:4458)	//declaration of 'allocator' hides class member	

/***********************************************
 * debug helpers
 */
#ifdef _DEBUG
	#define	DBGBREAK	__debugbreak();
#else
	#define	DBGBREAK
#endif


/***********************************************
 * c/c++ portability stuff
 */
#define UNUSED_PARAM(x) __pragma(warning(suppress:4100)) x
#define ALIGNOF(x)	__alignof(x)

#define strncasecmp _strnicmp
#define strcasecmp _stricmp
#define strtok_r strtok_s

namespace platform
{
    typedef HANDLE			OSFile;
	typedef HANDLE			OSThread;
	typedef void* (*OSThreadFunction)(void *userParam);

	typedef struct sOSCriticalSection
	{
		CRITICAL_SECTION cs;
	} OSMutex;

	typedef struct sOSFileFind
	{
		HANDLE			h;
		WIN32_FIND_DATA	findData;
		u8				utf8_jolly[64];
		u8				utf8_curFilename[512];

						sOSFileFind()					{h = INVALID_HANDLE_VALUE; }
	} OSFileFind;

	typedef struct sOSDriveEnumerator
	{
		u32	logicalDrives;
		u8	current;

		sOSDriveEnumerator() { logicalDrives = 0; current = 0; }
	} OSDriveEnumerator;

	struct OSEvent
	{
		HANDLE	h;
	};

	struct OSSocket
	{
		u32             readTimeoutMSec;
		SOCKET          socketID;
	};

} //namespace platform


namespace win32
{
	bool			utf8_towchar (const u8 *utf8_string, u32 nBytesToUse, wchar_t* out, u32 sizeInBytesOfOut);
	bool			wchar_to_utf8 (const wchar_t* wstring, u32 nBytesToUse, u8* out, u32 sizeInBytesOfOut);
} //namespace win32


#endif //_winOSInclude_h_
#endif //GOS_PLATFORM__WINDOWS
