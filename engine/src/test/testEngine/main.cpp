#include "test1.h"
#include "game1.h"
#include "Land1/Land1_app2.h"


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
        //if (!engine.setup (1600, 900, "test engine"))
        if (!engine.setup (300, 300, "test engine"))
            return -2;

        if (!engine.asset_build())  return -3;
        //if (!engine.asset_rebuildAll())  return -3;


        if (engine.setup_renderPipe())
        {
            //Test1 test;		test.run (&engine);
            //Game1 game;		game.run (&engine);
            Land1_app2 app;		app.run (&engine);
        }
    }
    

    gos::deinit();
    return 0;
}

