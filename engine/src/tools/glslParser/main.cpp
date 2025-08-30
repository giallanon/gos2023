#include "SPVReflect.h"
#include "PipelineParser.h"


using namespace gos;


//******************************** 
bool compile (const char *shaderSRCFile, const char *shaderStage, const char *firstDefine=NULL, ...)
{
    //se esistono delle define da passare al compilatore...
    char defineList[1024];
    memset (defineList, 0, sizeof(defineList));
    if (NULL != firstDefine)
    {
        va_list argptr;
        va_start (argptr, firstDefine);

        const char *def = firstDefine;
        while (1)
        {
            strcat_s (defineList, sizeof(defineList), "-D");
            strcat_s (defineList, sizeof(defineList), def);
            strcat_s (defineList, sizeof(defineList), " ");

            def = va_arg(argptr, const char *);
            if (NULL == def)
                break;
        }
        va_end(argptr);
    }


    //glslc -fshader-stage=vert --target-env=vulkan1.3 lineRenderer.vert.shader -g -O -o lineRenderer.vert.spv
    char cmd[2048];
    sprintf_s (cmd, sizeof(cmd), "glslc -fshader-stage=%s --target-env=vulkan1.3 %s %s/example/%s -g -O -o %s/example/compiled/%s.spv 2>&1", 
        shaderStage,
        defineList,
        gos::getAppPathNoSlash(), shaderSRCFile,
        gos::getAppPathNoSlash(), shaderSRCFile
    );

    printf ("%s\n", cmd);

    char *result;
    u32 len;
    if (!gos::runShellScriptAndStoreResult (cmd, gos::getScrapAllocator(), &result, &len))
        return false;

    if (NULL == result)
        return true;

    //c'e' stato qualche errore di compilazione
    printf("(%d): %s", len, result);
    GOSFREE_SCRAP(result);
    return false;
}


//******************************** 
void test_reflect_1()
{
    if (!compile("phong.vert.shader", "vert", "SBBO2_1", "SBBO2_1_NUM_ELEM=4", NULL)) return;
    if (!compile("phong.frag.shader", "frag", "SBBO2_1", "SBBO2_1_NUM_ELEM=4", NULL)) return;
    SPVReflect parser;
    if (parser.parseFromFile ("@ex/compiled/phong.vert.shader.spv", "@ex/compiled/phong.frag.shader.spv"))
    {
        parser.save ("@ex/compiled/phong.dat");
        parser.printInfo();
    }
    else
    {
        assert (gos::err::anyError());
        printf (gos::err::getErrByIndex(0));
        gos::err::clear();
    }    
}

//******************************** 
void test_reflect_2()
{
    if (!compile("shader_noVtxDecl.vert", "vert")) return;
    if (!compile("shader_noVtxDecl.frag", "frag")) return;
    SPVReflect parser;
    if (parser.parseFromFile ("@ex/compiled/shader_noVtxDecl.vert.spv", "@ex/compiled/shader_noVtxDecl.frag.spv"))
        parser.printInfo();
    else
    {
        assert (gos::err::anyError());
        printf (gos::err::getErrByIndex(0));
        gos::err::clear();
    }    
}

//******************************** 
void test_reflect_3()
{
    if (!compile("lineRenderer.vert", "vert")) return;
    if (!compile("lineRenderer.frag", "frag")) return;
    SPVReflect parser;
    if (parser.parseFromFile ("@ex/compiled/lineRenderer.vert.spv", "@ex/compiled/lineRenderer.frag.spv"))
        parser.printInfo();
    else
    {
        assert (gos::err::anyError());
        printf (gos::err::getErrByIndex(0));
        gos::err::clear();
    }    
}

//******************************** 
void test_pipelineParser_1 ()
{
    PipelineParser pp;

    gos::PipelineDef out;
    
    if (!pp.createFromIniFile ("@ex/pipeline1.txt", &out))
    {
        gos::err::clear();
    }
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
        fs::addAlias ("@ex", "example", eAliasPathMode::relativeToAppFolder);

        //test_reflect_1();   //phong
        //test_reflect_2(); //shader_noVtxDecl
        //test_reflect_3(); //lineRenderer
 
        test_pipelineParser_1 ();
    }
    

#ifdef GOS_PLATFORM__WINDOWS
    _getch();
#endif
    
    gos::deinit();
    return 0;
}