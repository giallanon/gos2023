#include "gosAssetBuilder.h"

using namespace gos;


//******************************** 
void test_assetBuilder2()
{
    const char BASE_FOLDER[] = { "test_assets_3" };

    asset::Builder builder;

    if (!builder.rebuildAll(BASE_FOLDER, true))
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