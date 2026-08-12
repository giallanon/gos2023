#include "gosAsset2Builder.h"
#include "gosAsset2Monitor.h"

using namespace gos;

//******************************** 
void test (gos::GPU *gpu, const char *subfolder)
{
    gos::asset2::Builder b(gpu);

    bool ret;
    char baseFolder[1024];
    sprintf_s (baseFolder, sizeof(baseFolder), "%s/%s", gos::getPhysicalPathToWritableFolder(), subfolder);
    
	ret = b.rebuild_all(baseFolder, true); b.save_dependencies_report (baseFolder); b.save_asset_manifest (baseFolder); return;
    
    ret = b.build(baseFolder, true); 
    if (ret)
    {
        b.save_dependencies_report (baseFolder);
        b.save_asset_manifest (baseFolder);

        b.debug_sanityCheck(baseFolder);
    }
}


//******************************** 
void test_monitor (gos::GPU *gpu, const char *path_to_DB)
{
	asset2::Monitor mon(gpu);
	mon.run (path_to_DB);
}

//******************************** 
int main()
{
    gos::sGOSInit init;
    init.memory__set_default_for_NON_games();

    init.set_log_mode (gos::sGOSInit::eLogMode::only_console, gos::Logger::LEVEL__DEFAULT);
    //init.set_log_mode (gos::sGOSInit::eLogMode::none);
    if (!gos::init (init, "assetBuilder2"))
	{
        return -1;
	}

	gos::GPU gpu;
	if (!gpu.init (GOSWinHandle::INVALID(), false))
		return -2;

	//test (&gpu, "test1");
	//test (&gpu, "test2");
	test_monitor(&gpu, "/home/giallanon/gixprogram/gos2023/engine/bin/testEngine/writable/assets");

	gpu.deinit();
    
    

#ifdef GOS_PLATFORM__WINDOWS
    printf ("\n\nPress a key to terminate\n");
    _getch();
#endif
    
    gos::deinit();
    return 0;
}