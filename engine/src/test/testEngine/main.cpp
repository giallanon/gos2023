#include "test1.h"
#include "game1.h"


#include "../gosEngine/model/gosModelImport_glTF.h"
void test_modelImport()
{
    struct Vertex 
    {
        gos::vec3f  pos;
        gos::vec2f  tutv0;
        gos::vec3f  normal;
    };

    gos::VtxLayout vtxLayot;
    gos::shape::VtxLayoutWriter writer(&vtxLayot);
    writer.begin()
        .addPos3(offsetof(Vertex, pos))
        .addTexCoord(offsetof(Vertex, tutv0))
        .addNorm3(offsetof(Vertex, normal))
    .end();
    
    
    gos::model::glTFImporter imp;
    gos::model::glTFImporter::Result result;

#ifdef GOS_PLATFORM__WINDOWS
    #define COMMON_PATH "C:/rhea\rheaSRC/gos2023/engine/common_assets"
#else
    #define COMMON_PATH "/home/giallanon/gixprogram/gos2023/engine/common_assets"
#endif


    bool ret = imp.importFromFile (COMMON_PATH "/model3d/omino/omino.glb", vtxLayot, gos::getSysHeapAllocator(), &result);
    //bool ret = imp.importFromFile (COMMON_PATH "/model3d/albero/albero.glb", vtxLayot, gos::getSysHeapAllocator(), &result);
    //bool ret = imp.importFromFile (COMMON_PATH "/model3d/sponza/sponza.glb", vtxLayot, gos::getSysHeapAllocator(), &result);


    if (ret)
    {
	    gos::logger::log (eTextColor::green, "Skeleton:\n");
	    gos::logger::incIndent();	
	    result.skeleton->debug__print(gos::logger::getSystemLogger());
	    gos::logger::decIndent();
    }
}


//******************************** 
int main()
{
    gos::sGOSInit init;
    init.memory_setDefaultForGame();
    
    init.setLogMode (gos::sGOSInit::eLogMode::both_console_and_file);
    if (!gos::init (init, "gosEngine"))
        return -1;


    //test_modelImport(); return 0;


    {
        gos::Engine engine;
        if (!engine.setup (1024, 768, "test engine"))
            return -2;

        if (!engine.asset_build())  return -3;
        //if (!engine.asset_rebuildAll())  return -3;
        {
            //Test1 test;         test.run (&engine);

            Game1 game;       game.run (&engine);

        }
    }
    

    gos::deinit();
    return 0;
}

