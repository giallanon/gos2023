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
	GOS_DECL_HANDLE(1024, 128, ENGVtxBuffer);			//2^10=1024 => num totale di oggetti, divisi in chunk da 2^7=128
	GOS_DECL_HANDLE(1024, 128, ENGIdxBuffer);			//2^10=1024 => num totale di oggetti, divisi in chunk da 2^7=128
	GOS_DECL_HANDLE(65536, 1024, ENGTexture);			//2^16=65536 => num totale di oggetti, divisi in chunk da 2^10=1024
	GOS_DECL_HANDLE(65536, 4096, ENGShape);				//2^16=65536 => num totale di oggetti, divisi in chunk da 2^12=4096
	GOS_DECL_HANDLE(65536, 4096, ENGGPUShape);			//2^16=65536 => num totale di oggetti, divisi in chunk da 2^12=4096
	GOS_DECL_HANDLE(1024, 128, ENGPipeline);			//2^10=1024 => num totale di oggetti, divisi in chunk da 2^7=128
	GOS_DECL_HANDLE(1024, 256, ENGVtxShader);			//2^10=1024 => num totale di oggetti, divisi in chunk da 2^8=256
	GOS_DECL_HANDLE(1024, 256, ENGPxlShader);			//2^10=1024 => num totale di oggetti, divisi in chunk da 2^8=256
	GOS_DECL_HANDLE(1024, 256, ENGSkeleton);			//2^10=1024 => num totale di oggetti, divisi in chunk da 2^8=256
	GOS_DECL_HANDLE(32768, 4096, ENGModel3d);			//32768 => num totale di oggetti, divisi in chunk da 2^12=4096
	GOS_DECL_HANDLE(65536, 4096, ENGModel3dInst);		//2^16=65536 => num totale di oggetti, divisi in chunk da 2^12=4096
	
	
	namespace engine
	{
		enum class eLoadMode : u8
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
		

		struct Resource; //fwd

		typedef void (*ResCallback_onSubresStateChanged)(Resource *res, const Resource *subres);
		
		struct ResHandleDepList
		{
			ResHandleDepList	*next;
			Resource		*brh;
		};


		struct Resource
		{
		public:
			void reset()			{ refCount = 0; uid.setInvalid(); status=eResStatus::error; deplist=NULL; callback_onSubresStateChanged=NULL; }
			bool isReady() const	{ return status==eResStatus::ready; }
			bool isError() const	{ return status==eResStatus::error; }

		public:
			asset2::UID			uid;			//se invalido, vuol dire che la risorsa e' stata creata 'a mano' e non e' un asset presente su disco
			eResStatus			status;			//stato della risorsa dal punto di vista dell'engine
			u8					_pad0;
			u8					_pad1;
			u8					_pad2;
			i32					refCount;
			ResHandleDepList	*deplist;		//elenco degli handle che dipendono da me (vengono notificati dei miei cambi di stato)

			ResCallback_onSubresStateChanged	*callback_onSubresStateChanged;
		};

		

		const char*		enumToString (engine::eLoadMode s);

	} //namespace engine

} //namespace gos


#endif //_gosEngineEnumAndDefine_h_

