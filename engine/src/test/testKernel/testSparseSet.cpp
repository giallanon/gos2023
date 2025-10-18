#include "TTest.h"
#include "gosSparseSet.h"

namespace test_sparseset
{


//*******************************
namespace test1
{
    struct Comp1
    {
        u32 num32;
        f32 fl32;
    };

    u32 calcNumPage (u32 numAllocatedElem, u32 numElemPerPage)
    {
        u32 n = numAllocatedElem / numElemPerPage;
        if (n * numElemPerPage < numAllocatedElem)
            n++;
        return n;
    }

    int run()
    {
        // static constexpr u32 PAGE_SIZE = 32;
        // static constexpr u32 DENSELIST_INITIAL_NUM_ELEM = 8;
        // static constexpr u16 NUM = 1000;

        static constexpr u32 PAGE_SIZE = 2048;
        static constexpr u32 DENSELIST_INITIAL_NUM_ELEM = 1024;
        static constexpr u32 NUM = 100000;

        gos::SparseSet<Comp1, PAGE_SIZE, DENSELIST_INITIAL_NUM_ELEM> ss1;
        ss1.setup (gos::getSysHeapAllocator());

        
        for (u32 i=0; i<NUM; i++)
        {
            Comp1 *comp = ss1.addIfNotExists(i);
            TEST_ASSERT (NULL != comp);
            comp->fl32 = (f32)i * 90.0f;
            comp->num32 = i;
        }

        TEST_ASSERT (ss1.debug_denseList_getNumElem() == NUM);
        TEST_ASSERT (ss1.debug_pageList_getNumElem() == calcNumPage(NUM, PAGE_SIZE));


        for (u32 i=0; i<NUM; i++)
        {
            Comp1 *comp = ss1.get(i);
            TEST_ASSERT (NULL != comp);
            TEST_ASSERT (comp->num32 == i);
            TEST_ASSERT (comp->fl32 == (f32)i * 90.0f);
        }

        for (u32 i=0; i<NUM; i++)
        {
            if (i % 2 == 0)
                ss1.remove(i);
        }        
        TEST_ASSERT (ss1.debug_denseList_getNumElem() == NUM / 2);
        TEST_ASSERT (ss1.debug_pageList_getNumElem() == calcNumPage(NUM, PAGE_SIZE));


        for (u32 i=0; i<NUM; i++)
        {
            if (i % 2 != 0)
            {
                Comp1 *comp = ss1.get(i);
                TEST_ASSERT (NULL != comp);
                TEST_ASSERT (comp->num32 == i);
                TEST_ASSERT (comp->fl32 == (f32)i * 90.0f);
            }
        }
        
        
        for (u32 i=0; i<PAGE_SIZE; i++)
        {
            ss1.remove(i);
        }
        TEST_ASSERT (ss1.debug_pageList_getNumElem() == calcNumPage(NUM, PAGE_SIZE) - 1);        


        gos::SparseSetIter iter;
        {
            u32 entityIndex;
            Comp1 *comp1;
            ss1.toStart (&iter);
            while (ss1.next(iter, &comp1, &entityIndex))
            {
                Comp1 *comp2 = ss1.get(entityIndex);
                TEST_ASSERT (NULL != comp2);
                TEST_ASSERT (comp2 == comp1);
                TEST_ASSERT (comp1->num32 == entityIndex);
                TEST_ASSERT (comp2->fl32 == (f32)entityIndex * 90.0f);
            }
        }

        return 0;
    }
}

} //namespace test_sparseset

//********************************+
void testSparseSet (Tester &tester)
{
    tester.run("sparseSet::test1", test_sparseset::test1::run);
}
