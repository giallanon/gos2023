#include "gosEngine.h"
#include "loaders/gosEngineLoaders_tex2D.h"
#include "loaders/gosEngineLoaders_shader.h"
#include "loaders/gosEngineLoaders_pipe.h"
#include "loaders/gosEngineLoaders_shape.h"
#include "loaders/gosEngineLoaders_skeleton.h"
#include "loaders/gosEngineLoaders_model3d.h"
#include "loaders/gosEngineLoaders_materialPBR.h"
#include "loaders/gosEngineLoaders_GPUShape.h"
#include "loaders/gosEngineLoaders_modelInst.h"



using namespace gos;
using namespace gos::engine;

typedef gos::AllocatorHeap<gos::AllocPolicy_Track_simple, gos::AllocPolicy_Thread_Unsafe>		GOSENGINELoaderMemAllocatorTS;

//***************************************
static void LoaderThread__do_load (HThreadMsgW msgqW, loaders::LoaderInfo &loaderInfo, loaders::BaseLoader **loaderList, res::Descr *res)
{
	const res::eType res_type = res->get_type();
	const asset2::UID uid = res->uid;
	loaderInfo.logger->log (eTextColor::darkGreen, "res::MT  [%s] [%08X] uid=%016" PRIX64 " load started\n", res::enumToString(res_type), res->handle.viewAsU32(), uid._uid);

	loaders::BaseLoader *loader = loaderList[(u32)res_type];
	assert (NULL != loader);

	loaders::CallbackData callback_data;
	callback_data.res = res;
	callback_data.user_data_pt = NULL;
	callback_data.user_data_1 = 0;
	callback_data.reschedule_load_at_time_msec = 0;

	switch (loader->load (loaderInfo, res, &callback_data))
	{
	default:
		DBGBREAK;
		thread::pushMsg (msgqW, Engine::MSG_FROM_LOADER_THREAD__ON_LOAD_FINISHED_KO, 0, res);
		break;

	case loaders::BaseLoader::eResult::failed:
		loaderInfo.logger->log (eTextColor::red, "res::MT  [%s] [%08X] uid=%016" PRIX64 " error loading\n", res::enumToString(res_type), res->handle.viewAsU32(), uid._uid);
		thread::pushMsg (msgqW, Engine::MSG_FROM_LOADER_THREAD__ON_LOAD_FINISHED_KO, 0, res);
		break;

	case loaders::BaseLoader::eResult::success:
		loaderInfo.logger->log (eTextColor::darkGreen, "res::MT  [%s] [%08X] uid=%016" PRIX64 " loaded\n", res::enumToString(res_type), res->handle.viewAsU32(), uid._uid);
		thread::pushMsg (msgqW, Engine::MSG_FROM_LOADER_THREAD__ON_LOAD_FINISHED_OK, 0, res);
		break;

	case loaders::BaseLoader::eResult::callback:
		loaderInfo.logger->log (eTextColor::darkGreen, "res::MT  [%s] [%08X] uid=%016" PRIX64 " callback\n", res::enumToString(res_type), res->handle.viewAsU32(), uid._uid);
		thread::pushMsg (msgqW, Engine::MSG_FROM_LOADER_THREAD__ON_LOAD_CALLBACK, 0, &callback_data, sizeof(callback_data));
		break;
	}
}

//***************************************
i16	Engine::LoaderThread_mainFN (void *paramsIN)
{
    GOSENGINELoaderMemAllocatorTS *localAllocator = GOSNEW(gos::getSysHeapAllocator(), GOSENGINELoaderMemAllocatorTS)("ENGLoader");
    localAllocator->setup (1024 * 1024 * 128); //128MB
    
    HThreadMsgR         msgqR;
    HThreadMsgW         msgqW;
    loaders::LoaderInfo loaderInfo;

    //copia dei params
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

    //spawn dei loader
    static constexpr u32 NUM_MAX_LOADER = 1 + (u32)res::eType::NUM_MAX;
    loaders::BaseLoader  *loaderList[NUM_MAX_LOADER];
    {
        memset (loaderList, 0, sizeof(loaderList));
        loaderList[(u32)res::eType::vtx_shader] = GOSNEW(localAllocator, loaders::Loader_vtxShader)();
        loaderList[(u32)res::eType::pxl_shader] = GOSNEW(localAllocator, loaders::Loader_pxlShader)();
        loaderList[(u32)res::eType::texture_2d] = GOSNEW(localAllocator, loaders::Loader_tex2D)();
        loaderList[(u32)res::eType::pipeline] = GOSNEW(localAllocator, loaders::Loader_pipeline)();
		loaderList[(u32)res::eType::shape] = GOSNEW(localAllocator, loaders::Loader_shape)();
		loaderList[(u32)res::eType::skeleton] = GOSNEW(localAllocator, loaders::Loader_skeleton)();
		loaderList[(u32)res::eType::model_3d] = GOSNEW(localAllocator, loaders::Loader_model3d)();
		loaderList[(u32)res::eType::materialPBR] = GOSNEW(localAllocator, loaders::Loader_materialPBR)();
		loaderList[(u32)res::eType::gpu_shape] = GOSNEW(localAllocator, loaders::Loader_GPUShape)();
		loaderList[(u32)res::eType::model_instance] = GOSNEW(localAllocator, loaders::Loader_modelInst)();
    }

	struct RescheduledMsg
	{
		u64			time_msec;
		res::Descr *res;
	};
	gos::FastArray<RescheduledMsg>	reschedule_list(localAllocator, 256);


    //loop
    static constexpr u8 NUM_MAX_MESSAGES = 64;
    thread::sMsg msgList[NUM_MAX_MESSAGES];
    bool bQuit = false;
    while (bQuit == false)
    {
		if (0 != reschedule_list.getNElem())
		{
			thread::waitForAnEvent (msgqR, 300);
		}
		else
		{
			if (!thread::waitForAnEvent (msgqR, u32MAX))
				break;
		}


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

                case MSG_FOR_LOADER_THREAD__DIE:
                    bQuit = true;
                    break;

				case MSG_FOR_LOADER_THREAD__LOAD:
					{
						res::Descr *res = (res::Descr*)msgList[i].buffer;
						if (true == bQuit)
							thread::pushMsg (msgqW, MSG_FROM_LOADER_THREAD__ON_LOAD_FINISHED_KO, 0, res);
						else
							LoaderThread__do_load (msgqW, loaderInfo, loaderList, res);
					}
					break;

				case MSG_FOR_LOADER_THREAD__LOAD_CONTINUE:
					{
						bool anyError = (msgList[i].paramU64 != 0);
						engine::loaders::CallbackData *callback_data = (engine::loaders::CallbackData*)msgList[i].buffer;
						res::Descr *res = callback_data->res;
						
						if (true == bQuit)
						{
							thread::pushMsg (msgqW, MSG_FROM_LOADER_THREAD__ON_LOAD_FINISHED_KO, 0, res);
						}
						else
						{
							const res::eType res_type = res->get_type();
							const asset2::UID uid = res->uid;
							loaderInfo.logger->log (eTextColor::darkGreen, "res::MT  [%s] [%08X] uid=%016" PRIX64 " load continued\n", res::enumToString(res_type), res->handle.viewAsU32(), uid._uid);

							loaders::BaseLoader *loader = loaderList[(u32)res_type];
							assert (NULL != loader);
							if (loader->load_continued (loaderInfo, anyError, callback_data))
							{
								if (0 == callback_data->reschedule_load_at_time_msec)
								{
									loaderInfo.logger->log (eTextColor::darkGreen, "res::MT  [%s] [%08X] uid=%016" PRIX64 " loaded\n", res::enumToString(res_type), res->handle.viewAsU32(), uid._uid);
									thread::pushMsg (msgqW, MSG_FROM_LOADER_THREAD__ON_LOAD_FINISHED_OK, 0, res);
								}
								else
								{
									//mi e' stato chiesto di reschedulare questo load fra un po' di tempo
									RescheduledMsg m = {
										.time_msec = callback_data->reschedule_load_at_time_msec,
										.res = res
									};

									loaderInfo.logger->log (eTextColor::darkGreen, "res::MT  [%s] [%08X] uid=%016" PRIX64 " rescheduling @%" PRIu64 "\n", res::enumToString(res_type), res->handle.viewAsU32(), uid._uid, m.time_msec);
									reschedule_list.append (m);
								}
							}
							else
							{
								loaderInfo.logger->log (eTextColor::red, "res::MT  [%s] [%08X] uid=%016" PRIX64 " error loading\n", res::enumToString(res_type), res->handle.viewAsU32(), uid._uid);
								thread::pushMsg (msgqW, MSG_FROM_LOADER_THREAD__ON_LOAD_FINISHED_KO, 0, res);
							}
						}
					}
					break;
                }

                thread::deleteMsg (msgList[i]);
            }

            if (bQuit)
                break;
        }

		u32 nToBeRescheduled = reschedule_list.getNElem();
		if (!bQuit && nToBeRescheduled)
		{
			
			const u64 timenow_msec = gos::getTimeSinceStart_msec();
			for (u32 i=0; i<nToBeRescheduled; i++)
			{
				if (timenow_msec >= reschedule_list(i).time_msec)
				{
					res::Descr *res = reschedule_list(i).res;

					reschedule_list.removeAndSwapWithLast(i);
					i--;
					nToBeRescheduled--;

					LoaderThread__do_load (msgqW, loaderInfo, loaderList, res);
				}
			}
		}
    }

	for (u32 i=0; i<reschedule_list.getNElem(); i++)
	{
		thread::pushMsg (msgqW, MSG_FROM_LOADER_THREAD__ON_LOAD_FINISHED_KO, 0, reschedule_list[i].res);
	}
	reschedule_list.unsetup();

    //free dei loader
    for (u32 i=0; i<NUM_MAX_LOADER; i++)
    {
        if (NULL != loaderList[i])
        {
            GOSDELETE(localAllocator, loaderList[i]);
            loaderList[i] = NULL;
        }
    }


    //free dell'allocator
    GOSDELETE(gos::getSysHeapAllocator(), localAllocator);
    return 0;
}

