#include "SPVReflect.h"

using namespace gos;


//******************************** 
bool compile (const char *shaderSRCFile, const char *shaderStage, bool bWithSourceLevelDebugInfo)
{
    //glslc -fshader-stage=vert --target-env=vulkan1.3 lineRenderer.vert.shader -g -O -o lineRenderer.vert.spv
    char cmd[1024];
    sprintf_s (cmd, sizeof(cmd), "glslc -fshader-stage=%s --target-env=vulkan1.3 %s/example/%s -g -O -o %s/example/compiled/%s.spv", 
        shaderStage,
        gos::getAppPathNoSlash(), shaderSRCFile,
        gos::getAppPathNoSlash(), shaderSRCFile
    );

    FILE *fp = _popen (cmd, "r");
    if (NULL != fp)
    {
        memset (cmd, 0, sizeof(cmd));
        while (fgets(cmd, sizeof(cmd), fp) != NULL)
            printf("%s", cmd);
        _pclose(fp);
    }

    _getch();

    if (0x00 == cmd[0])
        return true;
    return false;
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
        compile("lineRenderer.vert.shader", "vert", true); return 0;
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