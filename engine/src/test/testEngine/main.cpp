#include "test1.h"


//******************************** 
int main()
{
    gos::sGOSInit init;
    init.memory_setDefaultForGame();
    
    init.setLogMode (gos::sGOSInit::eLogMode::both_console_and_file);
    if (!gos::init (init, "gosEngine"))
        return -1;

    {
        gos::Engine engine;
        if (!engine.setup (1024, 768, "test engine"))
            return -2;

        Test1   test;
        test.run (&engine);
    }
    

    gos::deinit();
    return 0;
}

