#ifndef _gosTimer_h_
#define _gosTimer_h_
#include "../gosEnumAndDefine.h"

namespace gos
{
    /**********************************************************
    *	Timer
    **********************************************************/
    class Timer
    {
    public:
                        Timer()                                                                { }
        
        void            start();
        u64             elapsed_usec() const;
                            //ritorna il time elapsed (in us) dal precedente start()

    private:
        u64				timeStarted_usec;
    };

    /**********************************************************
    *	TimerFPS
    **********************************************************/
    class TimerFPS
    {
    public:
                        TimerFPS()                                                                  { reset(); avgFrameTime_usec = 0; }
        
        void            reset();
        void            onFrameBegin()                                                              { timeFrameStarted_usec = gos::getTimeSinceStart_usec(); }
        bool            onFrameEnd();
                            //ritorna true dopo circa 1 secondo accumulato. Quando ritorna true,
                            //la fn getAvgFrameTime_usec() ritorna il valor medio del frame time accumulato 

        f32             getAvgFrameTime_usec() const { return avgFrameTime_usec; }

    private:
        u64             timeBegin_usec;
        u64				timeFrameStarted_usec;
        u64             accumulatedTime_usec;
        u32             numFrameCounted;
        f32             avgFrameTime_usec = 0;
    };    

} // namespace gos


#endif // _gosTimer_h_
