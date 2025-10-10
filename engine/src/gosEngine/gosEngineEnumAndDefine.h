#ifndef _gosEngineEnumAndDefine_h_
#define _gosEngineEnumAndDefine_h_
#include "../gosGPU/gosGPU.h"
#include "../gosInput/gosInput.h"
#include "../gosAsset/gosAssetHub.h"
#include "../gosShape/gosShape.h"

namespace gos
{
	//A per "num max di handle", B per "num di chunk", C per "counter"
	GOS_DECL_HANDLE(10,7,14, ENGVtxBuffer);			//2^10=1024 => num totale di oggetti, divisi in chunk da 2^7=128
	GOS_DECL_HANDLE(10,7,14, ENGIdxBuffer);			//2^10=1024 => num totale di oggetti, divisi in chunk da 2^7=128
	GOS_DECL_HANDLE(16,10,6, ENGTexture);			//2^16=65536 => num totale di oggetti, divisi in chunk da 2^10=1024
	GOS_DECL_HANDLE(16,12,4, ENGShape);				//2^16=65536 => num totale di oggetti, divisi in chunk da 2^12=4096

	namespace engine
	{
		struct VtxBuffer
		{
			i32					refCount;
			GPUVtxBufferHandle	vbHandle;

			void reset()		{ refCount = 0; vbHandle.setInvalid(); }
		};

		struct IdxBuffer
		{
			i32					refCount;
			GPUIdxBufferHandle	ibHandle;

			void reset()		{ refCount = 0; ibHandle.setInvalid(); }
		};

		struct Texture
		{
			i32					refCount;
			GPUTextureHandle	texHandle;

			void reset()		{ refCount = 0; texHandle.setInvalid(); }
		};

		struct Shape
		{
			i32					refCount;

			GPUVtxBufferHandle	vbHandle;
			GPUIdxBufferHandle	ibHandle;
			u32					indexStart;		//posizione del primo idx di questa shape all'interno di vbHandle
			u32					numIndices;
			u32					vtxStart;		//posizione del primo vtx di questa shape all'interno di vbHandle
			u32					numVertex;

			u32					alloc_vtxbuf_offset;
			u32					alloc_vtxbuf_size;
			u32					alloc_idxbuf_offset;
			u32					alloc_idxbuf_size;

			void reset()		{ refCount = 0; vbHandle.setInvalid(); ibHandle.setInvalid(); numIndices=numVertex=0; alloc_vtxbuf_offset=alloc_vtxbuf_size=alloc_idxbuf_offset=alloc_idxbuf_size=0; }
		};




	} //namespace engine

} //namespace gos


#endif //_gosEngineEnumAndDefine_h_

