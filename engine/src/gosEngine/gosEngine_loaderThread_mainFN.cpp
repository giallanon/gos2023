#include "gosEngine.h"
#include "loaders/gosEngineLoaders_tex2D.h"
#include "loaders/gosEngineLoaders_shader.h"
#include "loaders/gosEngineLoaders_pipe.h"
#include "loaders/gosEngineLoaders_shape.h"
#include "loaders/gosEngineLoaders_skeleton.h"
#include "loaders/gosEngineLoaders_model3d.h"
#include "loaders/gosEngineLoaders_materialPBR.h"



using namespace gos;
using namespace gos::engine;

typedef gos::AllocatorHeap<gos::AllocPolicy_Track_simple, gos::AllocPolicy_Thread_Unsafe>		GOSENGINELoaderMemAllocatorTS;


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
		loaderInfo.stageHelper.setup (params->gpu, 8192*8192);

        //segnalo che sono partito
        thread::eventFire (params->hEvent_started);
    }

    //spawn dei loader
    static constexpr u32 NUM_MAX_LOADER = 1 + (u32)eAssetType::__NUM;
    loaders::BaseLoader  *loaderList[NUM_MAX_LOADER];
    {
        memset (loaderList, 0, sizeof(loaderList));
        loaderList[(u32)eAssetType::vtx_shader] = GOSNEW(localAllocator, loaders::Loader_vtxShader)();
        loaderList[(u32)eAssetType::pxl_shader] = GOSNEW(localAllocator, loaders::Loader_pxlShader)();
        loaderList[(u32)eAssetType::tex2D] = GOSNEW(localAllocator, loaders::Loader_tex2D)();
        loaderList[(u32)eAssetType::pipe] = GOSNEW(localAllocator, loaders::Loader_pipeline)();
		loaderList[(u32)eAssetType::shape] = GOSNEW(localAllocator, loaders::Loader_shape)();
		loaderList[(u32)eAssetType::skeleton] = GOSNEW(localAllocator, loaders::Loader_skeleton)();
		loaderList[(u32)eAssetType::model3d] = GOSNEW(localAllocator, loaders::Loader_model3d)();
		loaderList[(u32)eAssetType::materialPBR] = GOSNEW(localAllocator, loaders::Loader_materialPBR)();
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

                case MSG_FOR_LOADER_THREAD__DIE:
                    bQuit = true;
                    break;

				case MSG_FOR_LOADER_THREAD__LOAD:
					{
						res::Descr *res = (res::Descr*)msgList[i].buffer;

                        const asset2::UID uid = res->uid;
                        loaderInfo.logger->log (eTextColor::darkGreen, "res::MT  [%s] %016" PRIX64 " do load\n", asset2::enumToString(uid.getAssetType()), uid._uid);

                        loaders::BaseLoader *loader = loaderList[(u32)uid.getAssetType()];
                        assert (NULL != loader);
                        if (loader->load (loaderInfo, res))
						{
							loaderInfo.logger->log (eTextColor::darkGreen, "res::MT  [%s] %016" PRIX64 " loaded\n", asset2::enumToString(uid.getAssetType()), uid._uid);
                            thread::pushMsg (msgqW, MSG_FROM_LOADER_THREAD__ON_LOAD_FINISHED_OK, 0, res);
						}
                        else
						{
							loaderInfo.logger->log (eTextColor::darkGreen, "res::MT  [%s] %016" PRIX64 " ERROR LOADING\n", asset2::enumToString(uid.getAssetType()), uid._uid);
                            thread::pushMsg (msgqW, MSG_FROM_LOADER_THREAD__ON_LOAD_FINISHED_KO, 0, res);
						}
					}
					break;

                }

                thread::deleteMsg (msgList[i]);
            }

            if (bQuit)
                break;
        }
    }

    //TODO:: eliminare tutti i messaggi pendendi nel caso in cui ci siano ancora
    //risorse da unloadare


    //free dei loader
    for (u32 i=0; i<NUM_MAX_LOADER; i++)
    {
        if (NULL != loaderList[i])
        {
            GOSDELETE(localAllocator, loaderList[i]);
            loaderList[i] = NULL;
        }
    }

	loaderInfo.stageHelper.unsetup();

    //free dell'allocator
    GOSDELETE(gos::getSysHeapAllocator(), localAllocator);
    return 0;
}

