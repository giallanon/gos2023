#ifndef _gosGPUPipe2_cmdBufferWriter_h_
#define _gosGPUPipe2_cmdBufferWriter_h_
#include "../gosGPUEnumAndDefine.h"
#include "../vulkan/gosGPUVulkanEnumAndDefine.h"
#include "../vulkan/gosGPUVulkan.h"
#include "../../gos/gosBit.h"
#include "../../gos/gosUtils.h"
#include "../gosGPUResPipeline2.h"

namespace gos
{
    class GPU; //fwd decl

    namespace gpu
    {
        /*****************************
         * @brief   RenderCtx
         * 
         *          interfaccia principale per settare le pipeline 
         *          e invocare i cmd di rendering
         */
        class RenderCtx
        {
        public:
            /*****************************
             * @brief   Props
             * 
             *          Definisce la render-area, i render-target e lo zbuffer da utilizzare 
             */            
            class Props
            {
            public:
                void    reset();
                void    withRenderArea (u32 w, u32 h);
                void    withRenderArea (GPU *gpu, const GPURenderTargetHandle &rtHandle);
                void    withRT (const VkImageView &source, eAttachmentLoadOp loadOp, eAttachmentStoreOp storeOp, const gos::ColorHDR &color);
                void    withRT (GPU *gpu, const GPURenderTargetHandle &rtHandle, eAttachmentLoadOp loadOp, eAttachmentStoreOp storeOp, const gos::ColorHDR &color);
                void    withZB (GPU *gpu, const GPUZBufferHandle &zbHandle, eAttachmentLoadOp loadOp, eAttachmentStoreOp storeOp, f32 clearValue_depth, u32 clearValue_stencil);

            public:
                u8                          numColorAttachments;
                VkRenderingAttachmentInfo   colorAttachList[GOSGPU__NUM_MAX_ATTACHMENT];
                u8                          haveZB;
                VkRenderingAttachmentInfo   zBufferAttach;
                u32                         renderAreaW;
                u32                         renderAreaH;                
            };            

        public:
                        RenderCtx ();
                        ~RenderCtx();

            RenderCtx&  bindPipeline (const GPUPipelineHandle handle);
            RenderCtx&  bindDescriptorSet (const GPUDescrSetInstanceHandle handle, u8 set, u32 dynamicOffset = u32MAX);
            
                            //setDepthTestEnable/setDepthWriteEnable
                            //valido solo se la pipeline e' stata creata con VK_DYNAMIC_STATE_DEPTH_TEST_ENABLE/VK_DYNAMIC_STATE_DEPTH_WRITE_ENABLE 
            RenderCtx&  setDepthTestEnable (bool b);
            RenderCtx&  setDepthWriteEnable (bool b);

            RenderCtx&  bindVtxIdxBuffer (const GPUVtxBufferHandle vtxbuffer_handle, u32 vtxbuffer_offset, const GPUIdxBufferHandle idxbuffer_handle, u32 idxbuffer_offset);
            RenderCtx&  bindVtxBuffer (const GPUVtxBufferHandle handle, u32 offset = 0);
            RenderCtx&  bindVtxBuffers (const GPUVtxBufferHandle handleStream0, const GPUVtxBufferHandle handleStream1);
            RenderCtx&  bindIdxBufferU16 (const GPUIdxBufferHandle handle, u32 offset = 0);
            
            RenderCtx&  pushConstant (u8 whichOne, const void *data, u32 sizeof_data);
            RenderCtx&  drawIndexed (u32 indexCount, u32 instanceCount, u32 firstIndex, u32 vertexOffset, u32 firstInstance);
            RenderCtx&  draw (u32 vtxCount, u32 instanceCount, u32 firstVtx, u32 firstInstance);

            bool        end_render_ctx();

            bool        anyError() const                                                    { return flag.isBitSet (FLAG__ANY_ERROR); }

        public:
            bool        internal__reset  (GPU *gpuIN, VkCommandBuffer vkCommandBufferIN, const RenderCtx::Props &propsIN);

        private:
            void        priv_setError ();
            void        priv_flushPushConst();

        private:
            static constexpr u8         FLAG__ANY_ERROR                         = 0;
            static constexpr u8         FLAG__BEGIN_RENDER_HAS_BEEN_CALLED      = 1;

        private:
            GPU                         *gpu;
            VkCommandBuffer             vkCommandBuffer;
            Flag32                      flag;
            Props                       props;

            u8                          pushConstBuffer[128];
            const gpu::Pipeline2        *cache_curPipeline;
            GPUVtxBufferHandle          cache_vxtBuffer_handle[2];
            u32                         cache_vxtBuffer_offset[2];
            GPUIdxBufferHandle          cache_idxBuffer_handle;
            u32                         cache_idxBuffer_offset;
        };

        /******************************
         * @brief   CmdBufferWriter2
         * 
         * 
         */
        class CmdBufferWriter2
        {
        public:

            /*****************************
             * @brief   SetupRenderCtx
             * 
             *          classe di comodo raccogliere per definire i parametri che definiscono un
             *          RenderCtx
             */
            class SetupRenderCtx
            {
            public:
                SetupRenderCtx&     withRenderArea (u32 w, u32 h);
                SetupRenderCtx&     withRenderArea (const GPURenderTargetHandle &rtHandle);
                SetupRenderCtx&     withRT (const VkImageView &source, eAttachmentLoadOp loadOp, eAttachmentStoreOp storeOp, const gos::ColorHDR &color = ColorHDR(0,0,0));
                SetupRenderCtx&     withRT (const GPURenderTargetHandle &rtHandle, eAttachmentLoadOp loadOp, eAttachmentStoreOp storeOp, const gos::ColorHDR &color = ColorHDR(0,0,0));
                SetupRenderCtx&     withZB (const GPUZBufferHandle &zbHandle, eAttachmentLoadOp loadOp, eAttachmentStoreOp storeOp, f32 clearValue_depth=1.0f, u32 clearValue_stencil=0);
                RenderCtx&          define_end();

            private:
                                    SetupRenderCtx()    { }
                                    ~SetupRenderCtx()   { }
                void                reset(GPU *gpuIN, CmdBufferWriter2 *cmdBufferWriterIN, VkCommandBuffer vkCommandBufferIN, RenderCtx *out_renderCtx);

            private:
                GPU                 *gpu;
                CmdBufferWriter2    *cmdBufferWriter;
                VkCommandBuffer     vkCommandBuffer;
                RenderCtx           *out_renderCtx;
                RenderCtx::Props     props;

            friend CmdBufferWriter2;
            };


        public:
                                CmdBufferWriter2();
                                ~CmdBufferWriter2()          { }

            CmdBufferWriter2&   begin (GPU *gpuIN, const GPUCmdBufferHandle handle);
            CmdBufferWriter2&   setViewport (const GPUViewportHandle handle);

            CmdBufferWriter2&   imageTransition (const VkImage &image, const eImageLayout currentLayout, const eImageLayout newLayout);
            CmdBufferWriter2&   imageTransition (const GPURenderTargetHandle &rtHandle, const eImageLayout currentLayout, const eImageLayout newLayout);
            CmdBufferWriter2&   imageTransition (const GPUZBufferHandle &zbHandle, const eImageLayout currentLayout, const eImageLayout newLayout);
            
            CmdBufferWriter2&   copyImageToImage (const VkImage &source, const VkImage &destination, const VkExtent2D &srcSize, const VkExtent2D &dstSize);
            CmdBufferWriter2&   copyImageToImage (const GPURenderTargetHandle &rtHandle, const VkImage &destination, const VkExtent2D &srcSize, const VkExtent2D &dstSize);
            CmdBufferWriter2&   copyImageToImage (const GPURenderTargetHandle &rtSRC, const GPURenderTargetHandle &rtDST, const VkExtent2D &srcSize, const VkExtent2D &dstSize);

            CmdBufferWriter2&   copyBuffer (const VkBuffer srcBuffer, const VkBuffer dstBuffer, u32 offsetSRC, u32 offsetDST, u32 howManyByteToCopy);
            CmdBufferWriter2&   copyBuffer (GPUStgBufferHandle srcStageBufferHandle, GPUVtxBufferHandle dstVtxBufferHandle, u32 offsetSRC, u32 offsetDST, u32 howManyByteToCopy);
            CmdBufferWriter2&   copyBuffer (GPUStgBufferHandle srcStageBufferHandle, GPUIdxBufferHandle dstIdxBufferHandle, u32 offsetSRC, u32 offsetDST, u32 howManyByteToCopy);

			CmdBufferWriter2&   pipelineBarrier();


            SetupRenderCtx&     renderCtx_define_begin (RenderCtx *out_renderCtx);
            bool                end();    

            bool                anyError() const                            { return flag.isBitSet (FLAG__ANY_ERROR); }
            
        private:
            static constexpr u8    FLAG__ANY_ERROR              = 0;

        private:
            void                priv_setError()                             { flag.set(FLAG__ANY_ERROR); }

        private:
            GPU                 *gpu;
            Flag32              flag;
            VkCommandBuffer     vkCommandBuffer;
            SetupRenderCtx      setupRenderCtx;
        }; //class CmdBufferWriter


    } //namespace gpu
} //namespace gos

#endif //#define _gosGPUPipe2_cmdBufferWriter_h_

