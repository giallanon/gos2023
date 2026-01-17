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
        /******************************
         * @brief   CmdBufferWriter2
         * 
         * 
         */
        class CmdBufferWriter2
        {
        public:
            class BeginRend
            {
            public:
                BeginRend&  withRenderArea (u32 w, u32 h);
                BeginRend&  withRenderArea (const GPURenderTargetHandle &rtHandle);
                BeginRend&  withRT (const VkImageView &source, eAttachmentLoadOp loadOp, eAttachmentStoreOp storeOp, const gos::ColorHDR &color = ColorHDR(0,0,0));
                BeginRend&  withRT (const GPURenderTargetHandle &rtHandle, eAttachmentLoadOp loadOp, eAttachmentStoreOp storeOp, const gos::ColorHDR &color = ColorHDR(0,0,0));
                BeginRend&  withZB (const GPUZBufferHandle &zbHandle, eAttachmentLoadOp loadOp, eAttachmentStoreOp storeOp, f32 clearValue_depth=1.0f, u32 clearValue_stencil=0);

                BeginRend&  bindPipeline (const GPUPipelineHandle handle);
                BeginRend&  bindDescriptorSet (const GPUDescrSetInstanceHandle handle, u8 set, u32 dynamicOffset = u32MAX);
                
                                //setDepthTestEnable/setDepthWriteEnable
                                //valido solo se la pipeline e' stata creata con VK_DYNAMIC_STATE_DEPTH_TEST_ENABLE/VK_DYNAMIC_STATE_DEPTH_WRITE_ENABLE 
                BeginRend&  setDepthTestEnable (bool b);
                BeginRend&  setDepthWriteEnable (bool b);

                BeginRend&  bindVtxIdxBuffer (const GPUVtxBufferHandle vtxbuffer_handle, u32 vtxbuffer_offset, const GPUIdxBufferHandle idxbuffer_handle, u32 idxbuffer_offset);
                BeginRend&  bindVtxBuffer (const GPUVtxBufferHandle handle, u32 offset = 0);
                BeginRend&  bindVtxBuffers (const GPUVtxBufferHandle handleStream0, const GPUVtxBufferHandle handleStream1);
                BeginRend&  bindIdxBufferU16 (const GPUIdxBufferHandle handle, u32 offset = 0);
                
                BeginRend&  pushConstant (u8 whichOne, const void *data, u32 sizeof_data);
                BeginRend&  drawIndexed (u32 indexCount, u32 instanceCount, u32 firstIndex, u32 vertexOffset, u32 firstInstance);
                BeginRend&  draw (u32 vtxCount, u32 instanceCount, u32 firstVtx, u32 firstInstance);
                
                CmdBufferWriter2&   endRender();

                bool                anyError() const                                                    { return flag.isBitSet (FLAG__ANY_ERROR); }
                
            private:
                                    BeginRend ()                                                        { }
                                    ~BeginRend()                                                        { }

                void                priv_setup  (GPU *gpuIN, CmdBufferWriter2 *cmdBufferWriterIN, VkCommandBuffer vkCommandBufferIN);
                void                priv_setError();
                void                priv_recordBeginRenderingIfNeeded();
                void                priv_flushPushConst();

            private:
                static constexpr u8         FLAG__ANY_ERROR                         = 0;
                static constexpr u8         FLAG__vkBeginRender_HAS_BEEN_ISSUED     = 1;

            private:
                GPU                         *gpu;
                CmdBufferWriter2            *cmdBufferWriter;
                VkCommandBuffer             vkCommandBuffer;
                Flag32                      flag;
                u8                          numColorAttachments;
                VkRenderingAttachmentInfo   colorAttachList[GOSGPU__NUM_MAX_ATTACHMENT];
                u8                          haveZB;
                VkRenderingAttachmentInfo   zBufferAttach;
                u32                         renderAreaW;
                u32                         renderAreaH;
                const gpu::Pipeline2        *curPipeline;
                u8                          pushConstBuffer[128];


                GPUVtxBufferHandle          cache_vxtBuffer_handle[2];
                u32                         cache_vxtBuffer_offset[2];
                GPUIdxBufferHandle          cache_idxBuffer_handle;
                u32                         cache_idxBuffer_offset;

            friend CmdBufferWriter2;
            };

        
            /*****************************
             * @brief   SetupRenderCtx
             * 
             *          classe di comodo raccogliere per definire i parametri che definiscono un
             *          RenderCtx
             */
            class SetupRenderCtx
            {
            public:
                SetupRenderCtx&  withRenderArea (u32 w, u32 h);
                SetupRenderCtx&  withRenderArea (const GPURenderTargetHandle &rtHandle);
                SetupRenderCtx&  withRT (const VkImageView &source, eAttachmentLoadOp loadOp, eAttachmentStoreOp storeOp, const gos::ColorHDR &color = ColorHDR(0,0,0));
                SetupRenderCtx&  withRT (const GPURenderTargetHandle &rtHandle, eAttachmentLoadOp loadOp, eAttachmentStoreOp storeOp, const gos::ColorHDR &color = ColorHDR(0,0,0));
                SetupRenderCtx&  withZB (const GPUZBufferHandle &zbHandle, eAttachmentLoadOp loadOp, eAttachmentStoreOp storeOp, f32 clearValue_depth=1.0f, u32 clearValue_stencil=0);

            private:
                                SetupRenderCtx()    { }
                                ~SetupRenderCtx()   { }
                void            reset(GPU *gpuIN);

            private:
                GPU                         *gpu;
                u8                          numColorAttachments;
                VkRenderingAttachmentInfo   colorAttachList[GOSGPU__NUM_MAX_ATTACHMENT];
                u8                          haveZB;
                VkRenderingAttachmentInfo   zBufferAttach;
                u32                         renderAreaW;
                u32                         renderAreaH;

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


            BeginRend&          beginRender();
            bool                end();    


            void    setupRenderCtx()


            bool                anyError() const                            { return flag.isBitSet (FLAG__ANY_ERROR); }
            
        private:
            static constexpr u8    FLAG__ANY_ERROR              = 0;

        private:
            void                priv_setError()                             { flag.set(FLAG__ANY_ERROR); }

        private:
            GPU                 *gpu;
            Flag32              flag;
            VkCommandBuffer     vkCommandBuffer;
            BeginRend           beginRend;
        }; //class CmdBufferWriter


    } //namespace gpu
} //namespace gos

#endif //#define _gosGPUPipe2_cmdBufferWriter_h_

