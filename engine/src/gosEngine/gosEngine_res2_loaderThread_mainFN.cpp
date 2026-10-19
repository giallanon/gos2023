#include "gosEngine.h"
#include "res2/loaders/Loader_tex2D.h"
#include "res2/loaders/Loader_materialPBR.h"
#include "res2/loaders/Loader_shader.h"
#include "res2/loaders/Loader_pipeline.h"
#include "res2/loaders/Loader_gpushape.h"

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
		res_loaderList[(u32)res2::eResType::materialPBR] = GOSNEW(localAllocator, res2::Loader_materialPBR)();
		res_loaderList[(u32)res2::eResType::vtx_shader] = GOSNEW(localAllocator, res2::Loader_VtxShader)();
		res_loaderList[(u32)res2::eResType::pxl_shader] = GOSNEW(localAllocator, res2::Loader_PxlShader)();
		res_loaderList[(u32)res2::eResType::pipeline] = GOSNEW(localAllocator, res2::Loader_pipeline)();
		res_loaderList[(u32)res2::eResType::gpu_shape] = GOSNEW(localAllocator, res2::Loader_gpushape)();
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

				case RES2__MSG_FOR_LOADER_THREAD__LOAD:
					//richiesta di caricare una risorsa
					{
						res2::ResDescr *descr = (res2::ResDescr*)msgList[i].buffer;
						assert (descr->UID.isValid());
						assert (descr->UID.isASignature());

						//cerco il suo loader
						res2::BaseLoader *loader = res_loaderList[(u32)descr->res_handle.get_res_type()];
						assert (NULL != loader);

						res2::CallbackData_onLoad callback_data;
						callback_data.descr = descr;

						switch (loader->load (loaderInfo, descr, &callback_data))
						{
						default:
							DBGBREAK;
							break;

						case res2::BaseLoader::eResult::success:
							thread::pushMsg (msgqW, RES2__MSG_FROM_LOADER_THREAD__ON_LOAD_FINISHED_OK, 0, &callback_data, sizeof(res2::CallbackData_onLoad));
							break;

						case res2::BaseLoader::eResult::callback:
							thread::pushMsg (msgqW, RES2__MSG_FROM_LOADER_THREAD__ON_LOAD_FINISHED_OK_WANT_CALLBACK, 0, &callback_data, sizeof(res2::CallbackData_onLoad));
							break;							

						case res2::BaseLoader::eResult::failed:
							thread::pushMsg (msgqW, RES2__MSG_FROM_LOADER_THREAD__ON_LOAD_FINISHED_KO, 0);
							break;
						}
					}
					break;
				
				case RES2__MSG_FOR_LOADER_THREAD__LOAD_CONTINUED:
					//la fase di load necessita di ulteriori passaggi.
					//Ci stiamo palleggiano la palla tra engine e this
					{
						res2::CallbackData_onLoad *callback_data = (res2::CallbackData_onLoad*)msgList[i].buffer;

						//cerco il suo loader
						res2::BaseLoader *loader = res_loaderList[(u32)callback_data->descr->res_handle.get_res_type()];
						assert (NULL != loader);

						switch (loader->load_continued (loaderInfo, callback_data))
						{
						default:
							DBGBREAK;
							break;

						case res2::BaseLoader::eResult::success:
							thread::pushMsg (msgqW, RES2__MSG_FROM_LOADER_THREAD__ON_LOAD_FINISHED_OK, 0, callback_data, sizeof(res2::CallbackData_onLoad));
							break;

						case res2::BaseLoader::eResult::callback:
							thread::pushMsg (msgqW, RES2__MSG_FROM_LOADER_THREAD__ON_LOAD_FINISHED_OK_WANT_CALLBACK, 0, callback_data, sizeof(res2::CallbackData_onLoad));
							break;							

						case res2::BaseLoader::eResult::failed:
							thread::pushMsg (msgqW, RES2__MSG_FROM_LOADER_THREAD__ON_LOAD_FINISHED_KO, 0);
							break;
						}
					}
					break;

				case RES2__MSG_FOR_LOADER_THREAD__LOAD_FINISHED:
					//una risorsa e' stata caricata in precedenza e ora engine ha finito di usare
					//il file caricato. Devo fare il free di eventuali risorse allocate qui dentro durante
					//il file load
					{
						res2::CallbackData_onLoad *callback_data = (res2::CallbackData_onLoad*)msgList[i].buffer;

						//cerco il suo loader
						res2::BaseLoader *loader = res_loaderList[(u32)callback_data->descr->res_handle.get_res_type()];
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

    //free dell'allocator
    GOSDELETE(gos::getSysHeapAllocator(), localAllocator);
    return 0;
}

