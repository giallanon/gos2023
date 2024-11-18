#include "TTest.h"
#include "gosAllocTracker.h"

namespace test_allocTracker
{


namespace test1
{
    static constexpr u32    MAX_MEM = 32 * 1024 * 1024;
    static constexpr u32    NN = 40;

    gos::Random rnd(8734);

	u32 TO_ALLOC[NN];

	bool riservaSpazio (gos::AllocTracker &sm, u32 n, bool bFindBest, gos::AllocTracker::Handle *out)
	{
		if ( sm.alloc (n, bFindBest, out))
		{
			sm.DEBUG_sanityCheck();
			return true;
		}

		sm.DEBUG_sanityCheck();
		return false;
	}

	void liberaSpazio (gos::AllocTracker &sm, gos::AllocTracker::Handle &handle)
	{
		assert (handle.start >= 0 && handle.len > 0);
		sm.dealloc (handle);
		sm.DEBUG_sanityCheck();
	}

	void test1_mischia (u32 *indexes)
	{
		for (u32 t=0; t<NN; t++)
		{
			const u32 i1 = rnd.getU32(NN-1);
			const u32 i2 = rnd.getU32(NN-1);
			if (i1 != i2)
				GOSSWAP(indexes[i1], indexes[i2]);
		}
	}

    int test1 (gos::AllocTracker &sm, bool bFindBest, u32 *indexes)
	{
		gos::AllocTracker::Handle aa[NN];

		u32 totFreeMem = MAX_MEM;
		for (u32 i=0; i<NN; i++)
		{
			const u32 i1 = indexes[i];
			TEST_ASSERT ( riservaSpazio (sm, TO_ALLOC[i1], bFindBest, &aa[i1]) );
			totFreeMem -= TO_ALLOC[i1];
			TEST_ASSERT (sm.getMemLeft() == totFreeMem);
		}

		for (u32 i=0; i<NN; i++)
		{
			//verifico che lo spazio allocato non si sovrapponga ad altri gia' allocati
			u32 start = aa[i].start;
			u32 end = aa[i].start + aa[i].len - 1;
			for (u32 i1=0; i1<NN; i1++)
			{
				if (i1 == i)
					continue;
				const u32 s1 = aa[i1].start;
				const u32 s2 = s1 + aa[i1].len -1;
				TEST_FAIL_IF (start >= s1 && start <= s2);
				TEST_FAIL_IF (end >= s1 && end <= s2);
			}
		}

		//free in ordrine causale
		test1_mischia (indexes);
		for (u32 i=0; i<NN; i++)
		{
			const u32 i1 = indexes[i];
			liberaSpazio (sm, aa[i1]);
			totFreeMem += TO_ALLOC[i1];
			TEST_ASSERT (sm.getMemLeft() == totFreeMem);
		}

		TEST_ASSERT (sm.getMemLeft() == MAX_MEM);


		//adesso faccio un po di alloc e poi un po di free, un po' di alloc e poi un po di free
		totFreeMem = MAX_MEM;
		for (u32 i=0; i<NN; i++)
		{
			const u32 i1 = indexes[i];
			TEST_ASSERT ( riservaSpazio (sm, TO_ALLOC[i1], bFindBest, &aa[i1]) );
			totFreeMem -= TO_ALLOC[i1];
			TEST_ASSERT (sm.getMemLeft() == totFreeMem);

			if (rnd.get01() > 0.7f)
			{
				//faccio un po' di free
				for (u32 i2=0; i2<i; i2++)
				{
					const u32 i1 = indexes[i2];
					if (aa[i1].len > 0)
					{
						if (rnd.get01() > 0.3f)
						{
							liberaSpazio (sm, aa[i1]);
							totFreeMem += TO_ALLOC[i1];
							TEST_ASSERT (sm.getMemLeft() == totFreeMem);
						}
					}
				}
			}
		}

		//free di quello rimasto
		test1_mischia (indexes);
		for (u32 i=0; i<NN; i++)
		{
			const u32 i1 = indexes[i];
			if (aa[i1].len)
			{
				liberaSpazio (sm, aa[i1]);
				totFreeMem += TO_ALLOC[i1];
				TEST_ASSERT (sm.getMemLeft() == totFreeMem);
			}
		}

		TEST_ASSERT (sm.getMemLeft() == MAX_MEM);
        return 0;
	}

    int testMe (bool bFindBest)
    {
        for (u32 i=0; i<NN; i++)
            TO_ALLOC[i] = 1 + rnd.getU32(100);

        gos::AllocTracker sm;
        gos::AllocTracker::Handle h1, h2;
        sm.setup (MAX_MEM);

        
	    TEST_ASSERT ( riservaSpazio (sm, 1, bFindBest, &h1) );
	    liberaSpazio (sm, h1);
        TEST_ASSERT (sm.getMemAllocated() == 0);

	    TEST_ASSERT ( riservaSpazio (sm, MAX_MEM, bFindBest, &h1) );
	    liberaSpazio (sm, h1);
        TEST_ASSERT (sm.getMemAllocated() == 0);

	    TEST_ASSERT ( riservaSpazio (sm, 1, bFindBest, &h1) );
	    TEST_ASSERT ( !riservaSpazio (sm, MAX_MEM, bFindBest, &h1) );
	    TEST_ASSERT ( riservaSpazio (sm, MAX_MEM-1, bFindBest, &h1) );
	    liberaSpazio (sm, h1);
	    liberaSpazio (sm, h2);

        TEST_ASSERT (sm.getMemAllocated() == 0);



        u32 indexes[NN];
        for (u32 i=0; i<NN; i++)
            indexes[i] = i;
        for (u32 i=0; i< 10000; i++)
        {
            TEST_ASSERT( test1 (sm, bFindBest, indexes) );
            test1_mischia (indexes);
        }

        return 0;
    }

    


    int run()
    {
        TEST_ASSERT (testMe(true));
        TEST_ASSERT (testMe(false));
        return 0;
    }
} //namespace test1

} //namespace test_allocTracker


//********************************+
void testAllocTracker (Tester &tester)
{
    tester.run("AllocTracker::test1", test_allocTracker::test1::run);
}