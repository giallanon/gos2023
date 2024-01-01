#ifndef _gosEnumAndDefine_h_
#define _gosEnumAndDefine_h_
#include "gosDataTypes.h"

//================================================================
#define GOS_IS_POWER_OF_TWO(n)                          (n && !(n & (n - 1)))

/* allinea [num] alla potenza del 2 piu' vicina a [align].
    [align] deve a sua volta essere una potenza del 2.
    Esempio:
        GOS_ALIGN_NUMBER_TO_POWER_OF_TWO(31, 32) => 32
        GOS_ALIGN_NUMBER_TO_POWER_OF_TWO(33, 32) => 64
*/
constexpr inline u32 GOS_ALIGN_NUMBER_TO_POWER_OF_TWO (u32 num, u32 alignPowerOfTwo)         { assert(GOS_IS_POWER_OF_TWO(alignPowerOfTwo)); return (((num) + ((alignPowerOfTwo) - 1)) & ~((alignPowerOfTwo) - 1)); }

#define GOSSWAP(a,b)	{auto temp=(a); (a)=(b); (b)=temp;}


//================================================================
typedef void (*GOS_ConsoleTrap_CTRL_C)(void *userParam);

//================================================================
//thread thread main-fn prototype
typedef i16 (*GOS_ThreadMainFunction)(void *userParam);


//================================================================
enum class eSeek: u8
{
	start = 0,
	end = 1,
	current = 2
};

enum class eFileMode : u8
{
    readOnly = 0,
    writeOnly = 1,
    readWrite = 2
};

enum class eFolderDeleteMode : u8
{
	doNotDeleteAnyFolder = 0,
	deleteAlsoTheSubfolder = 1,
	deleteAlsoTheSubfolderAndTheMainFolder = 2
};

enum class eTextColor : u8
{
    black = 30,
    darkRred = 31,
    darkGreen = 32,
    darkYellow = 33,
    darkBlue = 34,
    darkMagenta = 35,
    darkCyan = 36,
    grey = 37,        //aka dark white

    //lightBlack = 90,
    red = 91,
    green = 92,
    yellow = 93,
    blue = 94,
    magenta = 95,
    cyan = 96,
    white = 97
};

enum class eBgColor : u8
{
    black = 40,
    darkRred = 41,
    darkGreen = 42,
    darkYellow = 43,
    darkBlue = 44,
    darkMagenta = 45,
    darkCyan = 46,
    grey = 47,        //aka dark white

    //lightBlack = 100,
    red = 101,
    green = 102,
    yellow = 103,
    blue = 104,
    magenta = 105,
    cyan = 106,
    white = 107
};

enum class eDayOfWeek : u8
{
    sunday      = 0,
    monday      = 1,
    tuesday     = 2,
    wednesday   = 3,
    thursday    = 4,
    friday      = 5,
    saturday    = 6
};

enum class eDataFormat : u8
{
    _1f32			= 0,
    _2f32			= 1,
    _3f32			= 2,
    _4f32			= 3,
    _1i32			= 4,
    _2i32			= 5,
    _3i32			= 6,
    _4i32			= 7,
    _1u32			= 8,
    _2u32			= 9,
    _3u32			= 10,
    _4u32			= 11,
    _4u8			= 12,
    _4u8_norm		= 13,	//speciale per HLSL, vuol dire che prende i 4 byte e, internamente nello shader, li converte in 4 float tra 0.0 e 1.0
};

enum class eSocketError: u8
{
    none = 0,
    denied = 1,        //Permission to create a socket of the specified type and/or protocol is denied.
    unsupported = 2,   //The implementation does not support the specified address family
    tooMany = 3,       //The per-process limit on the number of open file descriptors has been reached.
    noMem = 4,         //Insufficient memory is available.  The socket cannot be created until sufficient resources are freed.
    addressInUse = 5,
    addressProtected = 6,
    alreadyBound = 7,
    invalidDescriptor = 8,
    errorSettingReadTimeout = 9,
    errorSettingWriteTimeout = 10,
    errorListening = 11,
    no_such_host   = 12,
    connRefused = 13,
    timedOut = 14,
    invalidParameter = 15,

    unable_to_handshake = 0xfe,
    unknown = 0xff
};

enum class eThreadError : u8
{
    none = 0,
    invalidStackSize = 1,
    tooMany = 2,
    unknown = 0xff
};


namespace gos
{
    struct sGOSInit
    {
    public:
        enum class eLogMode
        {
            none = 0,
            console            
        };

    public:
                sGOSInit()                                          { setLogMode(eLogMode::console); setDefaultForGame(); }
        
        void    setDefaultForGame()                                 { setStartingSizeOfDefaultHeap_MB(1024); setStartingSizeOfScrapAllocator_MB (128); }
        void    setDefaultForNonGame()                              { setStartingSizeOfDefaultHeap_MB(1); setStartingSizeOfScrapAllocator_MB(1); }
        
        void    setStartingSizeOfDefaultHeap_MB (u32 mb)            { _startingSizeOfDefaultHeapAllocator_MB = mb; }
        void    setStartingSizeOfScrapAllocator_MB (u32 mb)         { _startingSizeOfScrapAllocator_MB = mb; }
        void    setLogMode (eLogMode m)                             { _logMode=m; }
        
    public:
        u32         _startingSizeOfDefaultHeapAllocator_MB;
        u32         _startingSizeOfScrapAllocator_MB;
        eLogMode    _logMode;
    };

    struct File
    {
        platform::OSFile osFile;  //"OSFile" e' dipendente dalla platform, per cui deve essere definito nel codice di platform
    };

    struct FileFind
    {
        platform::OSFileFind osFF;
    };

    struct Mutex
    {
        platform::OSMutex osm;  //"OSMutex" e' dipendente dalla platform, per cui deve essere definito nel codice di platform
    };

    struct Event
    {
        platform::OSEvent osEvt;  //"osEvt" e' dipendente dalla platform, per cui deve essere definito nel codice di platform
    };    

    struct Thread
    {
        void *p;
    };

    struct Socket
    {
        platform::OSSocket osSok;  //"OSSocket" e' dipendente dalla platform, per cui deve essere definito nel codice di platform
    };

    struct NetAddr
    {
	    sockaddr_in		addr;

	    NetAddr&		operator= (const NetAddr& b)							        { memcpy (&addr, &b.addr, sizeof(addr)); return *this; }
    };

    struct MacAddress
    {
    public:
        void                set (u8 a, u8 k, u8 c, u8 d,u8 e, u8 f)                     { b[0]=a; b[1]=k; b[2]=c; b[3]=d; b[4]=e; b[5]=f; }
        bool				operator== (const MacAddress &b) const;
        bool				operator!= (const MacAddress &b) const;
        u8                  serializeToBuffer(u8 *dst, u32 sizeof_dst) const            { if (sizeof_dst < 6) return 0; memcpy (dst, b, 6); return 6; }
        u8                  deserializeFromBuffer (const u8* src, u32 sizeof_dst)       { if (sizeof_dst < 6) { memset (b, 0, 6); return 0; } memcpy (b, src, 6); return 6; }

    public:
        u8  b[6];   //il mac address è composto da 48 bits, ovvero 6 byte
                    //In questo caso b[0] rappresenta il MSB e b[5] il LSB
    };

    struct IPv4
    {
    public:
        void        set (u8 a, u8 b, u8 c, u8 d)                            { ips[0]=a; ips[1]=b; ips[2]=c; ips[3]=d; }
        bool		operator== (const IPv4 &b) const                        { return (ips[0]==b.ips[0] && ips[1]==b.ips[1] && ips[2]==b.ips[2] && ips[3]==b.ips[3]); }
        bool		operator!= (const IPv4 &b) const                        { return (ips[0]!=b.ips[0] || ips[1]!=b.ips[1] || ips[2]!=b.ips[2] || ips[3]!=b.ips[3]); }
        u8          serializeToBuffer(u8 *dst, u32 sizeof_dst) const        { if (sizeof_dst < 4) return 0; memcpy (dst, ips, 4); return 4; }
        u8          deserializeFromBuffer (const u8 *src, u32 sizeof_src)   { if (sizeof_src < 4) { memset (ips, 0, 4); return 0; } memcpy (ips, src, 4); return 4; }

    public:
        u8  ips[4];
    };    

    struct NetworkAdapterInfo
    {
        char    name[32];
        char	ip[16];
        char	subnetMask[16];
    };

} //namespace gos

#endif //_gosEnumAndDefine_h_