#include "gosAssetBuilder.h"

using namespace gos;

//******************************** 
void test_assetBuilder1()
{
    gos::GPU gpu;
    if (!gpu.init (GOSWinHandle::INVALID(), false))
        return;
    
    {
        const char BASE_FOLDER[] = { "test_assets_1" };

        asset::Builder builder(&gpu);

        if (!builder.rebuildAll(BASE_FOLDER, true))
            return;
        
        builder.save_dependencies_report(BASE_FOLDER);
        
        builder.debug_sanityCheck (BASE_FOLDER);
    }
    gpu.deinit();
}

//******************************** 
void test_assetBuilder2()
{
    gos::GPU gpu;
    if (!gpu.init (GOSWinHandle::INVALID(), false))
        return;
    
    {
        const char BASE_FOLDER[] = { "test_assets_2" };

        asset::Builder builder(&gpu);

        if (!builder.buildAll(BASE_FOLDER, true))
            return;
        
        builder.save_dependencies_report(BASE_FOLDER);
        
        builder.debug_sanityCheck (BASE_FOLDER);
    }
    gpu.deinit();
}

//**************************************
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
int main()
{
    gos::sGOSInit init;
    init.memory_setDefaultForNonGame();

    init.setLogMode (gos::sGOSInit::eLogMode::only_console);
    if (!gos::init (init, "assetBuilder"))
        return -1;

    //creaTXTPerGliShaderDiBuilderTex2D();
    test_assetBuilder1();
    //test_assetBuilder2();
    

#ifdef GOS_PLATFORM__WINDOWS
    _getch();
#endif
    
    gos::deinit();
    return 0;
}