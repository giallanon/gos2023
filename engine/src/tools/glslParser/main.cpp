#include "SPVReflect.h"

using namespace gos;


//******************************** 
bool compile (const char *shaderSRCFile, const char *shaderStage, bool bWithSourceLevelDebugInfo)
{
    //glslc -fshader-stage=vert --target-env=vulkan1.3 lineRenderer.vert.shader -g -O -o lineRenderer.vert.spv
    char cmd[1024];
    sprintf_s (cmd, sizeof(cmd), "glslc -fshader-stage=%s --target-env=vulkan1.3 %s/example/%s -g -O -o %s/example/compiled/%s.spv 2>&1", 
        shaderStage,
        gos::getAppPathNoSlash(), shaderSRCFile,
        gos::getAppPathNoSlash(), shaderSRCFile
    );

    char *result;
    u32 len;
    if (!gos::runShellScriptAndStoreResult (cmd, gos::getScrapAllocator(), &result, &len))
        return false;

    if (NULL == result)
        return true;

    //c'e' stato qualche errore di compilazione
    printf("(%d): %s", len, result);
    GOSFREE_SCRAP(result);
    return true;
}

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
        compile("lineRenderer.vert.shader", "vert", true);
        
        /*fs::addAlias ("@ex", "example", eAliasPathMode::relativeToAppFolder);

        SPVReflect parser;
        if (parser.parseFromFile ("@ex/phong.vert.spv", "@ex/phong.frag.spv"))
            parser.printInfo();
            */
    }
    

#ifdef GOS_PLATFORM__WINDOWS
    _getch();
#endif
    
    gos::deinit();
    return 0;
}