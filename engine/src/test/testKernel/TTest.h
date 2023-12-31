#ifndef _TTest_h_
#define _TTest_h_
#include "gos.h"


//*******************************************
class Tester
{
public:
            Tester()                    { reset(); }
    void    reset()                     { numTotTest = numFailed = 0; }
    void    printReport() const         
            {
                gos::console::setBgColor (eBgColor::black);
                gos::logger::log ("\n");

                if (0 == numFailed)
                {
                    gos::console::setBgColor (eBgColor::green);
                    gos::console::setTextColor(eTextColor::black);
                }
                else
                {
                    gos::console::setBgColor (eBgColor::red);
                    gos::console::setTextColor(eTextColor::black);
                }

                gos::logger::log ("Final report:  num failed %d / %d", numFailed, numTotTest);

                gos::console::setBgColor (eBgColor::black);
                gos::logger::log ("\n");
                gos::logger::log ("\n");
            }

    template<typename Func, typename... Args>
    void run (const char *testName, Func fn, Args... args)
    {
        numTotTest++;

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
            numFailed++;
            gos::console::setBgColor (eBgColor::red);
            gos::console::setTextColor(eTextColor::black);
            gos::logger::log ("FAILED, line %d", errorOnLine);
        }

        gos::console::setBgColor (eBgColor::black);
        gos::console::setTextColor(eTextColor::grey);
        gos::logger::log (", time elapsed: %.02fms", (f32)timeElapsed_usec/1000.0f);
        gos::logger::log ("\n\n");
    }

private:
    u32 numTotTest;
    u32 numFailed;
};


#define TEST_ASSERT(condizione)  if(!(condizione)) return __LINE__;
#define TEST_FAIL_IF(condizione)  if((condizione)) return __LINE__;

/********************************+
template<typename Func, typename... Args>
void TEST(sTestResult &testResult, const char *testName, Func fn, Args... args)
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
*/

#endif //_TTest_h_