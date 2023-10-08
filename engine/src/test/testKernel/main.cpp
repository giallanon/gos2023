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
    return 0;
}