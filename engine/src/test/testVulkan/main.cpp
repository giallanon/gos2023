#include "VKExample1.h"



//********************************+
int main()
{
    gos::sGOSInit init;
    init.setDefaultForGame();
    if (!gos::init (init, "testVulkan"))
        return 1;


    VulkanExample1 app;
    if (!app.init())
        return 1;

    app.mainLoop();
    app.cleanup();

    gos::deinit();
    return 0;
}