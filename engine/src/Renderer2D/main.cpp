#include "TheApp.h"

using namespace gos;

//******************************** 
int main()
{
    gos::sGOSInit init;
    init.memory_setDefaultForGame();

    init.setLogMode (gos::sGOSInit::eLogMode::both_console_and_file);
    if (!gos::init (init, "Renderer2D"))
        return -1;

    if (!gos::input::init())
        return -2;

    GOSWinHandle mainWin;
    if (!gos::input::window_create (1024, 768, gos::getAppName(), &mainWin))
        return -3;
        
    gos::GPU gpu;
    if (!gpu.init (mainWin, false))
        return -4;

    //forever loop
    {
        TheApp app;

        app.setup (&gpu);
        app.run();
    }
    
    gpu.deinit();
    gos::input::window_destroy (mainWin);
    gos::input::deinit();
    gos::deinit();
    return 0;
}