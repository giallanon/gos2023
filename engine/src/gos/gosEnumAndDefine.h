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
typedef void (*ConsoleTrap_CTRL_C)(void *userParam);


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
} //namespace gos

#endif //_gosEnumAndDefine_h_