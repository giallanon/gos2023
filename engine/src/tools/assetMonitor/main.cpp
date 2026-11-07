#include "gosAsset2Builder.h"
#include "gosAsset2Monitor.h"

using namespace gos;

/**************************************************************** 
 * asset2::importe_texture2D richiede un paio di shader.
 * Il codice qui sotto serve per trasformare gli shader gia' compilati in un file di testo
 * che poi finisce statico in gosAsset2Builder_tex2D_shaders.h
 */
bool priv__do_creaTXTPerGliShaderDiBuilderTex2D(const char *shaderFile)
{
    u32 fsize;
    u8 *buffer = gos::fs::fileLoadInMemory (gos::getSysHeapAllocator(), shaderFile, &fsize);
    if (NULL == buffer)
    {
        gos::logger::err ("Can't open %s\n", shaderFile);
        return false;
    }

    char s[1024];
    sprintf_s (s, sizeof(s), "%s.txt", shaderFile);
    gos::File f;
    gos::fs::fileOpenForW (&f, s);
    u32 i = 0;
    while (i < fsize)
    {
        u32 left = fsize - i;
        if (left >= 32)
            left = 32;
        for (u32 i2=0; i2<left; i2++)
        {
            sprintf_s (s, sizeof(s),"0x%02X, ", buffer[i++]);
            gos::fs::fileWrite (f, s, 6);
        }
        gos::fs::fileWrite (f, "\n", 1);
    }
    gos::fs::fileClose(f);

    GOSFREE(gos::getSysHeapAllocator(), buffer);
    return true;
}
void creaTXTPerGliShaderDiBuilderTex2D()
{
    priv__do_creaTXTPerGliShaderDiBuilderTex2D("shader_per_Builder_tex2D/shader_builderTex2D.vert.spv");
    priv__do_creaTXTPerGliShaderDiBuilderTex2D("shader_per_Builder_tex2D/shader_builderTex2D.frag.spv");
}

//******************************** 
void test (gos::GPU *gpu, const char *baseFolder)
{
    gos::asset2::Builder b(gpu);

    bool ret;
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
	if (!fs::folderExists(path_to_DB))
	{
		logger::err ("Invalid path %s\n", path_to_DB);
		return;
	}
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
	logger::log (eTextColor::green, "GOS Asset Monitor, V 1.1\n");
	logger::log (eTextColor::green, "====================================\n");
	
	gos::GPU gpu;
	if (!gpu.init (GOSWinHandle::INVALID(), false))
	{
		logger::err ("Can't init GPU\n");
		return -2;
	}

#ifdef _DEBUG
	//test (&gpu, "@w/test_assets_1");
	//test (&gpu, "@w/test_assets_2");
	//test (&gpu, "@w/test_assets_3");
	test (&gpu, "@w/test_assets_4");
	//test (&gpu, "/home/giallanon/gixprogram/gos2023/engine/bin/land20260814/writable/assets/");
	#ifdef GOS_PLATFORM__WINDOWS
		test_monitor(&gpu, "C:/rhea/rheaSRC/gos2023/engine/bin/testRes2/writable/assets");
	#else
		//test_monitor(&gpu, "/home/giallanon/gixprogram/gos2023/engine/bin/testRes2/writable/assets");
	#endif
	
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
    
    

#if defined(GOS_PLATFORM__WINDOWS) && defined(_DEBUG)
    printf ("\n\nPress a key to terminate\n");
    _getch();
#endif
    
    gos::deinit();
    return 0;
}