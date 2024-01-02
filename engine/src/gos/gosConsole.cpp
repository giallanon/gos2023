#include "gos.h"


#define GOS_CONSOLE_ESC "\x1b"

using namespace gos;

#ifdef GOS_PLATFORM__WINDOWS
bool win32_enableVTMode()
{
    // Set output mode to handle virtual terminal sequences
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE)
        return false;

    DWORD dwMode = 0;
    if (!GetConsoleMode(hOut, &dwMode))
        return false;

    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    if (!SetConsoleMode(hOut, dwMode))
        return false;
    
    CONSOLE_SCREEN_BUFFER_INFO ScreenBufferInfo;
    GetConsoleScreenBufferInfo(hOut, &ScreenBufferInfo);
    COORD Size;
    Size.X = ScreenBufferInfo.srWindow.Right - ScreenBufferInfo.srWindow.Left + 1;
    Size.Y = ScreenBufferInfo.srWindow.Bottom - ScreenBufferInfo.srWindow.Top + 1;
	
	return true;
}
#endif //GOS_PLATFORM__WINDOWS

struct sInternalConsole
{
    eTextColor curTextColor;
    eBgColor curBGColor;
};

static sInternalConsole consoleData; 

//***************************************
bool console::priv_init ()
{
#ifdef GOS_PLATFORM__WINDOWS
    if (!win32_enableVTMode())
        return false;
#endif

    memset (&consoleData, 0, sizeof(consoleData));
    consoleData.curTextColor = eTextColor::grey;
    consoleData.curBGColor = eBgColor::black;
    
    console::setBgColor (consoleData.curBGColor);
    console::setTextColor(consoleData.curTextColor);
    return true;
}

//***************************************
void console::priv_deinit()
{
    console::setBgColor (eBgColor::black);
    console::setTextColor(eTextColor::grey);
}

//***************************************
void console::setWidthTo80Cols()
{
    printf (GOS_CONSOLE_ESC "[80$|");
}

//***************************************
void console::setWidthTo132Cols()
{
    printf (GOS_CONSOLE_ESC "[132$|");
}


//***************************************
void console::setWH (u16 w, u16 h)
{
    printf (GOS_CONSOLE_ESC "[8;%d;%dt", h, w);
}

//***************************************
eTextColor console::setTextColor (const eTextColor c)
{
    printf (GOS_CONSOLE_ESC "[%dm", static_cast<u8>(c));
    const eTextColor ret = consoleData.curTextColor;
    consoleData.curTextColor = c;
    return ret;
}

//***************************************
eBgColor console::setBgColor (const eBgColor c)
{
    printf (GOS_CONSOLE_ESC "[%dm", static_cast<u8>(c));
    const eBgColor ret = consoleData.curBGColor;
    consoleData.curBGColor = c;
    return ret;    
}

//***************************************
void console::clear()               { printf (GOS_CONSOLE_ESC "[2J"); }
void console::clearLine()           { printf (GOS_CONSOLE_ESC "[2K"); }
void console::clearEndOfLine()      { printf (GOS_CONSOLE_ESC "[K"); }
void console::clearStartOfLine()    { printf (GOS_CONSOLE_ESC "[1K"); }
void console::clearDown()           { printf (GOS_CONSOLE_ESC "[J"); }
void console::clearUp()             { printf (GOS_CONSOLE_ESC "[1J"); }

//***************************************
void console::cursorMove (u16 x, u16 y)
{
    printf (GOS_CONSOLE_ESC "[%dG", x);
    printf (GOS_CONSOLE_ESC "[%dd", y);
}

//***************************************
void console::cursorMoveX (u16 x)
{
    printf (GOS_CONSOLE_ESC "[%dG", x);
}

//***************************************
void console::cursorMoveY (u16 y)
{
    printf (GOS_CONSOLE_ESC "[%dd", y);
}

//***************************************
void console::setScrollingArea (u16 rowStart, u16 rowEnd)
{
    if (rowStart==0 && rowEnd==0)
        printf (GOS_CONSOLE_ESC "[r");  //tutto schermo
    else
    printf (GOS_CONSOLE_ESC "[%d;%dr", rowStart, rowEnd);
}