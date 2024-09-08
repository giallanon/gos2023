#ifndef _gosGPUMainLoop_h_
#define _gosGPUMainLoop_h_
#include "../gosGPUEnumAndDefine.h"
#include "../../gos/dataTypes/gosTimer.h"



namespace gos
{
    class GPU;  //fwd decl

    namespace gpu
    {
        /**************************************************
         * MainLoop
         * 
        */
        class MainLoop
        {
        public:
            struct RunResult
            {
                u32     flag;
            };


        public:
                    MainLoop ();
                    ~MainLoop();
                    
            void    setup (gos::GPU *gpuIN);
            void    unsetup();
        
            bool    run ();
            bool    swapchainRecreated() const                                  { return bSwapchainRecreated; }

            bool    canSubmitGFXJob () const;
            void    submitGFXJob (const GPUCmdBufferHandle &cmdBufferHandle);


            void    stat_onCPUFrameBegin()                                      { fpsMegaTimer.onFrameBegin(0); }
            void    stat_onCPUFrameEnd()                                        { fpsMegaTimer.onFrameEnd(0); }
            void    stat_setPrintReportEvery (u32 msec)                         { fpsMegaTimer.setPrintReportEvery(msec); }
            void    stat_printReport()                                          { fpsMegaTimer.printReport(); }


        private:
            static const u8     RESULTBIT_CAN_SUMBIT_GFX_JOB = 0; 

            enum class eStato : u8
            {
                waitingOnFence_inFlight,
                waitingOnFence_swapChainReady,
                askingNewFrame,
                waitingForAJob,
                unknown
            };

        private:
            GPU                 *gpu;
            FPSMegaTimer<3>     fpsMegaTimer;
            eStato              stato;
            VkSemaphore         semaphore_renderFinished;
            VkFence             fence_inFlight;
            VkFence             fenceSwapChainReady;
            GPUCmdBufferHandle  commandBuffer_GFX;

            bool                canAccept_GFXJob;
            bool                bSwapchainRecreated;

        };
    } //namespace gpu
} //namespace gos
#endif //_gosGPUMainLoop_h_