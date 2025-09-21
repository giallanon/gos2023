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
                    AquireSwapChainImage()          { gpu = NULL; stato = eStato::idle; acquiredImg.imageIndex=u32MAX; }
                    ~AquireSwapChainImage()         { unsetup(); }

            void    setup (gos::GPU *gpuIN);
            void    unsetup();

            //se ritorna true, allora this->imageIndex e this->image sono validi e utilizzabili immediatamente
            bool    tryAcquire ();

        public:
            GPU             *gpu;
            SwapchainImg    acquiredImg;
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

            void    submit (const GPUCmdBufferHandle &cmdBufferHandle, const gos::gpu::SwapchainImg &swapchainImg);
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

            bool    gfxJob_canSubmit (SwapchainImg *out);
            void    gfxJob_submitAndPresent (const GPUCmdBufferHandle &cmdBufferHandle, const SwapchainImg &info);


            void    stat_onCPUFrameBegin()                                      { cpuTimerFPS.onFrameBegin(); }
            void    stat_onCPUFrameEnd()                                        { cpuTimerFPS.onFrameEnd(); }
            void    stat_setPrintReportEvery (u32 msec)                         { printInfoFreq_msec = msec; }

        private:
            AquireSwapChainImage    acquire;
            PresentGFXJob           gfxJob;
            u64                     nextTimePrintInfo_msec;
            u64                     printInfoFreq_msec;
            gos::FIFOFixedSize<SwapchainImg,4>  acquiredList;
            gos::TimerFPS           cpuTimerFPS;
            bool                    gfxJobFinished;
        };


        
    } //namespace gpu
} //namespace gos
#endif //_gosGPUMainLoop_h_