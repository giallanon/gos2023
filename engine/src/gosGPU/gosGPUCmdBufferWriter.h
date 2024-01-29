#ifndef _gosGPUCmdBufferWriter_h_
#define _gosGPUCmdBufferWriter_h_
#include "gosGPUEnumAndDefine.h"
#include "../gos/gosUtils.h"


namespace gos
{
    class GPU; //fwd decl


    namespace gpu
    {
        /**********************************************
         * CmdBufferWriter
         * 
         * classe di comodo per fillare i CmdBuffer
         * 
        */
        class CmdBufferWriter
        {
        public:
                                CmdBufferWriter();

            CmdBufferWriter&    begin (GPU *gpuIN, const GPUCmdBufferHandle handle);
            CmdBufferWriter&    setViewport (const GPUViewportHandle handle);
            CmdBufferWriter&    bindPipeline (const GPUPipelineHandle handle);
            CmdBufferWriter&    bindDescriptorSet (const GPUDescrSetInstancerHandle handle);

            CmdBufferWriter&    setClearColor (u8 colorAttachmentIndex, const gos::ColorHDR &color);
            CmdBufferWriter&    setDepthBufferColor (f32 depth, u32 stencil);
            CmdBufferWriter&    renderPass_begin (const GPURenderLayoutHandle renderLayoutHandle, const GPUFrameBufferHandle frameBufferHandle);
            CmdBufferWriter&    bindVtxBuffer (const GPUVtxBufferHandle handle);
            CmdBufferWriter&    bindIdxBufferU16 (const GPUIdxBufferHandle handle);
            CmdBufferWriter&    drawIndexed (u32 indexCount, u32 instanceCount, u32 firstIndex, u32 vertexOffset, u32 firstInstance);
            CmdBufferWriter&    renderPass_end();

            bool                end();    

            bool                anyError() const                            { return gos::utils::isBitSET (&flag, FLAG__ANY_ERROR); }

        private:
            static constexpr u8    FLAG__ANY_ERROR          = 0;
            static constexpr u8    FLAG__RENDER_PASS_BEGIN  = 1;
            static constexpr u8    FLAG__PIPELINE_IS_BOUND  = 2;

        private:
            void                priv_setError()                             { gos::utils::bitSET (&flag, FLAG__ANY_ERROR); }

        private:
            GPU                 *gpu;
            u32                 flag;
            VkCommandBuffer     vkCommandBuffer;
            VkPipeline          vkPipelineHandle;
            VkPipelineLayout    vkPipelineLayoutHandle;
            VkClearValue        clearColorList[GOSGPU__NUM_MAX_ATTACHMENT];
            f32                 depthClearColor;
            u32                 stencilClearColor;


        }; //class CmdBufferWriter

    } //namespace gpu
} //namespace gos


#endif //_gosGPUCmdBufferWriter_h_