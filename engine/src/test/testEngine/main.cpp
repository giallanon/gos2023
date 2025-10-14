#include "test1.h"

//******************************** 
#include "entity/gosEntityList.h"
void testEntityList()
{
    gos::ent::List list;

    list.setup (gos::getSysHeapAllocator());

    static constexpr u32 NUM_ENTITIES = 40000;
    gos::FastArray<gos::Entity> entityList;
    entityList.setup (gos::getSysHeapAllocator(), NUM_ENTITIES);

    for (u32 i = 0; i < NUM_ENTITIES; i++)
    {
        const u32 pageIndex = gos::randomU32(100) * gos::ent::List::PAGE_SIZE;
        entityList[i].id = pageIndex + i;
    }


    list.reset();
    for (u32 i = 0; i < entityList.getNElem(); i++)
    {
        assert (list.addIfNotExists (entityList(i)));
    }
    for (u32 i = 0; i < entityList.getNElem(); i++)
    {
        assert (!list.addIfNotExists (entityList(i)));
    }


    auto ll = list.getList();
    for (u32 i = 0; i < entityList.getNElem(); i++)
    {
        const gos::Entity ent = entityList(i);
        assert (ll->simpleSearch(ent) != u32MAX);
    }



    assert (list.getCount() == entityList.getNElem());
    assert (list.getList()->getNElem() == entityList.getNElem());
}


//******************************** 
int main()
{
    gos::sGOSInit init;
    init.memory_setDefaultForGame();
    
    init.setLogMode (gos::sGOSInit::eLogMode::both_console_and_file);
    if (!gos::init (init, "gosEngine"))
        return -1;

    testEntityList(); return 0;


    {
        gos::Engine engine;
        if (!engine.setup (1024, 768, "test engine"))
            return -2;

        Test1   test;
        test.run (&engine);
    }
    

    gos::deinit();
    return 0;
}

