#ifndef _gosGPUMainLoop_h_
#define _gosGPUMainLoop_h_
#include "../gosGPUEnumAndDefine.h"
#include "../vulkan/gosGPUVulkanEnumAndDefine.h"
#include "../../gos/dataTypes/gosTimer.h"


namespace gos
{
    class GPU;  //fwd decl

    namespace gpu
    {
        class AquireSwapChainImage
        {
        public:
                    AquireSwapChainImage()          { gpu = NULL; stato = eStato::idle; imageIndex=u32MAX; }
                    ~AquireSwapChainImage()         { unsetup(); }

            void    setup (gos::GPU *gpuIN);
            void    unsetup();
            bool    tryAcquire (VkImage *out_image);
            void    submit (const GPUCmdBufferHandle &cmdBufferHandle);

        public:
            u32             imageIndex;
            gos::TimerFPS   timerFPS;

        private:
            enum class eStato : u8
            {
                idle,
                acquiring,
                waitingFence,
                acquired,
                jobInProgress
            };

        private:
            GPU         *gpu;
            VkFence     fence;
            eStato      stato;
        };

        class PresentGFXJob
        {
        public:
            PresentGFXJob()                         { gpu = NULL; stato = eStato::idle; }
            ~PresentGFXJob()                        { unsetup(); }

            void    setup (gos::GPU *gpuIN);
            void    unsetup();

            void    submit (const GPUCmdBufferHandle &cmdBufferHandle, u32 swapChainImageIndex);
            bool    hasFinished();

        private:
            enum class eStato : u8
            {
                idle,
                jobInProgress
            };

        private:
            GPU         *gpu;
            VkFence     fence;
            u32         swapChainImageIndex;
            u32         swapChainAutoID;
            eStato      stato;
        };


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
                fenceWaiting_swapChainImg,
                askingNewSwapchainImg,
                waitingForAJob,
                unknown
            };

        private:
            GPU                 *gpu;
            FPSMegaTimer<3>     fpsMegaTimer;
            eStato              stato;
            VkSemaphore         semaphore_renderFinished;
            VkFence             fence_inFlight;
            VkFence             fence_swapChainImgReady;
            GPUCmdBufferHandle  commandBuffer_GFX;

            bool                canAccept_GFXJob;
            bool                bSwapchainRecreated;

        };
    } //namespace gpu
} //namespace gos
#endif //_gosGPUMainLoop_h_