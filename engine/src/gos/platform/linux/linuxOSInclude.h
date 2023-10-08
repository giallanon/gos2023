#ifdef GOS_PLATFORM__LINUX
#ifndef _linuxosinclude_h_
#define _linuxosinclude_h_
#include <arpa/inet.h>
#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <inttypes.h>
#include <pthread.h>
#include <netinet/in.h>
#include <signal.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/eventfd.h>
#include <sys/epoll.h>
#include <termio.h>
#include <termios.h>
#include <unistd.h>

/***********************************************
 * typedef dei dati di base
 */
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


/***********************************************
 * debug helpers
 */
#define	ALWAYS_DBGBREAK	raise(SIGTRAP);
#ifdef _DEBUG
    #define	DBGBREAK	raise(SIGTRAP);
#else
    #define	DBGBREAK
#endif


/***********************************************
 * c/c++ portability stuff
 */
#define UNUSED_PARAM(x)	x __attribute__((unused))
#define ALIGNOF(x)	__alignof(x)

#define sprintf_s snprintf
#define strcat_s(dest, size, source) strcat(dest,source)
inline void strcpy_s (char *dest, size_t size, const char *source)              { snprintf(dest,size,"%s",source);}


namespace platform
{
    typedef int OSFile;
    typedef pthread_mutex_t OSMutex;

    struct OSFileFind
    {
                        OSFileFind(): dirp(NULL), dp(NULL) { }

        DIR	            *dirp;
        struct dirent   *dp;
        char            strJolly[64];
    };

    
} //namespace platform



#endif //_linuxosinclude_h_
#endif // GOS_PLATFORM__LINUX
