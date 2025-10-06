#include "gosEngine.h"

void run1 ();


//******************************** 
int main()
{
    gos::sGOSInit init;
    init.memory_setDefaultForGame();
    
    init.setLogMode (gos::sGOSInit::eLogMode::both_console_and_file);
    if (!gos::init (init, "gosEngine"))
        return -1;

    //main app
    run1 ();

    gos::deinit();
    return 0;
}



//******************************** 
void run1 ()
{
    gos::Engine engine;
    if (!engine.setup (1024, 768, "gos-engine-1.0"))
        return;

    while (engine.run())
    {
        printf ("ciao\n");
        gos::sleep_msec(500);
    }
}