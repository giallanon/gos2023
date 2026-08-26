#include "Land1/Land1_app2.h"


//******************************** 
int main()
{
    gos::sGOSInit init;
    init.memory__set_default_for_games();
    
    init.set_log_mode (gos::sGOSInit::eLogMode::both_console_and_file);
	if (!gos::init (init, "gosEngine"))
		return -1;

    {
        gos::Engine engine;
        if (!engine.setup (1600, 900, "test engine"))
        //if (!engine.setup (900, 400, "test engine"))
            return -2;

        //if (!engine.asset_build())  return -3;
        //if (!engine.asset_rebuildAll())  return -3;


        if (engine.setup_renderPipe())
        {
            //Test1 test;		test.run (&engine);
            //Game1 game;		game.run (&engine);
            Land1_app2 app;		
			app.enable_asset_monitor();
			app.run (&engine);
        }
    }
    

    gos::deinit();
    return 0;
}

