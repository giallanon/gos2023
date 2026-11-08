#include "gosEngine.h"
#include "res2/loaders/Loader_tex2D.h"
#include "res2/loaders/Loader_materialPBR.h"
#include "res2/loaders/Loader_shader.h"
#include "res2/loaders/Loader_pipeline.h"
#include "res2/loaders/Loader_gpushape.h"
#include "res2/loaders/Loader_skeleton.h"
#include "res2/loaders/Loader_model.h"

using namespace gos;
using namespace gos::engine;

typedef gos::AllocatorHeap<gos::AllocPolicy_Track_simple, gos::AllocPolicy_Thread_Unsafe>		res2__GOSENGINELoaderMemAllocatorTS;

/***************************************************
 * 
 */
class LoaderThread
{
public:
	void 	run (void *paramsIN);

private:
    static constexpr u32 NUM_MAX_RES_LOADER = 1 + (u32)res2::eResType::NUM_MAX;
	static constexpr u32 TOO_MANY_LOAD_PENDING = 16;
	static constexpr u32 MESSAGES__NUM_MAX_PENDING = 1024;
    static constexpr u32 MESSAGES__NUM_MAX_PER_WAIT = 64;

private:
	res2__GOSENGINELoaderMemAllocatorTS *localAllocator;
	res2::LoaderInfo					loaderInfo;
    HThreadMsgR 						msgqR;
    HThreadMsgW 						msgqW;
    res2::BaseLoader  					*res_loaderList[NUM_MAX_RES_LOADER];
    thread::sMsg 						msgList[MESSAGES__NUM_MAX_PER_WAIT];
	gos::FIFOFixedSize<thread::sMsg,MESSAGES__NUM_MAX_PENDING>	pending_msgList;
	u32 num_load_request_pending;
};


//***************************************
i16	Engine::res2__LoaderThread_mainFN (void *paramsIN)		{ LoaderThread m; m.run(paramsIN); return 0; }

void LoaderThread::run (void *paramsIN)
{
    localAllocator = GOSNEW(gos::getSysHeapAllocator(), res2__GOSENGINELoaderMemAllocatorTS)("res2__ENGLoader");
    localAllocator->setup (1024 * 1024 * 128); //128MB
    
    //copia dei params
    {
        const engine::sLoaderThreadInitParams *params = reinterpret_cast<const engine::sLoaderThreadInitParams*>(paramsIN);
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
    {
        memset (res_loaderList, 0, sizeof(res_loaderList));
        res_loaderList[(u32)res2::eResType::texture_2d] = GOSNEW(localAllocator, res2::Loader_tex2D)();
		res_loaderList[(u32)res2::eResType::materialPBR] = GOSNEW(localAllocator, res2::Loader_materialPBR)();
		res_loaderList[(u32)res2::eResType::vtx_shader] = GOSNEW(localAllocator, res2::Loader_VtxShader)();
		res_loaderList[(u32)res2::eResType::pxl_shader] = GOSNEW(localAllocator, res2::Loader_PxlShader)();
		res_loaderList[(u32)res2::eResType::pipeline] = GOSNEW(localAllocator, res2::Loader_pipeline)();
		res_loaderList[(u32)res2::eResType::gpu_shape] = GOSNEW(localAllocator, res2::Loader_gpushape)();
		res_loaderList[(u32)res2::eResType::skeleton] = GOSNEW(localAllocator, res2::Loader_skeleton)();
		res_loaderList[(u32)res2::eResType::model_3d] = GOSNEW(localAllocator, res2::Loader_model)();
    }


    //loop
	num_load_request_pending = 0;

    bool bQuit = false;
    while (bQuit == false)
    {
		//attesa messaggi
		{
			if (num_load_request_pending > 0)
			{
				thread::waitForAnEvent (msgqR, 10);
			}
			else
			{
				if (!thread::waitForAnEvent (msgqR, u32MAX))
				{
					bQuit = true;
					continue;
				}
			}
		}

        u32 nMsg;
        while (0 != (nMsg = thread::popMultipleMsg(msgqR, msgList, MESSAGES__NUM_MAX_PER_WAIT)))
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
					if (num_load_request_pending >= TOO_MANY_LOAD_PENDING)
					{
						//sto gia' processando un certo numero di load che sono al momento in attesa di callback.
						//Evito di schedularne ulteriori per evitare di allocare infinita memoria nel loader.
						//Mi salvo questi messaggi e li processo in seguito
						pending_msgList.push (msgList[i]);
					}
					else
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

						case res2::BaseLoader::eResult::failed:
							thread::pushMsg (msgqW, RES2__MSG_FROM_LOADER_THREAD__ON_LOAD_FINISHED_KO, 0);
							break;

						case res2::BaseLoader::eResult::callback:
							num_load_request_pending++;
							thread::pushMsg (msgqW, RES2__MSG_FROM_LOADER_THREAD__ON_LOAD_FINISHED_OK_WANT_CALLBACK, 0, &callback_data, sizeof(res2::CallbackData_onLoad));
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
							loader->load_finished (loaderInfo, callback_data);
							assert (num_load_request_pending > 0);
							num_load_request_pending--;
							break;

						case res2::BaseLoader::eResult::failed:
							thread::pushMsg (msgqW, RES2__MSG_FROM_LOADER_THREAD__ON_LOAD_FINISHED_KO, 0);
							loader->load_finished (loaderInfo, callback_data);
							assert (num_load_request_pending > 0);
							num_load_request_pending--;
							break;

						case res2::BaseLoader::eResult::callback:
							thread::pushMsg (msgqW, RES2__MSG_FROM_LOADER_THREAD__ON_LOAD_FINISHED_OK_WANT_CALLBACK, 0, callback_data, sizeof(res2::CallbackData_onLoad));
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

						assert (num_load_request_pending > 0);
						num_load_request_pending--;
					}
					break;
                }

                thread::deleteMsg (msgList[i]);
            }

            if (bQuit)
                break;
        }


		//richieste pendenti di load
		while (pending_msgList.getNElem() && num_load_request_pending < TOO_MANY_LOAD_PENDING)
		{
			thread::sMsg msg;
			pending_msgList.pop(&msg);
			assert (RES2__MSG_FOR_LOADER_THREAD__LOAD == msg.what);

			res2::ResDescr *descr = (res2::ResDescr*)msg.buffer;
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

			case res2::BaseLoader::eResult::failed:
				thread::pushMsg (msgqW, RES2__MSG_FROM_LOADER_THREAD__ON_LOAD_FINISHED_KO, 0);
				break;

			case res2::BaseLoader::eResult::callback:
				num_load_request_pending++;
				thread::pushMsg (msgqW, RES2__MSG_FROM_LOADER_THREAD__ON_LOAD_FINISHED_OK_WANT_CALLBACK, 0, &callback_data, sizeof(res2::CallbackData_onLoad));
				break;							
			}
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
}

