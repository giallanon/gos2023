#ifndef _gosMemory_h_
#define _gosMemory_h_
#include "../gosEnumAndDefine.h"
#include "gosAllocator.h"


namespace gos
{
    namespace mem
    {
		bool 		priv_init (const gos::sGOSInit &init);
		void 		priv_deinit ();

        constexpr inline bool isPointerAligned (const void *pointer, u32 alignPowerOfTwo)   { assert(GOS_IS_POWER_OF_TWO(alignPowerOfTwo)); return (((uintptr_t)(pointer)) % (alignPowerOfTwo) == 0); }

        u32     	calcAlignAdjustment (const void *mem, u32 alignmentPowerOfTwo);
        u32     	calcAlignAdjustmentWithHeader (const void *mem, u32 alignmentPowerOfTwo, u32 headerSize);

		Allocator*	getSysHeapAllocator();
		Allocator*	getScrapAllocator();

    } //namespace mem    
} //namespace gos


/***********************************************************************
 * Macro per la gestione delle allocazioni dinamiche
 *
 */
#ifdef _DEBUG
		#define GOSNEW(allocator, T)								new ( (allocator)->alloc (sizeof(T), ALIGNOF(T), __FILE__, __LINE__, true)) T

		#define GOSALLOCSTRUCT(allocator,T)							(T*)(allocator)->alloc (sizeof(T), ALIGNOF(T), __FILE__, __LINE__, false)

		#define GOSALIGNEDALLOC(allocator,sizeInByte,align)			(allocator)->alloc (sizeInByte, align, __FILE__, __LINE__, false)
		#define GOSALLOC(allocator,sizeInByte)						(allocator)->alloc (sizeInByte, ALIGNOF(void*), __FILE__, __LINE__, false)
		#define GOSALLOCT(retType, allocator,sizeInByte)			static_cast<retType>((allocator)->alloc (sizeInByte, ALIGNOF(retType), __FILE__, __LINE__, false))

		template <class T>
void	GOSDELETE(gos::Allocator *allocator, T* &p)			        { if ((p)) { (p)->~T(); allocator->dealloc((p), true); (p)=NULL; } }

		#define GOSFREE(allocator, p)								{ if((p)) { (allocator)->dealloc ((p), false); } }


		#define GOSALLOC_SCRAPT(retType, sizeInByte)				static_cast<retType>(gos::mem::getScrapAllocator()->alloc (sizeInByte, ALIGNOF(retType), __FILE__, __LINE__, false))
		#define GOSFREE_SCRAP(p)									{ if((p)) { gos::mem::getScrapAllocator()->dealloc ((p), false); } }

#else
		#define GOSNEW(allocator, T)								new ( (allocator)->alloc (sizeof(T), ALIGNOF(T))) T

		#define GOSALLOCSTRUCT(allocator,T)							(T*)(allocator)->alloc (sizeof(T), ALIGNOF(T))

		#define GOSALIGNEDALLOC(allocator,sizeInByte,align)			(allocator)->alloc (sizeInByte, align)
		#define GOSALLOC(allocator,sizeInByte)						(allocator)->alloc (sizeInByte, ALIGNOF(void*))
		#define GOSALLOCT(retType, allocator,sizeInByte)			static_cast<retType>((allocator)->alloc (sizeInByte, ALIGNOF(retType)))

		template <class T>
void	GOSDELETE(gos::Allocator *allocator, T* &p)			        { if ((p)) { (p)->~T(); allocator->dealloc((p)); (p)=NULL; } }

		#define GOSFREE(allocator, p)								{ if((p)) { (allocator)->dealloc ((p)); } }

		#define GOSALLOC_SCRAPT(retType, sizeInByte)				static_cast<retType>(gos::mem::getScrapAllocator()->alloc (sizeInByte, ALIGNOF(retType)))
		#define GOSFREE_SCRAP(p)									{ if((p)) { gos::mem::getScrapAllocator()->dealloc ((p)); } }
#endif


#endif //_gosMemory_h_