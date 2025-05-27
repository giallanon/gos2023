#include "SPVReflect.h"

using namespace gos;




//******************************** 
int main()
{
    gos::sGOSInit init;
    init.memory_setDefaultForNonGame();

    init.setLogMode (gos::sGOSInit::eLogMode::only_console);
    if (!gos::init (init, "glslParser"))
        return -1;
    else
    {
        fs::addAlias ("@ex", "example", eAliasPathMode::relativeToAppFolder);

        SPVReflect parser;
        if (parser.parseFromFile ("@ex/phong.vert.spv", "@ex/phong.frag.spv"))
            parser.printInfo();
    }
    

#ifdef GOS_PLATFORM__WINDOWS
    _getch();
#endif
    
    gos::deinit();
    return 0;
}