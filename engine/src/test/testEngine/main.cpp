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


int perftest1()
{
    const u32 N = 100000;
    gos::mat4x4f *p = (gos::mat4x4f*)malloc(sizeof(gos::mat4x4f) * N);
    //gos::mat4x4f *p = GOSALLOCT(gos::mat4x4f*, gos::getSysHeapAllocator(), sizeof(gos::mat4x4f) * N);
    //gos::mat4x4f *p = (gos::mat4x4f*)GOSALIGNEDALLOC(gos::getSysHeapAllocator(), sizeof(gos::mat4x4f) * N, 64);

    //printf ("---%d\n", alignof(gos::mat4x4f)); _getch();

    gos::FastArray<gos::mat4x4f> p2;
    p2.setup (gos::getSysHeapAllocator(), N);
    p2[N-1].identity();

    u64 totalTime[8] = { 0 };
    u64 timeStarted, timeElpased;

    for (u32 iii = 0; iii < 100; iii++)
    {
        //==== 1
        timeStarted = gos::getTimeSinceStart_usec();
        for (u32 ii = 0; ii < 10; ii++)
        {
            for (u32 i = 0; i < N; i++)
            {
                p[i].identity();
            }
        }
        timeElpased = gos::getTimeSinceStart_usec() - timeStarted;
        totalTime[0] += timeElpased;
        printf ("%.2f     ", ((f32)timeElpased / 1000.0f) / 10.0f);

        //==== 2
        timeStarted = gos::getTimeSinceStart_usec();
        for (u32 ii = 0; ii < 10; ii++)
        {
            for (u32 i = 0; i < N; i++)
            {
                p2.getElem(i).identity();
            }
        }
        timeElpased = gos::getTimeSinceStart_usec() - timeStarted;
        totalTime[1] += timeElpased;
        printf ("%.2f    ", ((f32)timeElpased / 1000.0f) / 10.0f);

        //==== 3
        timeStarted = gos::getTimeSinceStart_usec();
        for (u32 ii = 0; ii < 10; ii++)
        {
            p2.forEach([](u32 index, gos::mat4x4f &matrix) {
                matrix.identity();
                return true;
            });
        }
        timeElpased = gos::getTimeSinceStart_usec() - timeStarted;
        totalTime[2] += timeElpased;
        printf ("%.2f    ", ((f32)timeElpased / 1000.0f) / 10.0f);


        //==== 4
        timeStarted = gos::getTimeSinceStart_usec();
        gos::mat4x4f *pp = p2._getTypedPointer();
        for (u32 ii = 0; ii < 10; ii++)
        {
            for (u32 i = 0; i < N; i++)
            {
                pp[i].identity();
            }
        }
        timeElpased = gos::getTimeSinceStart_usec() - timeStarted;
        totalTime[3] += timeElpased;
        printf ("%.2f    ", ((f32)timeElpased / 1000.0f) / 10.0f);



        printf ("\n");
        for (u32 i = 0; i < N; i++)
        {
            if (memcmp (p2[i]._getValuesPt(), p[i]._getValuesPt(), sizeof(gos::mat4x4f)) != 0)
                return 1;
        }

    }

    printf ("----------------------------------------------\n");
    printf ("%.2f    %.2f    %.2f    %.2f    \n", 
        ((f32)totalTime[0] / 1000.0f) / 10.0f,
        ((f32)totalTime[1] / 1000.0f) / 10.0f,
        ((f32)totalTime[2] / 1000.0f) / 10.0f,
        ((f32)totalTime[3] / 1000.0f) / 10.0f
    );
    _getch();
    return 0;
}

//******************************** 
int main()
{
    gos::sGOSInit init;
    init.memory_setDefaultForGame();
    
    init.setLogMode (gos::sGOSInit::eLogMode::both_console_and_file);
    if (!gos::init (init, "gosEngine"))
        return -1;

    //return perftest1();

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

