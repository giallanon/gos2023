#ifndef _gosTimer_h_
#define _gosTimer_h_
#include "../gosEnumAndDefine.h"
#include "../gos.h"

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

        f32             getAvgFrameTime_usec() const                                                { return avgFrameTime_usec; }
        f32             getAvgFPS() const                                                           { return 1000000.0f / avgFrameTime_usec; }
        
    private:
        u64             timeBegin_usec;
        u64				timeFrameStarted_usec;
        u64             accumulatedTime_usec;
        u32             numFrameCounted;
        f32             avgFrameTime_usec = 0;
    };


    /**
    * @brief    FPSMegaTimer
    *           Mantiene un elenco di TimerFPS
    */
    template <int MAX_NUM_TIMER>
    class FPSMegaTimer
    {
    public:
                FPSMegaTimer()
                {
                    nextTimePrintReport_msec = 0;
                    print_report_every_msec = 1000;
                    numTimer = 0;
                }

        u8      addTimer (const char *name)
                {
                    assert (numTimer < MAX_NUM_TIMER);
                    
                    //sprintf_s (nameList[0], sizeof(nameList[0]), "CPU");
                    //sprintf_s (nameList[1], sizeof(nameList[1]), "GPU");
                    //sprintf_s (nameList[2], sizeof(nameList[2]), "FPS");
                    sprintf_s (timerList[numTimer].name, sizeof(timerList[numTimer].name), "%s", name);
                    timerList[numTimer].timer.onFrameBegin();
                    timerList[numTimer].avgFrameTime_usec = 0;
                    timerList[numTimer].avgFPS = 0;
                    return numTimer++;
                }

        void    onFrameBegin (u32 i)                        { timerList[i].timer.onFrameBegin(); }

        void    onFrameEnd(u32 i)
                {
                    if (timerList[i].timer.onFrameEnd())
                    {
                        timerList[i].avgFrameTime_usec = timerList[i].timer.getAvgFrameTime_usec();
                        timerList[i].avgFPS = timerList[i].timer.getAvgFPS();
                    }
                }

        void    setPrintReportEvery (u32 msec)              { print_report_every_msec = msec; }
        void    printReport()
                {
                    const u64 timeNow_msec = gos::getTimeSinceStart_msec();
                    if (timeNow_msec < nextTimePrintReport_msec)
                        return;
                    nextTimePrintReport_msec = timeNow_msec + print_report_every_msec;

                    char s[1024];
                    memset (s, 0, sizeof(s));
                    for (u8 i=0; i<numTimer; i++)
                    {
                        char temp[256];
                        const f32 msec = timerList[i].avgFrameTime_usec / 1000.0f;
                        sprintf_s (temp, sizeof(temp), "%s: avg %.2fms [fps: %.01f]", timerList[i].name, msec, timerList[i].avgFPS);

                        strcat_s (s, sizeof(s), temp);
                        if (i == numTimer -1)
                            strcat_s (s, sizeof(s), "\n");
                        else
                            strcat_s (s, sizeof(s), "      ");
                    }
                    printf (s);
                }    


    private:
        struct sATimer
        {
            char            name[16];
            gos::TimerFPS   timer;
            f32             avgFrameTime_usec;
            f32             avgFPS;
        };

    private:
        sATimer         timerList[MAX_NUM_TIMER];
        u64             nextTimePrintReport_msec;
        u32             print_report_every_msec;
        u8              numTimer;
    };    

} // namespace gos


#endif // _gosTimer_h_
