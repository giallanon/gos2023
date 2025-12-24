#include "gosAsset2Builder.h"

using namespace gos;

//******************************** 
void test1()
{
    gos::asset2::Builder b(NULL);

    bool ret;
    char baseFolder[1024];
    sprintf_s (baseFolder, sizeof(baseFolder), "%s/test1", gos::getPhysicalPathToWritableFolder());
    ret = b.rebuildAll(baseFolder); b.save_dependencies_report (baseFolder); return;
    
    ret = b.build(baseFolder); 
    if (ret)
    {
        b.save_dependencies_report (baseFolder);

        if (!b.debug_sanityCheck(baseFolder))
            b.save_dependencies_report (baseFolder);
    }
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