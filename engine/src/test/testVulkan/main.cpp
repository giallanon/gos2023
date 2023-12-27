#include "VKExample1.h"



//********************************+
int main()
{
    gos::sGOSInit init;
    init.setDefaultForGame();
    if (gos::init (init, "testVulkan"))
    {
        VulkanExample1 app;
        if (app.init())
        {
            app.mainLoop();
            app.cleanup();
        }
        gos::deinit();
    }

#ifdef GOS_PLATFORM__WINDOWS
    printf ("\nPress any key to terminate\n");
    _getch();
#endif

    return 0;
}