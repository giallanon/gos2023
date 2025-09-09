#ifndef _gosGPUMainLoop_h_
#define _gosGPUMainLoop_h_
#include "../gosGPUEnumAndDefine.h"
#include "../vulkan/gosGPUVulkanEnumAndDefine.h"
#include "../../gos/dataTypes/gosTimer.h"
#include "../../gos/gosFIFOFixedSize.h"


namespace gos
{
    class GPU;  //fwd decl

    namespace gpu
    {
        struct AcquiredSwapchainImg
        {
            VkImage image;
            u32     index;
        };        

        /********************************
         * @brief   AquireSwapChainImage
         *          Classe dedicata all'acquisizione di una swapchain-image.
         *          Chiamare tryAcquire() ripetutamente nel main loop.
         *          Quanto questa ritorna true allora this->acquiredImg e' valida e 
         *          puo' essere utilizzata immediatamente.
         */
        class AquireSwapChainImage
        {
        public:
                    AquireSwapChainImage()          { gpu = NULL; stato = eStato::idle; acquiredImg.index=u32MAX; }
                    ~AquireSwapChainImage()         { unsetup(); }

            void    setup (gos::GPU *gpuIN);
            void    unsetup();

            //se ritorna true, allora this->imageIndex e this->image sono validi e utilizzabili immediatamente
            bool    tryAcquire ();

        public:
            GPU                     *gpu;
            AcquiredSwapchainImg    acquiredImg;
            gos::TimerFPS           timerFPS;

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
            VkFence     fence;
            eStato      stato;
        };

        /********************************
         * @brief   PresentGFXJob
         *          Submitta una job grafico alla GPU e si preoccupa di presentarlo appena possibile.
         *          submit() pretende che <swapChainImageIndex> sia un valido indice ad una immagine di swapchain
         *          precedentemente acquisita (per esempio da AquireSwapChainImage).
         * 
         *          hasFinished() ritona true se la classe non ha alcun lavoro in canna (ovvero ritorna true dopo che il
         *          job e' stato presentato, oppure se non ha alcun job da gestire).
         */        
        class PresentGFXJob
        {
        public:
            PresentGFXJob()                         { gpu = NULL; stato = eStato::idle; }
            ~PresentGFXJob()                        { unsetup(); }

            void    setup (gos::GPU *gpuIN);
            void    unsetup();

            void    submit (const GPUCmdBufferHandle &cmdBufferHandle, u32 swapChainImageIndex);
            bool    hasFinished();

        public:
            gos::TimerFPS   timerFPS;
            
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
         * MainLoop2
         * 
        */
        class MainLoop2
        {
        public:
                    MainLoop2 ()                    { printInfoFreq_msec = 1000; nextTimePrintInfo_msec=0; gfxJobFinished=true; }
                    ~MainLoop2()                    { unsetup(); }
                    
            void    setup (gos::GPU *gpuIN)         { acquire.setup(gpuIN); gfxJob.setup(gpuIN); }
            void    unsetup()                       { acquire.unsetup(); gfxJob.unsetup(); }
        
            void    run ();

            bool    gfxJob_canSubmit (AcquiredSwapchainImg *out);
            void    gfxJob_submitAndPresent (const GPUCmdBufferHandle &cmdBufferHandle, const AcquiredSwapchainImg &info);


            void    stat_onCPUFrameBegin()                                      { cpuTimerFPS.onFrameBegin(); }
            void    stat_onCPUFrameEnd()                                        { cpuTimerFPS.onFrameEnd(); }
            void    stat_setPrintReportEvery (u32 msec)                         { printInfoFreq_msec = msec; }

        private:
            AquireSwapChainImage    acquire;
            PresentGFXJob           gfxJob;
            u64                     nextTimePrintInfo_msec;
            u64                     printInfoFreq_msec;
            gos::FIFOFixedSize<AcquiredSwapchainImg,4>  acquiredList;
            gos::TimerFPS           cpuTimerFPS;
            bool                    gfxJobFinished;
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