#ifndef _FPSMegaTimer_h_
#define _FPSMegaTimer_h_
#include "../../gos/gosEnumAndDefine.h"
#include "../../gos/dataTypes/gosTimer.h"


#define FPSTIMER_CPU    0
#define FPSTIMER_GPU    1
#define FPSTIMER_FPS    2

/****************************************
 * FPSMegaTimer
 * 
 * 
 */
class FPSMegaTimer
{
public:
            FPSMegaTimer()
            {
                nextTimePrintReport_msec = 0;

                memset (nameList, 0, sizeof(nameList));
                sprintf_s (nameList[0], sizeof(nameList[0]), "CPU");
                sprintf_s (nameList[1], sizeof(nameList[1]), "GPU");
                sprintf_s (nameList[2], sizeof(nameList[2]), "FPS");
                
                for (u32 i=0; i<N_TIMER; i++)
                {
                    timerList[i].onFrameBegin();
                    avgFrameTime_usec[i] = 0;
                    avgFPS[i] = 0;
                }
            }

    void    onFrameBegin (u32 i)                        { timerList[i].onFrameBegin(); }

    void    onFrameEnd(u32 i)
            {
                if (timerList[i].onFrameEnd())
                {
                    avgFrameTime_usec[i] = timerList[i].getAvgFrameTime_usec();
                    avgFPS[i] = timerList[i].getAvgFPS();
                }
            }

    void    printReport()
            {
                const u64 timeNow_msec = gos::getTimeSinceStart_msec();
                if (timeNow_msec < nextTimePrintReport_msec)
                    return;
                nextTimePrintReport_msec = timeNow_msec + 1000;

                char s[1024];
                memset (s, 0, sizeof(s));
                for (u32 i=0; i<N_TIMER; i++)
                {
                    char temp[256];
                    const f32 msec = avgFrameTime_usec[i] / 1000.0f;
                    sprintf_s (temp, sizeof(temp), "%s: avg %.2fms [fps: %.01f]", nameList[i], msec, avgFPS[i]);

                    strcat_s (s, sizeof(s), temp);
                    if (i == N_TIMER-1)
                        strcat_s (s, sizeof(s), "\n");
                    else
                        strcat_s (s, sizeof(s), "      ");
                }
                printf (s);
            }    

private:
    static const u8 N_TIMER = 3;

private:
    gos::TimerFPS   timerList[N_TIMER];
    f32             avgFrameTime_usec[N_TIMER];
    f32             avgFPS[N_TIMER];
    u64             nextTimePrintReport_msec;
    char            nameList[N_TIMER][32];
};

#endif //_FPSMegaTimer_h_
