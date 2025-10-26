#include "test1.h"


//******************************** 
#include "gosEngine_fixedSizeBufferTracker.h"
void test_fixedSizeBufferTracker()
{
    gos::engine::FixedSizeBufferTracker tracker;

    static constexpr u32 NUM_MAX_OBJECT = 1024;
    tracker.setup (gos::getSysHeapAllocator(), NUM_MAX_OBJECT);

    gos::FastArray<gos::engine::ResHandle> handleList;
    gos::FastArray<gos::engine::ResHandle> handleList2;
    handleList.setup (gos::getSysHeapAllocator(), NUM_MAX_OBJECT);
    handleList2.setup (gos::getSysHeapAllocator(), NUM_MAX_OBJECT);

    for (u32 i=0; i<NUM_MAX_OBJECT; i++)
    {
        gos::engine::ResHandle handle;
        assert (tracker.bind (&handle));
        handleList.append (handle);
        handleList2.append (handle);
    }

    for (u32 i=0; i<NUM_MAX_OBJECT; i++)
    {
        gos::engine::ResHandle handle;
        assert (false == tracker.bind (&handle));
    }

    for (u32 i=0; i<NUM_MAX_OBJECT; i++)
    {
        assert (tracker.isBound (handleList(i)));
    } 

    for (u32 i=0; i<NUM_MAX_OBJECT; i++)
    {
        auto handle = handleList(0);
        handleList.removeAndSwapWithLast(0);

        assert (tracker.isBound (handle));
        tracker.unbind (handle);
        assert (false == tracker.isBound (handle));

        //printf ("%d..", i);
        for (u32 i2=0; i2<handleList.getNElem(); i2++)
        {
            assert (tracker.isBound(handleList(i2)));
        }
    } 
    //printf ("\n");


    //secondo giro
    handleList.reset();
    for (u32 i=0; i<NUM_MAX_OBJECT; i++)
    {
        gos::engine::ResHandle handle;
        assert (tracker.bind (&handle));
        handleList.append (handle);
    }

    for (u32 i=0; i<NUM_MAX_OBJECT; i++)
    {
        assert (tracker.isBound (handleList(i)));
        assert (!tracker.isBound (handleList2(i)));
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

    //testEntityList(); return 0;
    //test_fixedSizeBufferTracker(); return 0;

    {
        gos::Engine engine;
        if (!engine.setup (1024, 768, "test engine"))
            return -2;

        //if (!engine.assetHub_rebuildAll())  return -3;
        if (!engine.assetHub_buildAll())  return -3;

        {
            Test1   test;
            test.run (&engine);
        }
    }
    

    gos::deinit();
    return 0;
}

