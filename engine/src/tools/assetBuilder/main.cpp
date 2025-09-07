#include "gosAssetBuilder.h"
#include "builders/builder_pipedef.h"
#include "builders/builder_shader.h"

using namespace gos;


//******************************** 
void test_assetBuilder2()
{
    const char BASE_FOLDER[] = { "test_assets_2" };

    asset::Builder builder;
    builder.addBuilder<gos::asset::Builder_vtxShader>();
    builder.addBuilder<gos::asset::Builder_pxlShader>();
    builder.addBuilder<gos::asset::Builder_pipeDef>();


    if (!builder.buildAll(BASE_FOLDER, true))
        return;
    
    builder.save_dependencies_report(BASE_FOLDER);
    
    builder.debug_sanityCheck (BASE_FOLDER);
}



//******************************** 
int main()
{
    gos::sGOSInit init;
    init.memory_setDefaultForNonGame();

    init.setLogMode (gos::sGOSInit::eLogMode::only_console);
    if (!gos::init (init, "assetBuilder"))
        return -1;

    test_assetBuilder2();
    

#ifdef GOS_PLATFORM__WINDOWS
    _getch();
#endif
    
    gos::deinit();
    return 0;
}