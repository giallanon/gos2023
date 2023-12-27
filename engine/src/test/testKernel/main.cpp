#include "gos.h"

void testGos();
void testMath();

//********************************+
int main()
{
    gos::sGOSInit init;
    init.setDefaultForGame();
    if (!gos::init (init, "testKernel"))
        return 1;


    testGos();
    testMath();

    gos::deinit();

#ifdef GOS_PLATFORM__WINDOWS
    printf ("\nPress any key to terminate\n");
    _getch();
#endif

    return 0;
}