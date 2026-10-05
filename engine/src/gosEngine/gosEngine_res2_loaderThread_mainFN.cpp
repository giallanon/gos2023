#include "gosEngine.h"
#include "res2/loaders/Loader_tex2D.h"

using namespace gos;
using namespace gos::engine;

typedef gos::AllocatorHeap<gos::AllocPolicy_Track_simple, gos::AllocPolicy_Thread_Unsafe>		res2__GOSENGINELoaderMemAllocatorTS;


//***************************************
i16	Engine::res2__LoaderThread_mainFN (void *paramsIN)
{
    res2__GOSENGINELoaderMemAllocatorTS *localAllocator = GOSNEW(gos::getSysHeapAllocator(), res2__GOSENGINELoaderMemAllocatorTS)("res2__ENGLoader");
    localAllocator->setup (1024 * 1024 * 128); //128MB
    
    //copia dei params
	res2::LoaderInfo	loaderInfo;
    HThreadMsgR         		msgqR;
    HThreadMsgW         		msgqW;
    {
        const sLoaderThreadInitParams *params = reinterpret_cast<const sLoaderThreadInitParams*>(paramsIN);
        msgqR = params->msgqR;
        msgqW = params->msgqW;
		loaderInfo.thread_allocator = localAllocator;
		loaderInfo.engine_allocator = params->engine_allocator;
		loaderInfo.engine = params->engine;
        loaderInfo.logger = params->logger;
		loaderInfo.gpu = params->gpu;
		loaderInfo.ctx = params->ctx;		

        //segnalo che sono partito
        thread::signal_fire (params->hEvent_started);
    }

    //spawn dei loader per le risorse
    static constexpr u32 NUM_MAX_RES_LOADER = 1 + (u32)res::eType::NUM_MAX;
    res2::BaseLoader  *res_loaderList[NUM_MAX_RES_LOADER];
    {
        memset (res_loaderList, 0, sizeof(res_loaderList));
        res_loaderList[(u32)res2::eResType::texture_2d] = GOSNEW(localAllocator, res2::Loader_tex2D)();
    }	

    //spawn dei loader per gli asset
    static constexpr u32 NUM_MAX_ASSET_LOADER = 1 + (u32)res::eType::NUM_MAX;
    res2::BaseLoader  *asset_loaderList[NUM_MAX_ASSET_LOADER];
    {
        memset (asset_loaderList, 0, sizeof(asset_loaderList));
    }	



    //loop
    static constexpr u8 NUM_MAX_MESSAGES = 64;
    thread::sMsg msgList[NUM_MAX_MESSAGES];
    bool bQuit = false;
    while (bQuit == false)
    {
		if (!thread::waitForAnEvent (msgqR, u32MAX))
			break;


        u32 nMsg;
        while (0 != (nMsg = thread::popMultipleMsg(msgqR, msgList, NUM_MAX_MESSAGES)))
        {
            for (u32 i=0; i<nMsg; i++)
            {
                switch (msgList[i].what)
                {
                default:
                    DBGBREAK;
                    break;

                case RES2__MSG_FOR_LOADER_THREAD__DIE:
                    bQuit = true;
                    break;

				case RES2__MSG_FOR_LOADER_THREAD__RESOURCE_LOAD:
					//richiesta di caricare una risorsa
					{
						res2::ResDescr *descr = (res2::ResDescr*)msgList[i].buffer;
						assert (descr->signatureUID.isValid());

						//cerco il suo loader
						res2::BaseLoader *loader = res_loaderList[(u32)descr->res_handle.get_type()];
						assert (NULL != loader);

						res2::CallbackData_onLoad callback_data;
						callback_data.descr = descr;
						if (loader->load (loaderInfo, descr, &callback_data))
							thread::pushMsg (msgqW, RES2__MSG_FROM_LOADER_THREAD__ON_LOAD_FINISHED_OK, 0, &callback_data);
						else
							thread::pushMsg (msgqW, RES2__MSG_FROM_LOADER_THREAD__ON_LOAD_FINISHED_KO, 0);
					}
					break;

				case RES2__MSG_FOR_LOADER_THREAD__RESOURCE_LOAD_FINISHED:
					//una risorsa e' stata caricata in precedenza e ora engine ha finito di usare
					//il file caricato. Devo fare il free di eventuali risorse allocate qui dentro durante
					//il file load
					{
						res2::CallbackData_onLoad *callback_data = (res2::CallbackData_onLoad*)msgList[i].buffer;

						//cerco il suo loader
						res2::BaseLoader *loader = res_loaderList[(u32)callback_data->descr->res_handle.get_type()];
						assert (NULL != loader);

						loader->load_finished (loaderInfo, callback_data);
					}
					break;
                }

                thread::deleteMsg (msgList[i]);
            }

            if (bQuit)
                break;
        }
    }


    //free dei loader
    for (u32 i=0; i<NUM_MAX_RES_LOADER; i++)
    {
        if (NULL != res_loaderList[i])
        {
            GOSDELETE(localAllocator, res_loaderList[i]);
            res_loaderList[i] = NULL;
        }
    }

    for (u32 i=0; i<NUM_MAX_ASSET_LOADER; i++)
    {
        if (NULL != asset_loaderList[i])
        {
            GOSDELETE(localAllocator, asset_loaderList[i]);
            asset_loaderList[i] = NULL;
        }
    }	

    //free dell'allocator
    GOSDELETE(gos::getSysHeapAllocator(), localAllocator);
    return 0;
}

