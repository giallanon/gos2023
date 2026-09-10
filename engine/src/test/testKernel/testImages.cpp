#include "TTest.h"
#include "gosImageBuffer.h"

using namespace gos;

namespace test_images
{
//*******************************
namespace test1
{
    static constexpr u32 BUFF1_ROWS = 16;
    static constexpr u32 BUFF1_COLS = 8;
    static constexpr u32 BUFF2_ROWS = 512;
    static constexpr u32 BUFF2_COLS = 256;

    struct OneElem
    {
        u32 xx;
        u32 yy;
    };

    bool check (const image::BufferDescr &descr, const OneElem *p, u32 startX, u32 startY, u32 dimX, u32 dimY, u32 firstXX, u32 firstYY)
    {
        for (u32 y = 0; y < descr.num_row; y++)
        {
            u32 ct = y * descr.num_col;
            for (u32 x = 0; x < descr.num_col; x++)
            {
                if (x >= startX && x <= startX + dimX-1 && y >= startY && y <= startY + dimY-1)
                {
                    if (p[ct].xx != firstXX + x - startX)
                        return false;
                    if (p[ct].yy != firstYY + y - startY) 
                        return false;
                }
                else
                {
                    if (p[ct].xx) 
                        return false;
                    if (p[ct].yy) 
                        return false;
                }

                ct++;
            }
        }
        return true;
    }

    int do_run(OneElem *buffer1, OneElem *buffer2)
    {
        image::BufferDescr  bd1;
        bd1.setup (BUFF1_ROWS, BUFF1_COLS, sizeof(OneElem), sizeof(OneElem) * BUFF1_COLS);
        image::BufferDescr  bd2;
        bd2.setup (BUFF2_ROWS, BUFF2_COLS, sizeof(OneElem), sizeof(OneElem) * BUFF2_COLS);


        for (u32 y = 0; y < BUFF1_ROWS; y++)
        {
            for (u32 x = 0; x < BUFF1_COLS; x++)
            {
                buffer1[x + y * BUFF1_COLS].xx = 100 + x;
                buffer1[x + y * BUFF1_COLS].yy = 100 + y;
            }
        }

        memset (buffer2, 0, bd2.sizeof_one_row * bd2.num_row);
        TEST_ASSERT( true == image::blt (bd1, buffer1, 0, 0, BUFF1_COLS, BUFF1_ROWS, bd2, buffer2, 0, 0) );
        TEST_ASSERT( true == check (bd2, buffer2, 0, 0, BUFF1_COLS, BUFF1_ROWS,    100, 100) );

        memset (buffer2, 0, bd2.sizeof_one_row * bd2.num_row);
        TEST_ASSERT( true == image::blt (bd1, buffer1, 0, 0, BUFF1_COLS, BUFF1_ROWS, bd2, buffer2, 1, 0) );
        TEST_ASSERT( true == check (bd2, buffer2, 1, 0, BUFF1_COLS, BUFF1_ROWS,    100, 100) );

        memset (buffer2, 0, bd2.sizeof_one_row * bd2.num_row);
        TEST_ASSERT( true == image::blt (bd1, buffer1, 0, 0, BUFF1_COLS, BUFF1_ROWS, bd2, buffer2, 0, 1) );
        TEST_ASSERT( true == check (bd2, buffer2, 0, 1, BUFF1_COLS, BUFF1_ROWS,    100, 100) );

        memset (buffer2, 0, bd2.sizeof_one_row * bd2.num_row);
        TEST_ASSERT( true == image::blt (bd1, buffer1, 0, 0, BUFF1_COLS, BUFF1_ROWS, bd2, buffer2, 1, 1) );
        TEST_ASSERT( true == check (bd2, buffer2, 1, 1, BUFF1_COLS, BUFF1_ROWS,    100, 100) );

        memset (buffer2, 0, bd2.sizeof_one_row * bd2.num_row);
        TEST_ASSERT( true == image::blt (bd1, buffer1, 2, 3, 4, 4, bd2, buffer2, 1, 1) );
        TEST_ASSERT( true == check (bd2, buffer2, 1, 1, 4, 4,     102, 103) );

        memset (buffer2, 0, bd2.sizeof_one_row * bd2.num_row);
        TEST_ASSERT( true == image::blt (bd1, buffer1, 0, 0, 8, 4, bd2, buffer2, -1, -1) );
        TEST_ASSERT( true == check (bd2, buffer2, 0, 0, 7, 3,     101, 101) );
        return 0;
    }

    int run()
    {
        gos::Allocator *localAllocator = gos::getSysHeapAllocator();
        OneElem *buffer1 = GOSALLOCT(OneElem*, localAllocator, sizeof(OneElem) * BUFF1_ROWS * BUFF1_COLS);
        OneElem *buffer2 = GOSALLOCT(OneElem*, localAllocator, sizeof(OneElem) * BUFF2_ROWS * BUFF2_COLS);

        int ret = do_run (buffer1, buffer2);

        GOSFREE(localAllocator, buffer1);
        GOSFREE(localAllocator, buffer2);

        return ret;
    }
}//namespace test1
} //namespace test_images

//********************************+
void testImages (Tester &tester)
{
    tester.run("test_images::test1", test_images::test1::run);
}
