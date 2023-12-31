#ifndef _TTest_h_
#define _TTest_h_
#include "gos.h"

#define TEST_ASSERT(condizione)  if(!(condizione)) return __LINE__;
#define TEST_FAIL_IF(condizione)  if((condizione)) return __LINE__;

//********************************+
template<typename Func, typename... Args>
void TEST(const char *testName, Func fn, Args... args)
{
    gos::console::setBgColor (eBgColor::white);
    gos::console::setTextColor(eTextColor::black);
    gos::logger::log ("%s: TEST BEGIN", testName);
    gos::console::setBgColor (eBgColor::black);
    gos::console::setTextColor(eTextColor::grey);
    gos::logger::log ("\n");
    gos::logger::incIndent();

    const u64 timeNow_usec = gos::getTimeSinceStart_usec();
    const int errorOnLine = fn(args...);
    const u64 timeElapsed_usec = gos::getTimeSinceStart_usec() - timeNow_usec;

    gos::logger::decIndent();
    gos::console::setBgColor (eBgColor::white);
    gos::console::setTextColor(eTextColor::black);
    gos::logger::log ("%s: TEST FINISHED:", testName);

    gos::console::setBgColor (eBgColor::black);
    gos::console::setTextColor(eTextColor::grey);
    gos::logger::log(" ");

    if (errorOnLine == 0)
    {
        gos::console::setBgColor (eBgColor::green);
        gos::console::setTextColor(eTextColor::black);
        gos::logger::log ("SUCCESS");
    }
    else
    {
        gos::console::setBgColor (eBgColor::red);
        gos::console::setTextColor(eTextColor::black);
        gos::logger::log ("FAILED, line %d", errorOnLine);
    }

    gos::console::setBgColor (eBgColor::black);
    gos::console::setTextColor(eTextColor::grey);
    gos::logger::log (", time elapsed: %.02fms", (f32)timeElapsed_usec/1000.0f);
    gos::logger::log ("\n\n");
}

#endif //_TTest_h_