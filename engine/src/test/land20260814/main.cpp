#include "gosEngine.h"
#include "App.h"

using namespace gos;

//******************************** 
int main()
{
    gos::sGOSInit init;
    init.memory__set_default_for_games();
    
    init.set_log_mode (gos::sGOSInit::eLogMode::both_console_and_file, 1);
	if (!gos::init (init, "gosEngine"))
		return -1;

    {
        gos::Engine engine;
        if (!engine.setup (1600, 700, "land20260814"))
            return -2;

        //if (!engine.asset_build())  return -3;
        //if (!engine.asset_rebuildAll())  return -3;


        if (engine.setup_renderPipe())
        {
			App app;
			//app.enable_asset_monitor();
			app.run (&engine);
        }
    }
    

    gos::deinit();
    return 0;
}

