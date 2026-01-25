#ifndef _GOSGPUStageHelper_h_
#define _GOSGPUStageHelper_h_
#include "pipe2/gosGPUPipe2_cmdBufferWriter.h"
#include "utils/gosGPUMainLoop.h"

namespace gos
{
    namespace gpu
    {
		/***************************************************
		 * @brief	StageHelper
		 * 			Classe di comodo per lo stagin delle risorse.
		 * 			Mantiene uno stagine-buffer e un cmdBuffer-
		 * 			Alla chiamata di submit(), le operazioni vengono accodate immediatamente a GPU e
		 * 			la fn rimane in blocco fino a che GPU non ha finito
		 */
		class StageHelper
		{
		public:
					StageHelper();
					~StageHelper()													{ unsetup (); };

            void	setup (GPU *gpuIN, u32 sizeof_stagingBuffer);
            void	unsetup ();

            StageHelper& 	begin();

            StageHelper& 	imageTransition (const VkImage &image, const eImageLayout currentLayout, const eImageLayout newLayout);
            StageHelper& 	imageTransition (const GPURenderTargetHandle &rtHandle, const eImageLayout currentLayout, const eImageLayout newLayout);
            StageHelper& 	imageTransition (const GPUZBufferHandle &zbHandle, const eImageLayout currentLayout, const eImageLayout newLayout);
            
            StageHelper& 	copyImageToImage (const VkImage &source, const VkImage &destination, const VkExtent2D &srcSize, const VkExtent2D &dstSize);
            StageHelper& 	copyImageToImage (const GPURenderTargetHandle &rtHandle, const VkImage &destination, const VkExtent2D &srcSize, const VkExtent2D &dstSize);
            StageHelper& 	copyImageToImage (const GPURenderTargetHandle &rtSRC, const GPURenderTargetHandle &rtDST, const VkExtent2D &srcSize, const VkExtent2D &dstSize);

			
			StageHelper&	mem_to_buffer (const void *src, u32 sizeof_src, GPUVtxBufferHandle dstBufferHandle, u32 offsetDST);
			StageHelper&	mem_to_buffer (const void *src, u32 sizeof_src, GPUIdxBufferHandle dstBufferHandle, u32 offsetDST);

			StageHelper&	mem_to_stgBuffer (const void *src, u32 sizeof_src, u32 *out_CAN_BE_NULL_stgBuffer_offset);

            void			submit();

			GPUCmdBufferHandle	get_cmdBuffer_handle() const 	{ return handle_cmdBuffer; }
			GPUStgBufferHandle	get_stagBuffer_handle() const 	{ return handle_stgBuffer; }

		private:
			void    priv_mem_to_stgBuffer (const void *src, u32 sizeof_src, u32 stgBuffer_offset);
			void 	priv_stgBuffer_to_buffer (u32 stgBuffer_offset, GPUVtxBufferHandle dstBufferHandle, u32 offsetDST, u32 howManyByteToCopy);
			void 	priv_stgBuffer_to_buffer (u32 stgBuffer_offset, GPUIdxBufferHandle dstBufferHandle, u32 offsetDST, u32 howManyByteToCopy);

		private:
			GPU					*gpu;
			CmdBufferWriter2	cw;
			GPUCmdBufferHandle	handle_cmdBuffer;
			GPUStgBufferHandle	handle_stgBuffer;
			TransferJob			job;
			u32 				sizeof_stagingBuffer;
			u32 				ct;
		};
	} //namespace gpu
} //namespace gos

#endif //_GOSGPUStageHelper_h_