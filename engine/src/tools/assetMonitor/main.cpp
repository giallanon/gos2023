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
int main (int argc, char *argv[])
{
    gos::sGOSInit init;
    init.memory__set_default_for_NON_games();
    init.set_log_mode (gos::sGOSInit::eLogMode::only_console, gos::Logger::LEVEL__DEFAULT);
    if (!gos::init (init, "assetMonitor"))
	{
		printf ("Error during GOS init\n");
        return -1;
	}

	logger::log (eTextColor::green, "====================================\n");
	logger::log (eTextColor::green, "GOS Asset Monitor, V 1.0\n");
	logger::log (eTextColor::green, "====================================\n");
	
	gos::GPU gpu;
	if (!gpu.init (GOSWinHandle::INVALID(), false))
	{
		logger::err ("Can't init GPU\n");
		return -2;
	}

#ifdef _DEBUG
	//test (&gpu, "test1");
	//test (&gpu, "test2");
	test_monitor(&gpu, "/home/giallanon/gixprogram/gos2023/engine/bin/testEngine/writable/assets/");
#else
	//for (u8 i=0; i<argc; i++)	printf ("arg %d: %s\n", i, argv[i]);

	//mi aspetto come parametro il path ad una cartella con dentro il DB degli assett
	if (argc < 2)
	{
		logger::err ("Invalid args. Expected syntax is: assetMonitor <path_to_DB_folder>\n");
		return -3;
	}

	test_monitor(&gpu, argv[1]);
#endif

	gpu.deinit();
    
    

#ifdef GOS_PLATFORM__WINDOWS
    printf ("\n\nPress a key to terminate\n");
    _getch();
#endif
    
    gos::deinit();
    return 0;
}