#ifndef _gosGPUMemMappedDynBuffer_h_
#define _gosGPUMemMappedDynBuffer_h_
#include "gosGPUEnumAndDefine.h"

namespace gos
{
	namespace gpu
	{
		/********************************************************
		 * @brief 	MemMappedDynBuffer<>
		 * 			Gli UBO/SBO, se considerati come array, devono avere gli elementi di una ben precisa dimensione,
		 * 			dipendente dalla GPU.
		 * 			Questa classe crea un array di <num_elemIN> elementi di tipo DATA assicurandosi che ogni singolo
		 * 			elemento dell'array sia grosso quanto un multiplo della dimensione minima richiesta dalla GPU
		 * 
		 * 
		 */
		template<class DATA>
		class MemMappedDynBuffer
		{
		public:
			MemMappedDynBuffer()
			{
				allocator = NULL;
				buffer = NULL;
				sizeof_oneElem = 0;
				num_max_elem = 0;
			}

			~MemMappedDynBuffer()																		{ unsetup(); }

			void	setup (gos::Allocator *allocatorIN, u32 num_elemIN, u32 minOffsetAlignment)
			{
				allocator = allocatorIN;
				num_max_elem = num_elemIN;

				sizeof_oneElem = sizeof(DATA);
				if (minOffsetAlignment > 0)
					sizeof_oneElem = (sizeof_oneElem + minOffsetAlignment - 1) & ~(minOffsetAlignment - 1);

				const u32 byteToAlloc = num_max_elem * sizeof_oneElem;
				buffer = static_cast<u8*>(GOSALIGNEDALLOC(allocator, byteToAlloc, sizeof_oneElem));
			}

			void	unsetup()
			{
				if (NULL != allocator)
				{
					GOSFREE(allocator, buffer);
					buffer = NULL;
					allocator = NULL;
				}
			}

			DATA*			getElem(u32 i)																{ assert (i < num_max_elem); return reinterpret_cast<DATA*>(&buffer[sizeof_oneElem * i]); }
			const DATA*		queryElem(u32 i) const														{ assert (i < num_max_elem); return reinterpret_cast<DATA*>(&buffer[sizeof_oneElem * i]); }

			const void*	getBuffer() const 																{ return buffer; }
			u32		getNumMaxElem() const																{ return num_max_elem; }
			u32		getRealSizeAllocated() const														{ return num_max_elem * sizeof_oneElem; }
			u32		getRealSizeOfOneElem() const														{ return sizeof_oneElem; }


			Allocator*	getAllocator() const 															{ return allocator; }

		private:
			gos::Allocator *allocator;
			u8				*buffer;
			u32				sizeof_oneElem;
			u32				num_max_elem;
		};


	} //namespace gpu

} //namespace gos

#endif //_gosGPUMemMappedDynBuffer_h_

