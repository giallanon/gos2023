#ifndef _gosGPUCmdBufferWriter_h_
#define _gosGPUCmdBufferWriter_h_
#include "gosGPUEnumAndDefine.h"
#include "../gos/gosBit.h"
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
            CmdBufferWriter&    bindDescriptorSet (const GPUDescrSetInstanceHandle handle, u8 set, u32 dynamicOffset = u32MAX);

            CmdBufferWriter&    setClearColor (u8 colorAttachmentIndex, const gos::ColorHDR &color);
            CmdBufferWriter&    setDepthBufferColor (f32 depth, u32 stencil);
            CmdBufferWriter&    renderPass_begin (const GPURenderLayoutHandle renderLayoutHandle, const GPUFrameBufferHandle frameBufferHandle);
            CmdBufferWriter&    bindVtxBuffer (const GPUVtxBufferHandle handle);
            CmdBufferWriter&    bindVtxBuffers (const GPUVtxBufferHandle handleStream0, const GPUVtxBufferHandle handleStream1);
            CmdBufferWriter&    bindIdxBufferU16 (const GPUIdxBufferHandle handle);
            CmdBufferWriter&    pushConstant (u8 whichOne, const void *data, u32 sizeof_data);
            CmdBufferWriter&    pushDescriptor_begin (u8 set);
            CmdBufferWriter&    pushDescriptor_UBO (const GPUUniformBufferHandle &handle, u8 binding);
            CmdBufferWriter&    pushDescriptor_end ();
            
            CmdBufferWriter&    drawIndexed (u32 indexCount, u32 instanceCount, u32 firstIndex, u32 vertexOffset, u32 firstInstance);
            CmdBufferWriter&    draw (u32 vtxCount, u32 instanceCount, u32 firstVtx, u32 firstInstance);
            CmdBufferWriter&    renderPass_end();

            CmdBufferWriter&    imageTransition (const VkImage &image, const eImageLayout currentLayout, const eImageLayout newLayout);
            CmdBufferWriter&    copyImageToImage (const VkImage &source, const VkImage &destination, const VkExtent2D &srcSize, const VkExtent2D &dstSize);

            bool                end();    

            bool                anyError() const                            { return flag.isBitSet (FLAG__ANY_ERROR); }

        private:
            static constexpr u8    FLAG__ANY_ERROR          = 0;
            static constexpr u8    FLAG__RENDER_PASS_BEGIN  = 1;
            static constexpr u8    FLAG__PIPELINE_IS_BOUND  = 2;
            static constexpr u8    FLAG__PUSH_DESCRIPTOR_BEGIN = 3;

        private:
            struct sWriteDescr
            {
                u8  num;
                u8  set;
                VkWriteDescriptorSet descr[GOSGPU__NUM_MAX_WRITE_DESCRIPTORS_PER_CMDBUFFER];
                VkDescriptorBufferInfo bufferInfo[GOSGPU__NUM_MAX_WRITE_DESCRIPTORS_PER_CMDBUFFER];
            };

        private:
            void                priv_setError()                             { flag.set(FLAG__ANY_ERROR); }

        private:
            GPU                 *gpu;
            Flag32              flag;
            VkCommandBuffer     vkCommandBuffer;
            VkClearValue        clearColorList[GOSGPU__NUM_MAX_ATTACHMENT];
            sWriteDescr         writeDescr;
            f32                 depthClearColor;
            u32                 stencilClearColor;
            const gpu::sPipeline      *curPipeline;
            


        }; //class CmdBufferWriter

    } //namespace gpu
} //namespace gos


#endif //_gosGPUCmdBufferWriter_h_