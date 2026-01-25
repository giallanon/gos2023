#include "gosAsset2Builder.h"

using namespace gos;

//******************************** 
void test (gos::GPU *gpu, const char *subfolder)
{
    gos::asset2::Builder b(gpu);

    bool ret;
    char baseFolder[1024];
    sprintf_s (baseFolder, sizeof(baseFolder), "%s/%s", gos::getPhysicalPathToWritableFolder(), subfolder);
    
	ret = b.rebuildAll(baseFolder, true); b.save_dependencies_report (baseFolder); b.save_asset_manifest (baseFolder); return;
    
    ret = b.build(baseFolder, true); 
    if (ret)
    {
        b.save_dependencies_report (baseFolder);
        b.save_asset_manifest (baseFolder);

        b.debug_sanityCheck(baseFolder);
    }
}

//******************************** 
int main()
{
    gos::sGOSInit init;
    init.memory_setDefaultForNonGame();

    //init.setLogMode (gos::sGOSInit::eLogMode::only_console);
    init.setLogMode (gos::sGOSInit::eLogMode::none);
    if (!gos::init (init, "assetBuilder2"))
        return -1;

    gos::GPU gpu;
    if (!gpu.init (GOSWinHandle::INVALID(), false))
        return -2;

    //test (&gpu, "test1");
	test (&gpu, "test2");
    
    gpu.deinit();

#ifdef GOS_PLATFORM__WINDOWS
    printf ("\n\nPress a key to terminate\n");
    _getch();
#endif
    
    gos::deinit();
    return 0;
}