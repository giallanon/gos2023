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
    nextTimeCalc_usec = gos::getTimeSinceStart_usec() + 1000000;
    numFrameCounted = 0;
    frametime_accumulated_usec = 0;
}

//******************************
bool TimerFPS::onFrameEnd()
{
    const u64 timeNow_usec = gos::getTimeSinceStart_usec();
    if (0 != frametime_started_usec)
    {
        frametime_accumulated_usec += (timeNow_usec - frametime_started_usec);
        numFrameCounted++;
        frametime_started_usec = 0;
    }

    if (timeNow_usec >= nextTimeCalc_usec)
    {
        frametime_avg_usec = frametime_accumulated_usec / (float)numFrameCounted;
        reset();
        return true;
    }

    return false;
}
