#ifndef _gosGPUMappedBuffer_h_
#define _gosGPUMappedBuffer_h_
#include "gosGPUResBuffer.h"

namespace gos
{
	class GPU;  //fwd

    namespace gpu
    {
		/***************************************************
		 * @brief	MappedBufW
		 * 			Classe di comodo ritornata da gpu->begin_write()
		 * 			Serve per scrivere su un buffer precedentemente allocato
		 * 			in modalita' eMemAccessMode::shared_cpuW o eMemAccessMode::shared_cpuRW
		 */
		class MappedBufW
		{
		public:
					MappedBufW()											{ priv_reset(); }
					~MappedBufW()											{ end(); }

			void 	write (const void *src, u32 howManyBytes, u32 dst_offset);
			
					template<class TYPE>
			void 	writeT (const TYPE value, u32 dst_offset)				{ write (&value, sizeof(TYPE), dst_offset); }
			
			void	end();

		public:
			const gpu::Buffer *buffer;


		private:
			void 	priv_reset();
			bool 	bind (GPU *gpu, const gpu::Buffer *b);

		private:
			
			GPU		*gpu;
			u32 	min_offset;
			u32 	max_offset;

		friend GPU;
		};
	} //namespace gpu
} //namespace gos

#endif //_gosGPUMappedBuffer_h_