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
         * @brief   GFXJob
         *          Si interfaccia con la coda grafica di GPU.
         *          submitAndPresent()  => submitta una job grafico alla GPU e si preoccupa di presentarlo appena possibile
         *          submit()            => submitta una job grafico alla GPU senza presentarlo
         * 
         *          submitAndPresent() pretende che <swapChainImageIndex> sia un valido indice ad una immagine di swapchain
         *          precedentemente acquisita (per esempio da AquireSwapChainImage).
         * 
         *          hasFinished() ritona true se la classe non ha alcun lavoro in canna (ovvero ritorna true dopo che il
         *          job e' stato completato da GPU, oppure se non ha alcun job da gestire).
         */        
        class GFXJob
        {
        public:
                    GFXJob()                         { gpu = NULL; stato = eStato::idle; }
                    ~GFXJob()                        { unsetup(); }

            void    setup (gos::GPU *gpuIN);
            void    unsetup();

            void    submitAndPresent (const GPUCmdBufferHandle &cmdBufferHandle, const gos::gpu::SwapchainImg &swapchainImg)    { priv_submit (cmdBufferHandle, swapchainImg.imageIndex); }
            void    submit (const GPUCmdBufferHandle &cmdBufferHandle)                                                          { priv_submit (cmdBufferHandle, u32MAX); }
            bool    hasFinished();

            const gos::TimerFPS*    getTimerFPS() const                                                                         { return &timerFPS; }

            
        private:
            enum class eStato : u8
            {
                idle,
                jobInProgress
            };

        private:
            void        priv_submit (const GPUCmdBufferHandle &cmdBufferHandle, u32 swapChainImageIndex);

        private:
            gos::TimerFPS   timerFPS;
            GPU         *gpu;
            VkFence     fence;
            u32         swapChainImageIndex;
            u32         swapChainAutoID;
            eStato      stato;
        };


        /********************************
         * @brief   TransferJob
         *          Si interfaccia con la coda di "transfer" di GPU (se GPU non ne supporta, allora usa una coda gfx).
         *          submit()            => submitta un job alla GPU         * 
         *          hasFinished()       => ritorna true quando il job submitteed e' stato completato da GPU
         */        
        class TransferJob
        {
        public:
                    TransferJob()                         { gpu = NULL; stato = eStato::idle; }
                    ~TransferJob()                        { unsetup(); }

            void    setup (gos::GPU *gpuIN);
            void    unsetup();

            void    submit (const GPUCmdBufferHandle &cmdBufferHandle);
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

            void    stat_onCommandBufferBegin()                                 { cmdBufferTimerFPS.onFrameBegin(); }
            void    stat_onCommandBufferEnd()                                   { cmdBufferTimerFPS.onFrameEnd(); }

            void    stat_setPrintReportEvery (u32 msec)                         { printInfoFreq_msec = msec; }

        private:
            AquireSwapChainImage    acquire;
            GFXJob                  gfxJob;
            u64                     nextTimePrintInfo_msec;
            u64                     printInfoFreq_msec;
            gos::FIFOFixedSize<SwapchainImg,4>  acquiredList;
            gos::TimerFPS           cpuTimerFPS;
            gos::TimerFPS           cmdBufferTimerFPS;
            bool                    gfxJobFinished;
        };


        
    } //namespace gpu
} //namespace gos
#endif //_gosGPUMainLoop_h_