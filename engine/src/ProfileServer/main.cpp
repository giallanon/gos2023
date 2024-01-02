#include "main.h"
#include "Server.h"

class ConsoleArea
{
public:
    ConsoleArea()                                   { setArea (0,0,80,60); }
    ConsoleArea (u16 x, u16 y, u16 w, u16 h)        { setArea (x,y,w,h); }

    void setArea (u16 x, u16 y, u16 w, u16 h)       { areaX=x; areaY=y; areaW=w; areaH=h; cursorX=x; cursorY=y; }
    void setBgColor (eBgColor col)                  { bgCol = col; }
    void setTextColor (eTextColor col)              { textCol = col; }
    void clear()
    {
        gos::console::setBgColor (bgCol);
        gos::console::setTextColor (textCol);
        for (u16 row=0; row<areaH; row++)
        {
            gos::console::cursorMove (areaX, areaY+row);
            for (u16 col=0; col<areaW; col++)
                printf (" ");
            printf ("\n");
        }
    }

    void	print (const char *format, ...)
    {
        gos::console::setBgColor (bgCol);
        gos::console::setTextColor (textCol);
        gos::console::cursorMove (cursorX, cursorY);
        va_list argptr;
        va_start (argptr, format);
        int nWritten = vprintf (format, argptr);
        va_end (argptr); 

        cursorX += nWritten;
    }


private:
    u16         areaX, areaY, areaW, areaH;
    eBgColor    bgCol;
    eTextColor  textCol;
    u16         cursorX;
    u16         cursorY;
};


//***********************************************
void consoleTrap_CTRL_C (void *userParam)
{
    printf ("CTRL-C detected\n");
    Server *server = reinterpret_cast<Server*>(userParam);
    server->quit();
}

//***********************************************
void run()
{
    Server server;
    
    gos::console::trap_CTRL_C (consoleTrap_CTRL_C, &server);
    server.run();
}

void testConsoleArea()
{
    gos::console::setBgColor (eBgColor::black);
    gos::console::clear();

    ConsoleArea areaRowTop(1, 1, 80, 1);
    areaRowTop.setBgColor (eBgColor::darkBlue);
    areaRowTop.setTextColor (eTextColor::yellow);
    areaRowTop.clear();

    for (u8 t=0; t<8; t++)
    {
        for (u8 i=1;i<9; i++)
            areaRowTop.print("%d", i);
        areaRowTop.print(" ");
    }

    ConsoleArea area1(1, 2, 30, 10);
    area1.setBgColor (eBgColor::blue);
    area1.setTextColor (eTextColor::yellow);
    area1.clear();

    ConsoleArea area2(32, 4, 20, 10);
    area2.setBgColor (eBgColor::darkCyan);
    area2.setTextColor (eTextColor::white);
    area2.clear();

    getchar();
}

//***********************************************
int main ()
{
    gos::sGOSInit init;
    init.memory_setDefaultForNonGame();
    //init.setLogMode (gos::sGOSInit::eLogMode::only_file);
    if (!gos::init (init, "ProfileServer"))
        return 1;

    testConsoleArea();

/*
    gos::console::setBgColor (eBgColor::black);
    gos::console::clear();
    
    gos::console::cursorMove(0,0);
    gos::console::setBgColor (eBgColor::darkBlue);
    for (u8 row=0; row<40; row++)
        printf ("%02d\n", row);

    gos::console::cursorMove(3,0);
    gos::console::setBgColor (eBgColor::white);
    gos::console::setTextColor (eTextColor::black);
    printf ("Profile Server v%d.%d.%d", VERSION_MAJOR, VERSION_MINOR, VERSION_BUILD);
    gos::console::clearEndOfLine();
    printf ("\n");
    gos::console::setBgColor (eBgColor::black);
    gos::console::setTextColor (eTextColor::grey);
    printf ("press CTRL C to terminate\n\n");

    gos::console::setScrollingArea (10, 20);
    gos::console::cursorMove(3, 10);
    run();
*/


    gos::deinit();

#ifdef GOS_PLATFORM__WINDOWS
    printf ("\nPress any key to terminate\n");
    _getch();
#endif

    return 0;
}