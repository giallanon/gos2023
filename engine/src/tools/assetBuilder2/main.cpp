#include "gosAsset2Builder.h"

using namespace gos;

//******************************** 
void test1()
{
    gos::asset2::Builder b;

    char s[1024];
    sprintf_s (s, sizeof(s), "%s/test1", gos::getPhysicalPathToWritableFolder());
    b.rebuildAll(s);
}

//******************************** 
int main()
{
    gos::sGOSInit init;
    init.memory_setDefaultForNonGame();

    init.setLogMode (gos::sGOSInit::eLogMode::only_console);
    if (!gos::init (init, "assetBuilder2"))
        return -1;

    test1();
    

#ifdef GOS_PLATFORM__WINDOWS
    _getch();
#endif
    
    gos::deinit();
    return 0;
}