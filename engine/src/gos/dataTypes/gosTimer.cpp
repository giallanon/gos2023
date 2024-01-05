#include "../gos.h"
#include "gosTimer.h"


using namespace gos;


/************************************************************************************************************************
 * Timer
 * 
 *************************************************************************************************************************/
void Timer::start()
{
    timeStarted_usec = gos::getTimeSinceStart_usec();
}

//****************************
u64 Timer::elapsed_usec() const
{
	return gos::getTimeSinceStart_usec() - timeStarted_usec;
}





/************************************************************************************************************************
 * Timer FPS
 * 
 *************************************************************************************************************************/
void TimerFPS::reset()
{
    timeBegin_usec = gos::getTimeSinceStart_usec();
    numFrameCounted = 0;
    accumulatedTime_usec = 0;
}

//******************************
bool TimerFPS::onFrameEnd()
{
    const u64 timeNow_usec = gos::getTimeSinceStart_usec();
    accumulatedTime_usec += (timeNow_usec - timeFrameStarted_usec);
    numFrameCounted++;

    if (timeNow_usec - timeBegin_usec >= 1000000)
    {
        avgFrameTime_usec = accumulatedTime_usec / (float)numFrameCounted;
        reset();
        return true;
    }

    return false;
}
