#ifndef _gosEnumAndDefine_h_
#define _gosEnumAndDefine_h_
#include "gosDataFormat.h"

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

enum class eWaitEventOrigin : u8
{
	socket = 1,
	osevent = 2,
	serialPort = 3,
	msgQ = 4,
	deleted = 5
};

enum class eAliasPathMode : u8
{
    relativeToAppFolder = 0,
    relativeToWritableFolder,
    absolutePath
};

enum class eImageFormat : u8
{
    U8_RGBA_sRGB	= 0,
    U8_RGBA			= 1,
    U8_RGB			= 2,
    U8_R			= 3,

    U16_RGBA		= 4,
    U16_RGB			= 5,
    U16_R			= 6,

    U32_RGBA		= 7,
    U32_RGB			= 8,
    U32_R			= 9,

    F32_RGBA		= 10,
    F32_RGB			= 11,
    F32_R			= 12,

    U8_BGRA_sRGB	= 20,

    //depth format	(range 0xE0 - 0xEF)
    DEPTH_F32				= 0xE0,			//solo depth float 32bit
    DEPTH_U16				= 0xE1,			//solo depth 16 bit intero
    DEPTH_F32_STENCIL_U8	= 0xEA,			//depth f32 e stencil u8
    DEPTH_U16_STENCIL_U8	= 0xEB,			//depth u16 e stencil u8
    DEPTH_U24_STENCIL_U8	= 0xEC,			//depth u24 e stencil u8

    //compressed format
    DDS_BC3			= 0xF2,
    DDS_BC4			= 0xF3,
    DDS_BC5			= 0xF4,

    //uso interno
    _SAME_AS_CURRENT_SWAPCHAIN = 0xFF
};

enum class eDrawPrimitive : u8
{
	pointList = 0,
	
	lineList = 1,
	lineStrip = 2,
	
	trisList = 3,
	trisStrip = 4,
	trisFan = 5
};

enum class eShaderType : u8
{
	vertexShader = 0,
	fragmentShader = 1,
	unknown = 0xff
};

enum class eZFunc : u8
{
	NEVER           = 0,
	LESS            = 1,
	EQUAL           = 2,
	LESS_EQUAL      = 3,
	GREATER         = 4,
	NOT_EQUAL       = 5,
	GREATER_EQUAL   = 6,
	ALWAYS          = 7 
};

enum class eStencilOp : u8
{
	KEEP       		= 0,
	ZERO       		= 1,
	REPLACE    		= 2,
	INCR_AND_CLAMP  = 3,
	DECR_AND_CLAMP  = 4,
	INVERT     		= 5,
	INCR_AND_WRAP   = 6,
	DECR_AND_WRAP   = 7 
};

enum class eStencilFunc : u8
{
	NEVER           = 0,
	LESS            = 1,
	EQUAL           = 2,
	LESS_EQUAL      = 3,
	GREATER         = 4,
	NOT_EQUAL       = 5,
	GREATER_EQUAL   = 6,
	ALWAYS          = 7 
};

enum class eCullMode : u8
{
	NONE	= 0,
	CW		= 1,
	CCW		= 2
};

enum class eImageLayout : u8
{
	undefined = 0,					//VK_IMAGE_LAYOUT_UNDEFINED
	general,						//VK_IMAGE_LAYOUT_GENERAL
    color_attachment_optimal, 		//VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    shader_readonly,				//VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	transfer_src,					//VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
    transfer_dst,					//VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
	presentation,					//VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
};

enum class eAttachmentLoadOp : u8
{
    load = 0, 		//VK_ATTACHMENT_LOAD_OP_LOAD
    clear,			//VK_ATTACHMENT_LOAD_OP_CLEAR
    dont_care		//VK_ATTACHMENT_LOAD_OP_DONT_CARE
};

enum class eAttachmentStoreOp : u8
{
	store = 0, 		//VK_ATTACHMENT_STORE_OP_STORE
	dont_care,		//VK_ATTACHMENT_STORE_OP_DONT_CARE
    none			//VK_ATTACHMENT_STORE_OP_NONE
};

enum class eDepthStencilLayout : u8
{
	undefined = 0,						//VK_IMAGE_LAYOUT_UNDEFINED
    depth_attachment_optimal, 			//VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    depth_shader_readonly,				//VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL;
};

namespace gos
{
    struct sGOSInit
    {
    public:
        enum class eLogMode
        {
            none = 0,
            only_console,
            only_file,
            both_console_and_file
        };

        enum class eWritableFolder
        {
            inTheAppFolder = 0,     //in questo caso la directory si chiama "writable" ed e' una sottodir della dir dove sta l'eseguibile
            inUserFolder            //in questo caso la dir si chiama come l'App ed e' una sottdir della userFolder definita dall'OS
        };

    public:
                sGOSInit()                                                  
                { 
                    setLogMode(eLogMode::only_console); 
                    _memory.setDefaultForGame(); 
                    _writableFolder.reset();
                }
        
        void    setLogMode (eLogMode m)                                     { _logMode=m; }

        void    memory_setDefaultForGame()                                  { _memory.setDefaultForGame(); }
        void    memory_setDefaultForNonGame()                               { _memory.memory_setDefaultForNonGame(); }
        void    memory_setStartingSizeOfDefaultHeap_MB (u32 mb)             { _memory.startingSizeOfDefaultHeapAllocator_MB = mb; }
        void    memory_setStartingSizeOfScrapAllocator_MB (u32 mb)          { _memory.startingSizeOfScrapAllocator_MB = mb; }

        void    writableFolder_setMode (eWritableFolder m)                  { _writableFolder.mode = m; }
        void    writableFolder_setSuffix (const char *suff)                 { sprintf_s (_writableFolder.suffix, sizeof(_writableFolder.suffix), "%s", suff); }
        
    public:
        struct sMemory
        {
            u32         startingSizeOfDefaultHeapAllocator_MB;
            u32         startingSizeOfScrapAllocator_MB;

            void setDefaultForGame()                { startingSizeOfDefaultHeapAllocator_MB = 1024; startingSizeOfScrapAllocator_MB = 128; }
            void memory_setDefaultForNonGame()      { startingSizeOfDefaultHeapAllocator_MB = 1;    startingSizeOfScrapAllocator_MB = 1; }
        };

        struct sWritableFolder
        {
            eWritableFolder mode;
            char            suffix[32];

            void reset() { mode = eWritableFolder::inTheAppFolder; memset (suffix, 0, sizeof(suffix)); }
        };

    public:
        eLogMode        _logMode;
        sMemory         _memory;
        sWritableFolder _writableFolder;
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