#ifndef _gosEngineEnumAndDefine_h_
#define _gosEngineEnumAndDefine_h_
#include "../gos/gosFastArray.h"
#include "../gosGPU/gosGPU.h"
#include "../gosInput/gosInput.h"
#include "../gosAsset2/gosAsset2EnumAndDefine.h"
#include "../gosAsset2/gosAsset2Hub.h"
#include "../gosShape/gosShape.h"
#include "../gosShape/skeleton/gosSkeleton.h"
#include "../gosGeom/gosGeomCamera3.h"


namespace gos
{
	//A per "num max di handle", B per "num di chunk", C per "counter"
	GOS_DECL_HANDLE(10,7,14, ENGVtxBuffer);			//2^10=1024 => num totale di oggetti, divisi in chunk da 2^7=128
	GOS_DECL_HANDLE(10,7,14, ENGIdxBuffer);			//2^10=1024 => num totale di oggetti, divisi in chunk da 2^7=128
	GOS_DECL_HANDLE(16,10,6, ENGTexture);			//2^16=65536 => num totale di oggetti, divisi in chunk da 2^10=1024
	GOS_DECL_HANDLE(16,12,4, ENGShape);				//2^16=65536 => num totale di oggetti, divisi in chunk da 2^12=4096
	GOS_DECL_HANDLE(16,12,4, ENGGPUShape);			//2^16=65536 => num totale di oggetti, divisi in chunk da 2^12=4096
	GOS_DECL_HANDLE(10,7,14, ENGPipeline);			//2^10=1024 => num totale di oggetti, divisi in chunk da 2^7=128
	GOS_DECL_HANDLE(10,8,14, ENGVtxShader);			//2^10=1024 => num totale di oggetti, divisi in chunk da 2^8=256
	GOS_DECL_HANDLE(10,8,14, ENGPxlShader);			//2^10=1024 => num totale di oggetti, divisi in chunk da 2^8=256ù
	GOS_DECL_HANDLE(10,8,14, ENGSkeleton);			//2^10=1024 => num totale di oggetti, divisi in chunk da 2^8=256ù
	

	namespace engine
	{
		enum eLoadMode
		{
			asap = 0,
			onDemand = 1
		};

		enum class eResStatus : u8
		{
			ready		= 0,
			notLoaded	= 1,		//esiste nell'engine ma non e' stata ancora caricata
			loading		= 2,		//esiste nell'engine e' ed in fase di caricamente
			unloading	= 3,		//esiste nell'engine ma la risorsa sta per essere deallocata
			error 		= 0xff		//errore fatale. Esiste nell'engine ma probabilmente il loader non e' riuscito a caricarla, questo asset e' spacciato per sempre
		};		

		struct BaseResHandle
		{
		public:
			void reset()			{ refCount = 0; uid.setInvalid(); status=eResStatus::error; }
			bool isReady() const	{ return status==eResStatus::ready; }
			bool isError() const	{ return status==eResStatus::error; }

		public:
			asset2::UID			uid;			//se invalido, vuol dire che la risorsa e' stata creata 'a mano' e non e' un asset presente su disco
			eResStatus			status;
			u8					_pad0;
			u8					_pad1;
			u8					_pad2;
			i32					refCount;
		};

		struct ResVtxBuffer
		{
		public:
			void reset()		{ brh.reset(); vbHandle.setInvalid(); }

		public:
			BaseResHandle		brh;
			GPUVtxBufferHandle	vbHandle;
		};

		struct ResIdxBuffer
		{
		public:
			void reset()		{ brh.reset(); ibHandle.setInvalid(); }

		public:
			BaseResHandle		brh;
			GPUIdxBufferHandle	ibHandle;
		};

		struct ResGPUShape
		{
		public:
			void reset()		{ brh.reset(); handle_shape.setInvalid(); vbHandle.setInvalid(); ibHandle.setInvalid(); numIndices=numVertex=0; alloc_vtxbuf_offset=alloc_vtxbuf_size=alloc_idxbuf_offset=alloc_idxbuf_size=0; }

		public:
			BaseResHandle		brh;

			ENGShape			handle_shape;	//se valid, indica la ENGShape dalla quale this e' stata creata
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
		};



		struct ResShape
		{
		public:
			struct Data
			{
				void 	reset()													{ shape.reset(); }
				void 	destroy (gos::Allocator *allocator, gos::GPU *gpu)		{ shape::shapeFree (allocator, &shape); reset(); }

				gos::Shape	shape;
			};

			struct DataForLoaderThread
			{
				u32		handle_asU32;
				Data	data;
			};

		public:
			void reset()		{ brh.reset(); data.reset(); }

		public:
			BaseResHandle	brh;
			Data			data;
		};



		struct ResTexture
		{
		public:
			struct Data
			{
				void 	reset()													{ texHandle.setInvalid(); }
				void 	destroy (gos::Allocator *allocator, gos::GPU *gpu)		{ gpu->deleteResource (texHandle); reset(); }

				GPUTextureHandle	texHandle;
			};

			struct DataForLoaderThread
			{
				u32		handle_asU32;
				Data	data;
			};			

		public:
			void reset()		{ brh.reset(); data.reset(); }

		public:
			BaseResHandle		brh;
			Data				data;
		};		



		struct ResPipeline
		{
		public:
			struct Data
			{
				GPUPipelineHandle	pipeHandle;
				
				void 	reset()													{ pipeHandle.setInvalid(); }
				void 	destroy (gos::Allocator *allocator, gos::GPU *gpu)		{ gpu->deleteResource (pipeHandle); reset(); }
			};

			struct DataForLoaderThread
			{
				u32		handle_asU32;
				Data	data;
			};

		public:
			void reset()		{ brh.reset(); data.reset(); }

		public:
			BaseResHandle		brh;
			Data				data;
		};



		struct ResShader
		{
		public:
			struct Data
			{
				GPUShaderHandle	shaderHandle;
				
				void 	reset()													{ shaderHandle.setInvalid(); }
				void 	destroy (gos::Allocator *allocator, gos::GPU *gpu)		{ gpu->deleteResource (shaderHandle); reset(); }
			};

			struct DataForLoaderThread
			{
				u32		handle_asU32;
				Data	data;
			};

		public:
			void reset()		{ brh.reset(); data.reset(); }

		public:
			BaseResHandle		brh;
			Data				data;
		};		


		struct ResSkeleton
		{
		public:
			struct Data
			{
				void 	reset()													{ skeleton = NULL; }
				void 	destroy (gos::Allocator *allocator, gos::GPU *gpu)		{ GOSDELETE(allocator, skeleton); reset(); }

				gos::Skeleton	*skeleton;
			};

			struct DataForLoaderThread
			{
				u32		handle_asU32;
				Data	data;
			};

		public:
			void reset()		{ brh.reset(); data.reset(); }

		public:
			BaseResHandle	brh;
			Data			data;
		};


		const char*		enumToString (engine::eLoadMode s);

	} //namespace engine

} //namespace gos


#endif //_gosEngineEnumAndDefine_h_

