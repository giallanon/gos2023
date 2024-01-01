#include "gos.h"
#include "TTest.h"

void testGos (Tester &tester);
void testMath(Tester &tester);
void testThread (Tester &tester);

//********************************+
int main()
{
    gos::sGOSInit init;
    init.setDefaultForGame();
    if (!gos::init (init, "testKernel"))
        return 1;


    Tester tester;
    {
        testGos (tester);
        testMath(tester);
        testThread (tester);
    }
    tester.printReport();

    gos::deinit();

#ifdef GOS_PLATFORM__WINDOWS
    printf ("\nPress any key to terminate\n");
    _getch();
#endif

    return 0;
}