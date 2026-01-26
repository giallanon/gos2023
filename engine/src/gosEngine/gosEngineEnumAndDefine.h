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
	GOS_DECL_HANDLE(10,8,14, ENGPxlShader);			//2^10=1024 => num totale di oggetti, divisi in chunk da 2^8=256
	GOS_DECL_HANDLE(10,8,14, ENGSkeleton);			//2^10=1024 => num totale di oggetti, divisi in chunk da 2^8=256
	GOS_DECL_HANDLE(16,12,4, ENGModel3d);			//2^16=65536 => num totale di oggetti, divisi in chunk da 2^12=4096
	
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

		

		const char*		enumToString (engine::eLoadMode s);

	} //namespace engine

} //namespace gos


#endif //_gosEngineEnumAndDefine_h_

