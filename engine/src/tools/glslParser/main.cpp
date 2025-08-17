#include "SPVReflect.h"
#include "SPVDataType.h"
#include "PipelineParser.h"

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
void test_reflect_1()
{
    compile("phong.vert.shader", "vert", true);
    compile("phong.frag.shader", "frag", true);
    SPVReflect parser;
    if (parser.parseFromFile ("@ex/compiled/phong.vert.shader.spv", "@ex/compiled/phong.frag.shader.spv"))
        parser.printInfo();
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
    compile("shader_noVtxDecl.vert", "vert", true);
    compile("shader_noVtxDecl.frag", "frag", true);
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
    compile("lineRenderer.vert", "vert", true);
    compile("lineRenderer.frag", "frag", true);
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
    
    if (!pp.parseFromFile ("@ex/pipeline1.txt", &out))
    {
        gos::err::clear();
    }
    
    assert (out.outputRT_fmt == eImageFormat::_SAME_AS_CURRENT_SWAPCHAIN);
}

//******************************** 
void test_SPVDataType()
{
    SPVDataTypeDefinition dt;

    dt.begin();
        dt.add_simple ("var1", eDataFormat::_1f32);
        dt.add_simple ("var2", eDataFormat::_2f32);
        
        dt.begin_struct ("struct1");
            dt.add_simple ("s1-var1", eDataFormat::_2f32);
            dt.add_simple ("s1-var2", eDataFormat::_1i8);
        dt.end_struct();

        dt.begin_struct ("struct2");
            dt.add_simple ("s2-var1", eDataFormat::_2i32);
            dt.begin_struct ("struct3");
                dt.add_simple ("s2-s3-var1", eDataFormat::_1u8);
                dt.add_simple ("s2-s3-var2", eDataFormat::_4f32);
            dt.end_struct();
            dt.add_simple ("s2-var2", eDataFormat::_mat2x2);
            dt.add_simple ("s2-var3", eDataFormat::_mat3x3);
        dt.end_struct();
        
        dt.add_simple ("var3", eDataFormat::_3f32);

        dt.begin_array ("arr1", 3);
            dt.add_simple ("arr1-1", eDataFormat::_2f32);
        dt.end_array();


        dt.begin_array ("arr2", 12);
            dt.begin_struct ("struct3");
                dt.add_simple ("arr2-1", eDataFormat::_2f32);
                dt.add_simple ("arr2-2", eDataFormat::_1i32);
            dt.end_struct();
        dt.end_array();        
    dt.end();

    dt.debug_print_just_names();
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

        test_SPVDataType();
        //test_reflect_1();
        //test_reflect_2();
        //test_reflect_3();
        //test_pipelineParser_1 ();
    }
    

#ifdef GOS_PLATFORM__WINDOWS
    _getch();
#endif
    
    gos::deinit();
    return 0;
}