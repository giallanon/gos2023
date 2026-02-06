#include "gosEngine.h"
#include "../gos/memory/gosAllocatorHeap.h"
#include "../gosAsset2/gosAsset2Builder.h"

using namespace gos;

typedef gos::AllocatorHeap<gos::AllocPolicy_Track_simple, gos::AllocPolicy_Thread_Safe>		GOSENGINEMemAllocatorTS;

#define GOS_ENGINE__ASSET_HUB_PATH "@w/assets"

//******************************** 
Engine::Engine()
{
    allocator = NULL;
    gpu = NULL;
    inputCtx = NULL;
    bQuitEngine = false;
    asset_logger = NULL;
    memset (resHandler_list, 0, sizeof(resHandler_list));
}

//******************************** 
void Engine::unsetup()
{
    bQuitEngine = true;
    if (NULL == gpu || NULL == allocator)
        return;

    //chiedo al thread di morire e aspetto che termini
    thread::pushMsg (msgq_1W, MSG_FOR_LOADER_THREAD__DIE, 0);
    thread::waitEnd (hThreadLoader);
    thread::deleteMsgQ (msgq_1R, msgq_1W);
    thread::deleteMsgQ (msgq_2R, msgq_2W);

    //resource manager
	resManager.unsetup();
    vtxBufferMan.unsetup();
    idxBufferMan.unsetup();
    listof_knownUID.unsetup();

    
    //handle lists
    handleList_GPUShape.unsetup();
	handleList_model3dInst.unsetup();
	map_of_shape_to_gpushape.unsetup();
    resHandler_texture.unsetup();
    resHandler_pipeline.unsetup();
    resHandler_shape.unsetup();
	resHandler_skeleton.unsetup();
	resHandler_model3d.unsetup();

    asset2::dbcontext_close (asset_ctx);

    //win & gpu
    GOSWinHandle mainWin = gpu->getWindow();
    
    gpu->deinit();
    GOSDELETE(gos::getSysHeapAllocator(), gpu);
    gpu = NULL;

    GOSDELETE(gos::getSysHeapAllocator(), inputCtx);
    inputCtx = NULL;

    gos::input::window_destroy (mainWin);
    gos::input::deinit();

    priv_resList_unsetup();

    GOSDELETE(allocator, asset_logger);

    //engine allocator
    GOSDELETE(gos::getSysHeapAllocator(), allocator);
    allocator = NULL;
}

//******************************** 
bool Engine::setup (u32 mainWin_w, u32 mainWin_h, const char *mainWin_title)
{
    if (!gos::input::init())
    {
        logger::err ("Engine::setup() => input::init failed\n");
        return false;
    }

    //main win
    GOSWinHandle mainWin;
    if (!gos::input::window_create (1024, 768, gos::getAppName(), &mainWin))
    {
        logger::err ("Engine::setup() => input::window_create failed\n");
        return false;
    }
    gos::input::window_setTitle (mainWin, mainWin_title);

    //GPU
    gpu = GOSNEW(gos::getSysHeapAllocator(), gos::GPU)();
    if (!gpu->init (mainWin, false))
    {
        logger::err ("Engine::setup() => gpu->init() failed\n");
        return false;
    }

    //Creo un allocatore dedicato
    GOSENGINEMemAllocatorTS *engAllocator = GOSNEW(gos::getSysHeapAllocator(), GOSENGINEMemAllocatorTS)("ENG");
    engAllocator->setup (1024 * 1024 * 128); //128MB
    this->allocator = engAllocator;

    priv_resList_setup();

    //asset
    {
        gos::LoggerStdout *ll = GOSNEW(allocator,gos::LoggerStdout)();
        ll->enableStdouLogging();
        asset_logger = ll;
        if (!asset2::dbcontext_open (GOS_ENGINE__ASSET_HUB_PATH, true, &asset_ctx))
        {
            logger::err ("Engine::setup() => can't open asset contex in %s\n", GOS_ENGINE__ASSET_HUB_PATH);
            return false;
        }
    }    
    

    //loader thread
    //creo 2 code, una che uso per mandare msg da this al thread e una per mandare
    //msg dal thread a this
    thread::createMsgQ (&msgq_1R, &msgq_1W);
    thread::createMsgQ (&msgq_2R, &msgq_2W);

    sLoaderThreadInitParams params;
    params.msgqR = msgq_1R;
    params.msgqW = msgq_2W;
    params.logger = asset_logger;
    params.gpu = gpu;
    params.ctx = &asset_ctx;
	params.engine_allocator = this->allocator;
	params.engine = this;
    thread::eventCreate (&params.hEvent_started);

    eThreadError err = thread::create (&hThreadLoader, Engine::LoaderThread_mainFN, &params);
    if (err != eThreadError::none)
    {
        logger::err ("Engine::setup() => error creating thread: errcode=%d\n", static_cast<int>(err));
        return false;
    }



    //creo l'input context di default
    inputCtx = GOSNEW(gos::getSysHeapAllocator(), input::Context)("global");
    {
        
        inputCtx->action_add ("app_terminate")
            .action_add ("toggle_fullscreen")
            .action_add ("toggle_vsync")
            .action_add ("show_all_actions")

            .action_add ("toggle_mouse_mode")
            .action_add ("move_forward")
            .action_add ("move_backward")
            .action_add ("strafe_left")
            .action_add ("strafe_right")
            .action_add ("strafe_up")
            .action_add ("strafe_down")
            .action_add ("rotateX")
            .action_add ("rotateY");
            


        inputCtx->action_bindToBtn ("app_terminate", input::eOrigin::keyboard, GLFW_KEY_Q, input::eButtonStatus::pressed, input::sButtonModifier(input::eButtonModifier::LCTRL));
        inputCtx->action_bindToBtn ("app_terminate", input::eOrigin::window, GOS_BUTTON_WINDOW_CLOSE, input::eButtonStatus::pressed);
        inputCtx->action_bindToBtn ("toggle_fullscreen", input::eOrigin::keyboard, GLFW_KEY_ENTER, input::eButtonStatus::pressed, input::sButtonModifier(input::eButtonModifier::LALT));
        inputCtx->action_bindToBtn ("toggle_vsync", input::eOrigin::keyboard, GLFW_KEY_BACKSPACE, input::eButtonStatus::pressed, input::sButtonModifier(input::eButtonModifier::LALT));
        inputCtx->action_bindToBtn ("show_all_actions", input::eOrigin::keyboard, GLFW_KEY_F1, input::eButtonStatus::pressed, input::sButtonModifier(input::eButtonModifier::LCTRL, input::eButtonModifier::LSHIFT));
        
        inputCtx->action_bindToBtn ("toggle_mouse_mode", input::eOrigin::keyboard, GLFW_KEY_TAB, input::eButtonStatus::pressed);
        inputCtx->action_bindToBtn ("move_forward", input::eOrigin::keyboard, GLFW_KEY_W, input::eButtonStatus::both);
        inputCtx->action_bindToBtn ("move_backward", input::eOrigin::keyboard, GLFW_KEY_S, input::eButtonStatus::both);
        inputCtx->action_bindToBtn ("strafe_left", input::eOrigin::keyboard, GLFW_KEY_A, input::eButtonStatus::both);
        inputCtx->action_bindToBtn ("strafe_right", input::eOrigin::keyboard, GLFW_KEY_D, input::eButtonStatus::both);
        inputCtx->action_bindToBtn ("strafe_up", input::eOrigin::keyboard, GLFW_KEY_Q, input::eButtonStatus::both);
        inputCtx->action_bindToBtn ("strafe_down", input::eOrigin::keyboard, GLFW_KEY_Z, input::eButtonStatus::both);

        inputCtx->action_bindToAxleREL ("rotateX",  input::eOrigin::mouse, input::eAxle::y, input::eAxleDirection::both);
        inputCtx->action_bindToAxleREL ("rotateY",  input::eOrigin::mouse, input::eAxle::x, input::eAxleDirection::both);    

    }


    //handle list
	resManager.setup (allocator);
	resManager.addResType<res::VtxBuffer> (res::eType::vtx_buffer, 1024, 128);
	resManager.addResType<res::IdxBuffer> (res::eType::idx_buffer, 1024, 128);
	resManager.addResType<res::Shader>	  (res::eType::vtx_shader, 1024, 256);
	resManager.addResType<res::Shader>	  (res::eType::pxl_shader, 1024, 256);


    handleList_GPUShape.setup (allocator);
	handleList_model3dInst.setup (allocator);
	map_of_shape_to_gpushape.setup (allocator, 8192);
    priv_setup_resource_handler(eAssetType::tex2D,      &resHandler_texture);
    priv_setup_resource_handler(eAssetType::pipe,       &resHandler_pipeline);
    priv_setup_resource_handler(eAssetType::shape,      &resHandler_shape);
	priv_setup_resource_handler(eAssetType::skeleton,	&resHandler_skeleton);
	priv_setup_resource_handler(eAssetType::model3d,	&resHandler_model3d);
    
    //resource manager
    vtxBufferMan.setup (allocator, gpu);
    idxBufferMan.setup (allocator, gpu);
    listof_knownUID.setup (allocator, 8192);


    //attendo che il loader-thread abbia segnalato che e' partito
    if (!thread::eventWait (params.hEvent_started, 20000))
    {
        logger::err ("Engine::setup() => error waiting for thread to start\n");
        return false;
    }
    thread::eventDestroy (params.hEvent_started);

    return true;
}

//************************************
void Engine::toggleVSync()
{ 
    if (gpu->vsync_isEnabled())
    {
        gpu->vsync_enable (false);
        gos::logger::log (eTextColor::yellow, "VSYNC: off\n");
    }
    else
    {
        gpu->vsync_enable (true);
        gos::logger::log (eTextColor::yellow, "VSYNC: on\n");
    }
}

//******************************** 
bool Engine::update()
{
    if (bQuitEngine)
        return false;

    //loader-thread
    priv_flushLoaderThreadMsg();

    //input
    gos::input::pollEvents();
    input::resolveEvents (gpu->getWindow(), inputCtx, &evtList);
    return true;
}

//******************************** 
void Engine::priv_flushLoaderThreadMsg()
{
    const u32 nMsg = thread::popMultipleMsg(msgq_2R, loaderMsgList, LOADER_THREAD__NUM_MAX_MESSAGES_TO_READ);
    for (u32 i=0; i<nMsg; i++)
    {
        switch (loaderMsgList[i].what)
        {
        default:
            DBGBREAK;
            break;

        case MSG_FROM_LOADER_THREAD__ON_LOAD_FINISHED_OK:
        case MSG_FROM_LOADER_THREAD__ON_LOAD_FINISHED_KO:
            //il thread mi segnala che una risorsa e' stata caricata
            {
                const bool bLoadOK = (MSG_FROM_LOADER_THREAD__ON_LOAD_FINISHED_OK == loaderMsgList[i].what);

                asset2::UID uid;
                uid._uid = loaderMsgList[i].paramU64;
                void *res = loaderMsgList[i].buffer;

                if (bLoadOK)
                    asset_logger->log (eTextColor::darkGreen, "asset::  [%s] %016" PRIX64 " loaded\n", asset2::enumToString(uid.getAssetType()), uid._uid);
                else
                    asset_logger->log (eTextColor::red, "asset::  [%s] %016" PRIX64 " FAILED to load\n", asset2::enumToString(uid.getAssetType()), uid._uid);


                engine::Resource *brh = static_cast<engine::Resource*>(res);
                if (bLoadOK)
                    brh->status = engine::eResStatus::ready;
                else
                    brh->status = engine::eResStatus::error;


				//informo tutti quelli che dipendono da questa risorsa del suo cambio di stato
				// engine::ResHandleDepList *p = brh->deplist;
				// while (p)
				// {
				// 	p->brh->callback_onSubresStateChanged (p->brh, brh);
				// 	p = p->next;
				// }
            }
            break;
        }

        thread::deleteMsg (loaderMsgList[i]);
    }
}

//******************************** 
input::eMouseMode Engine::getMouseMode() const
{
    return input::window_getMouseMode(gpu->getWindow());
}

//******************************** 
void Engine::setMouseMode (input::eMouseMode mode)
{
    input::window_setMouseMode (gpu->getWindow(), mode);
}

//******************************** 
bool Engine::inputEvent_getNext (InputEvent *out)
{
    while (1)
    {
        out->actionID = evtList.nextActionID(&out->value);
        if (0 == out->actionID)
            return false;

        switch (out->actionID)
        {
        default:
            return true;
            break;

        case COMPILE_TIME_STR_CRC32("show_all_actions"):
            inputCtx->logAllMappedInput();
            break;

        case COMPILE_TIME_STR_CRC32("app_terminate"):
            bQuitEngine = true;
            break;

        case COMPILE_TIME_STR_CRC32("toggle_fullscreen"):
            this->toggleFullscreen();
            break;

        case COMPILE_TIME_STR_CRC32("toggle_vsync"):
            this->toggleVSync();
            break;

        case COMPILE_TIME_STR_CRC32("toggle_mouse_mode"):
        printf ("engine:: toggle mouse mode\n");
            input::window_toggleMouseMode(gpu->getWindow());
            break;
        }
    }
}

//******************************** 
bool Engine::asset_rebuildAll()
{
    asset2::dbcontext_close (asset_ctx);
    gos::asset2::Builder builder (gpu);
    const bool ret = builder.rebuildAll (GOS_ENGINE__ASSET_HUB_PATH, true);
    asset2::dbcontext_open (GOS_ENGINE__ASSET_HUB_PATH, true, &asset_ctx);
    return ret;
}

//******************************** 
bool Engine::asset_build()
{
    asset2::dbcontext_close (asset_ctx);
    gos::asset2::Builder builder (gpu);
    const bool ret = builder.build (GOS_ENGINE__ASSET_HUB_PATH, true);
    asset2::dbcontext_open (GOS_ENGINE__ASSET_HUB_PATH, true, &asset_ctx);
    return ret;

}

//*****************************************
bool Engine::asset_bind (asset2::UID uid, engine::Resource *resPadre, u32 handle_asU32)
{
    if (listof_knownUID.insertIfNotExists (uid, handle_asU32))
    {
        asset_logger->log (eTextColor::darkGreen, "asset::  [%s] %016" PRIX64 " bind to handle %08X\n", asset2::enumToString(uid.getAssetType()), uid._uid, handle_asU32);

        bool ret = true;

        //gestisco le dipendenze di questo asset. Se lui dipende da altri, prima creare anche quelli
        u8 memblock[1024];
        asset2::FastUIDList fastUIDList;
        fastUIDList.setupWithBase (memblock, sizeof(memblock), gos::getScrapAllocator());

        asset_logger->incIndent();
        asset2::asset_get_runtime_dependecies_list (asset_ctx, uid, false, &fastUIDList);
        for (u32 i=0; i<fastUIDList.getNElem(); i++)
        {
            const asset2::UID uid_child = fastUIDList(i);
            BaseResourceHandler *base_resHandler = priv_get_baseResourceHandler(uid_child.getAssetType());


            u32 subres_handleAsU32;
            if (!internal__from_asset_to_handle (uid_child, &subres_handleAsU32))
            {
                //devo creare un handle appropriato per l'asset in questione
                if (!base_resHandler->handle_get_or_create_from_asset (this, uid_child, engine::eLoadMode::onDemand, &subres_handleAsU32))
                    ret = false;
            }

            //uid_child e' un mio figlio, quindi mi segno che fa parte delle mie sub-resource
            engine::Resource *subres = base_resHandler->from_handle_to_resource(subres_handleAsU32);
            assert (NULL != subres);
            priv_resList_add_figlio (resPadre, subres);

            //quando subres cambia stato, resPadre vuole esserne informato
            priv_resList_add_owner (subres, resPadre);
            


        }
        asset_logger->decIndent();
        
        return ret;
    }
    else
    {
        //uid era gia' nella lista degli asset bindati.. e' un errore
        DBGBREAK;
        return true;
    }
}

/**************************************************************** 
 * RES
 *****************************************************************/
bool Engine::res_assetUID_to_resUID (asset2::UID uid, res::eType *out_res_type) const
{
    assert (uid.isAnAsset());
    switch (uid.getAssetType())
    {
    default:
        DBGBREAK;
        return false;
    case eAssetType::vtx_shader:    *out_res_type =  res::eType::vtx_shader; return true;
    case eAssetType::pxl_shader:    *out_res_type =  res::eType::pxl_shader; return true;
    case eAssetType::pipe:          *out_res_type =  res::eType::pipeline; return true;
    case eAssetType::tex2D:         *out_res_type =  res::eType::texture_2d; return true;
    case eAssetType::shape:         *out_res_type =  res::eType::shape;     return true;
    case eAssetType::skeleton:      *out_res_type =  res::eType::skeleton;  return true;
    case eAssetType::model3d:       *out_res_type =  res::eType::model_3d;  return true;
    }
}

//**************************************************************** 
void Engine::res_addChild (res::Descr *resPadre, res::Handle child_handle)
{
    res::HandleChain *chain = res_newHandleChain();
    chain->handle = child_handle;

    chain->next = resPadre->figli;
    resPadre->figli = chain;
}

//**************************************************************** 
void Engine::res_addPadre (res::Descr *res, res::Handle padre_handle)
{
    res::HandleChain *chain = res_newHandleChain();
    chain->handle = padre_handle;

    chain->next = res->padri;
    res->padri = chain;
}

res::HandleChain* Engine::res_newHandleChain ()
{
	res::HandleChain *ret = GOSALLOCT(res::HandleChain*, allocator, sizeof(res::HandleChain));
	return ret;
}

void Engine::res_freeHandleChain (res::HandleChain *p)
{
	p->handle.set_invalid();
	p->next = NULL;
	GOSFREE(allocator, p);
}

void* Engine::res_createHandle (res::eType res_type, res::Handle *out_handle)
{
	assert (NULL != out_handle);

	//risorsa non bindata ad un asset
	res::Descr *res = (res::Descr*)resManager.raw_reserve (res_type, out_handle);
	if (NULL == res)
	{
		logger::err ("Engine::res_createHandle() => can't create handle for res type=%d\n", (u8)res_type);
		return NULL;
	}

	res->reset();
	res->refCount = 1;
	res->status = res::eStatus::ready;

	return res;
}

void* Engine::res_getOrCreateHandleFromAsset (const char *uid_runtimeName, res::Handle *out_handle)
{
    assert (NULL != out_handle);
    asset2::UID uid;
    if (!asset2::asset_getBy_rtname (asset_ctx, uid_runtimeName, &uid))
    {
        logger::err ("Engine::res_getOrCreateHandleFromAsset(%s) => invalid runtime name\n", uid_runtimeName);
        return NULL;
    }

	return res_getOrCreateHandleFromAsset (uid, out_handle);
}

//**************************************************************** 
void* Engine::res_getOrCreateHandleFromAsset (asset2::UID uid, res::Handle *out_handle)
{
    assert (uid.isValid());
    assert (NULL != out_handle);

	res::eType res_type;
	if (!res_assetUID_to_resUID (uid, &res_type))
    {
        logger::err ("Engine::res_getOrCreateHandleFromAsset() => can't deduct res_type frome assert uid [%016]" PRIX64 "\n", uid._uid);
        return NULL;
    }


	HashListOfLoadedUID::Position pos;
    u32 handle_asU32;
    if (listof_knownUID.findWithPos (uid, &handle_asU32, &pos))
    {
        //l'asset e' gia' noto e quindi e' gia' stato associato ad un handle.
        //Ritorno quell'handle stesso
        out_handle->set_fromU32(handle_asU32);
        assert (out_handle->get_value_TYPE() == (u32)res_type);

        //incremento il ref count
        res::Descr *res = (res::Descr*)res_get(*out_handle);
        res->refCount++;
        return res;
    }

    //l'asset e' nuovo, devo quindi creare un nuovo handle
    res::Descr *res = (res::Descr*)resManager.raw_reserve (res_type, out_handle);
    if (NULL == res)
    {
        logger::err ("Engine::res_getOrCreateHandleFromAsset() => can't create handle for res type=%d and asset uid=%016" PRIX64 "\n", (u8)res_type, uid._uid);
        return NULL;
    }
    res->reset();
    res->refCount = 1;
    res->status = res::eStatus::notLoaded;

    //inserisco la coppia <uid, handle> in hashlist
    listof_knownUID.insertInPosition (pos, out_handle->view_as_U32());

    asset_logger->log ("res::    new handle [%08X] for asset uid [%016" PRIX64 "]\n", out_handle->view_as_U32(), uid._uid);


    //se questo asset ha delle dipendenze runtime, recupero/creo i relativi handle
    u8 memblock[256];
    asset2::FastUIDList fastUIDList;
    fastUIDList.setupWithBase (memblock, sizeof(memblock), gos::getScrapAllocator());

    asset_logger->incIndent();
    asset2::asset_get_runtime_dependecies_list (asset_ctx, uid, false, &fastUIDList);
    for (u32 i=0; i<fastUIDList.getNElem(); i++)
    {
        const asset2::UID child_uid = fastUIDList(i);
           
        res::Handle child_handle;
        res::Descr *child_res = (res::Descr*)res_getOrCreateHandleFromAsset (child_uid, &child_handle);

        //child_handle diventa uno dei miei figli
        res_addChild (res, child_handle);
        asset_logger->log ("res::    new child [%08X] for [%08X]\n", child_handle.view_as_U32(), out_handle->view_as_U32());

        //io divento uno dei padri di child_handle
        res_addPadre (child_res, *out_handle);
    }
    asset_logger->decIndent();

    return res;
}

void* Engine::res_get (res::Handle handle)
{
    return resManager.raw_get_data (handle);
}

void Engine::res_do_destroy (res::Handle handle)
{
    res::Descr *res = (res::Descr*)res_get(handle);
    if (NULL == res)
        return;

    assert (res->status != res::eStatus::loading);
    //chiamo il "distruttore" di me stesso solo se la risorsa era stata effettivamente caricata
    if (res::eStatus::ready == res->status)
    {
        (this->*res->on_destroy)(res);
    }
    res->status = res::eStatus::notLoaded;

    //se ho dei figli, faccio il release
    res::HandleChain *p = res->figli;
    while (p)
    {
		res::HandleChain *next = p->next;
        res_release (p->handle);
        res_freeHandleChain(p);
		p = next;
    }

    //elimino la lista dei miei padri, ma non c'e' bisogno di notificarli
	//o di rimuovermi dalla lista dei loro figli perche' essendo io refCountato,
	//io posso essere distrutto solo se tutti i miei padri sono a loro volta stati distrutti
	//nel qual caso la loro lista dei figli e' gia' stata pulita
    p = res->padri;
    while (p)
    {
		res_freeHandleChain(p);
		p = p->next;
    }

    //libero l'handle
    resManager.raw_release(handle);
}

void Engine::res_release (res::Handle handle)
{
    res::Descr *res = (res::Descr*)res_get(handle);
    if (NULL == res)
        return;

    assert (res->refCount > 0);
    if (1 == res->refCount)
    {
        //dobbiamo effettivamente fare il free della risorsa
        switch (res->status)
        {
        case res::eStatus::loading:
            //anche questo non dovrebbe succedere perche' quando la risorsa viene passata
            //al loader, il suo ref-count viene incrementato
            DBGBREAK;
            break;

        case res::eStatus::ready:
            res_do_destroy(handle);
            break;

        case res::eStatus::notLoaded:
        case res::eStatus::error:
            //la risorsa non e' stata nemmeno caricata, non c'e' da preoccuparsene, posso fare il "free" immediatamente
            res_do_destroy(handle);
            break;

        }
    }
    else
        res->refCount--;
}


/**************************************************************** 
 * VTX BUFFER
 *****************************************************************/
bool Engine::vtxBuffer_create (u32 sizeInByte, eMemAccessMode mode, ENGVtxBuffer *out_handle)
{
	res::VtxBuffer *res = (res::VtxBuffer*)res_createHandle(res::eType::vtx_buffer, &out_handle->res_handle);
	if (NULL == res)
    {
        logger::err ("Engine::vtxBuffer_create() => can't create handle\n");
        return false;
    }
	
	res->_descr.on_destroy = &Engine::vtxBuffer_on_destroy;

	return gpu->vertexBuffer_create (sizeInByte, mode, &res->vbHandle);
}

void Engine::vtxBuffer_on_destroy (void *resIN)
{
	res::VtxBuffer *res = (res::VtxBuffer*)resIN;
	gpu->deleteResource (res->vbHandle);
	
}


/**************************************************************** 
 * IDX BUFFER
 *****************************************************************/
bool Engine::idxBuffer_create (u32 sizeInByte, eMemAccessMode mode, ENGIdxBuffer *out_handle)
{
	res::IdxBuffer *res = (res::IdxBuffer*)res_createHandle(res::eType::idx_buffer, &out_handle->res_handle);
	if (NULL == res)
    {
        logger::err ("Engine::idxBuffer_create() => can't create handle\n");
        return false;
    }
	
	res->_descr.on_destroy = &Engine::idxBuffer_on_destroy;

	return gpu->indexBuffer_create (sizeInByte, mode, &res->ibHandle);
}

void Engine::idxBuffer_on_destroy (void *resIN)
{
	res::VtxBuffer *res = (res::VtxBuffer*)resIN;
	gpu->deleteResource (res->vbHandle);
}

/**************************************************************** 
 * VTX SHADER
 *****************************************************************/
bool Engine::vtxshader_createFromAsset (const char *uid_runtimeName, ENGVtxShader *out_handle, engine::eLoadMode loadMode)
{
    return res_getOrCreateHandleFromAsset (uid_runtimeName, &out_handle->res_handle);
}

bool Engine::vtxshader_createFromFile (const char *filename, const char *mainFnName, ENGVtxShader *out_handle)
{
	res::Shader *res = (res::Shader*)res_createHandle(res::eType::vtx_shader, &out_handle->res_handle);
	if (NULL == res)
    {
        logger::err ("Engine::vtxshader_createFromFile() => can't create handle\n");
        return false;
    }
	res->_descr.on_destroy = &Engine::vtxshader_on_destroy;

    return gpu->vtxshader_createFromFile (filename, mainFnName, &res->shaderHandle);
}

bool Engine::vtxshader_createFromMemory (const void *bufferIN, u32 bufferSize, const char *mainFnName, ENGVtxShader *out_handle)
{
	res::Shader *res = (res::Shader*)res_createHandle(res::eType::vtx_shader, &out_handle->res_handle);
	if (NULL == res)
    {
        logger::err ("Engine::vtxshader_createFromMemory() => can't create handle\n");
        return false;
    }
	res->_descr.on_destroy = &Engine::vtxshader_on_destroy;

    return gpu->vtxshader_createFromMemory (bufferIN, bufferSize, mainFnName, &res->shaderHandle);
}

void Engine::vtxshader_on_destroy (void *resIN)
{
	res::Shader *res = (res::Shader*)resIN;
	gpu->deleteResource (res->shaderHandle);
}


/**************************************************************** 
 * PXL SHADER
 *****************************************************************/
bool Engine::pxlshader_createFromAsset (const char *uid_runtimeName, ENGPxlShader *out_handle, engine::eLoadMode loadMode)
{
    return res_getOrCreateHandleFromAsset (uid_runtimeName, &out_handle->res_handle);
}

bool Engine::pxlshader_createFromFile (const char *filename, const char *mainFnName, ENGPxlShader *out_handle)
{
	res::Shader *res = (res::Shader*)res_createHandle(res::eType::pxl_shader, &out_handle->res_handle);
	if (NULL == res)
    {
        logger::err ("Engine::pxlshader_createFromFile() => can't create handle\n");
        return false;
    }
	res->_descr.on_destroy = &Engine::pxlshader_on_destroy;

    return gpu->pxlshader_createFromFile (filename, mainFnName, &res->shaderHandle);
}

bool Engine::pxlshader_createFromMemory (const void *bufferIN, u32 bufferSize, const char *mainFnName, ENGPxlShader *out_handle)
{
	res::Shader *res = (res::Shader*)res_createHandle(res::eType::pxl_shader, &out_handle->res_handle);
	if (NULL == res)
    {
        logger::err ("Engine::pxlshader_createFromMemory() => can't create handle\n");
        return false;
    }
	res->_descr.on_destroy = &Engine::pxlshader_on_destroy;

    return gpu->pxlshader_createFromMemory (bufferIN, bufferSize, mainFnName, &res->shaderHandle);
}

void Engine::pxlshader_on_destroy (void *resIN)
{
	res::Shader *res = (res::Shader*)resIN;
	gpu->deleteResource (res->shaderHandle);
}


/**************************************************************** 
 * GPU SHAPE
 *****************************************************************/
bool Engine::GPUShape_create (ENGShape handle_shape, gpu::StageHelper &stageHelper, ENGGPUShape *out_handle)
{
	assert (NULL != out_handle);
	if (map_of_shape_to_gpushape.find(handle_shape, out_handle))
		return true;

	const engine::ResShape *res_shape;
	if (!get (handle_shape, &res_shape))
    {
        logger::err ("Engine::GPUShape_create() => invalid handle_shape\n");
        return false;
    }

	engine::ResGPUShape *res = priv_GPUShape_create(&res_shape->data.shape, stageHelper, out_handle);
	if (NULL == res)
		return false;
	res->handle_shape = handle_shape;

	//mappo la coppia <shape, gpu_shape>
	map_of_shape_to_gpushape.insertIfNotExists (handle_shape, *out_handle);
	return true;
}

bool Engine::GPUShape_create (const gos::Shape *shape, gpu::StageHelper &stageHelper, ENGGPUShape *out_handle)
{
    engine::ResGPUShape *res = priv_GPUShape_create(shape, stageHelper, out_handle);
    if (NULL == res)
    {
        logger::err ("Engine::GPUShape_create() => can't create handle\n");
        return false;
    }
	return true;
}

engine::ResGPUShape* Engine::priv_GPUShape_create (const gos::Shape *shape, gpu::StageHelper &stageHelper, ENGGPUShape *out_handle)
{
    assert (NULL != out_handle);
    engine::ResGPUShape *res = handleList_GPUShape.reserveTS(out_handle);
    if (NULL == res)
    {
        logger::err ("Engine::priv_GPUShape_create() => can't create handle\n");
        return NULL;
    }

    u32 byteNeeded;

    //vtxbuffer
    if (shape->numVtx)
    {
        const u32 sizeOfAVertex = gos::shape::calcSizeOfAVertex(shape->vtxLayout);
        byteNeeded = sizeOfAVertex * shape->numVtx;
        vtxBufferMan.reserve (byteNeeded, &res->alloc_vtxbuf_offset, &res->alloc_vtxbuf_size, &res->vbHandle);
        res->vtxStart = res->alloc_vtxbuf_offset / sizeOfAVertex;
        res->numVertex = shape->numVtx;
    }

    //idxBuffer    
    if (shape->numIdx)
    {
        const u32 sizeOfAnIndex = sizeof(u16);
        byteNeeded = sizeOfAnIndex * shape->numIdx;
        idxBufferMan.reserve (byteNeeded, &res->alloc_idxbuf_offset, &res->alloc_idxbuf_size, &res->ibHandle);
        res->indexStart = res->alloc_idxbuf_offset / sizeOfAnIndex;
        res->numIndices = shape->numIdx;
    }

    res->brh.status = engine::eResStatus::ready;

	stageHelper.begin()
		.mem_to_buffer (shape->vtxBuffer, shape->numVtx * shape::calcSizeOfAVertex(shape->vtxLayout), res->vbHandle, res->alloc_vtxbuf_offset)
		.mem_to_buffer (shape->idxBuffer, shape->numIdx * sizeof(u16), res->ibHandle, res->alloc_idxbuf_offset)
		.submit();
    return res;
}

void Engine::release (ENGGPUShape &handle)
{
    engine::ResGPUShape res;
    if (handleList_GPUShape.releaseTS (handle, &res))
    {
        if (res.numVertex)
            vtxBufferMan.release (res.vbHandle, res.alloc_vtxbuf_offset, res.alloc_vtxbuf_size);

        if (res.numIndices)
            idxBufferMan.release (res.ibHandle, res.alloc_idxbuf_offset, res.alloc_idxbuf_size);

		//unmappo la coppia <shape, gpu_shape>
		map_of_shape_to_gpushape.remove(res.handle_shape);
    }    
    handle.setInvalid();
}


/**************************************************************** 
 * SHAPE
 *****************************************************************/
bool Engine::shape_createFromAsset (const char *uid_runtimeName, ENGShape *out_handle, engine::eLoadMode loadMode)
{
    assert (NULL != out_handle);
    
    asset2::UID uid;
    if (!asset2::asset_getBy_rtname (asset_ctx, uid_runtimeName, &uid))
    {
        logger::err ("Engine::shape_createFromAsset(%s) => invalid runtime name\n", uid_runtimeName);
        return false;
    }

    u32 handle_asU32;
    if (!resHandler_shape.handle_get_or_create_from_asset (this, uid, loadMode, &handle_asU32))
        return false;

    out_handle->setFromU32(handle_asU32);
    return true;
}

bool Engine::shape_create (const VtxLayout &vtxLayout, u32 numVtx, u32 numIdx, ENGShape *out_handle)
{
    assert (NULL != out_handle);
    engine::ResShape *res = resHandler_shape.reserveTS(out_handle);
    if (NULL == res)
    {
        logger::err ("Engine::shape_create() => can't create handle\n");
        return false;
    }

    if (!shape::shapeAlloc (allocator, vtxLayout, numVtx, numIdx, &res->data.shape))
    {
        logger::err ("Engine::shape_create() => error during shapeAlloc\n");
        resHandler_shape.releaseTS (*out_handle, res);
        return false;
    }
    
    res->brh.status = engine::eResStatus::ready;
    return true;
}


/**************************************************************** 
 * SKELETON
 *****************************************************************/
bool Engine::skeleton_createFromAsset (const char *uid_runtimeName, ENGSkeleton *out_handle, engine::eLoadMode loadMode)
{
    assert (NULL != out_handle);
    
    asset2::UID uid;
    if (!asset2::asset_getBy_rtname (asset_ctx, uid_runtimeName, &uid))
    {
        logger::err ("Engine::skeleton_createFromAsset(%s) => invalid runtime name\n", uid_runtimeName);
        return false;
    }

    u32 handle_asU32;
    if (!resHandler_skeleton.handle_get_or_create_from_asset (this, uid, loadMode, &handle_asU32))
        return false;

    out_handle->setFromU32(handle_asU32);
    return true;
}

bool Engine::skeleton_createFromMemory (const u8 *buffer, u32 sizeof_buffer, ENGSkeleton *out_handle)
{
    assert (NULL != out_handle);
    engine::ResSkeleton *res = resHandler_skeleton.reserveTS(out_handle);
    if (NULL == res)
    {
        logger::err ("Engine::skeleton_createFromMemory() => can't create handle\n");
        return false;
    }

	const u32 n = skeleton::deserialize (buffer, sizeof_buffer, allocator, &res->data.skeleton);
    if (0 == n)
    {
        logger::err ("Engine::skeleton_createFromMemory() => error during shapeAlloc\n");
        resHandler_skeleton.releaseTS (*out_handle, res);
        return false;
    }
    
    res->brh.status = engine::eResStatus::ready;
    return true;
}

bool Engine::skeleton_create (const Skeleton &sk, ENGSkeleton *out_handle)
{
	if (skeleton_createFromMemory (sk.blob, skeleton::get_blob_size(sk), out_handle))
		return true;

	logger::err ("Engine::skeleton_create() => error creating skeleton\n");
	return false;
}



/**************************************************************** 
 * MODEL 3d
 *****************************************************************/
bool Engine::model_createFromAsset (const char *uid_runtimeName, ENGModel3d *out_handle, engine::eLoadMode loadMode)
{
    assert (NULL != out_handle);
    
    asset2::UID uid;
    if (!asset2::asset_getBy_rtname (asset_ctx, uid_runtimeName, &uid))
    {
        logger::err ("Engine::model_createFromAsset(%s) => invalid runtime name\n", uid_runtimeName);
        return false;
    }

    u32 handle_asU32;
    if (!resHandler_model3d.handle_get_or_create_from_asset (this, uid, loadMode, &handle_asU32))
        return false;

    out_handle->setFromU32(handle_asU32);
    return true;
}

gos::Model*	Engine::model_create (ENGSkeleton handle_skeleton, u16 num_shape, u16 num_material, u16 num_meshes, ENGModel3d *out_handle)
{
    assert (NULL != out_handle);
    engine::ResModel3d *res = resHandler_model3d.reserveTS(out_handle);
    if (NULL == res)
    {
        logger::err ("Engine::model_create() => can't create handle\n");
        return NULL;
    }

	res->data.model.reset();
	if (!model::alloc (allocator, num_shape, num_material, num_meshes, &res->data.model))
    {
        logger::err ("Engine::model_create() => can't allocate model\n");
		resHandler_model3d.releaseTS (*out_handle, res);
        return NULL;
    }

	model::set_skeleton(res->data.model, handle_skeleton);
    
    res->brh.status = engine::eResStatus::ready;
    return &res->data.model;
}


/**************************************************************** 
 * MODEL INSTANCE
 *****************************************************************/
bool Engine::modelinst_create (ENGModel3d handle_model, ENGModel3dInst *out_handle)
{
	assert (NULL != out_handle);

	const engine::ResModel3d *res_model;
	if (!get (handle_model, &res_model))
	{
		//il modello deve essere "loaded"
		DBGBREAK;
		out_handle->setInvalid();
		return false;
	}

	model::Reader mr (&res_model->data.model);
	const engine::ResSkeleton *res_skeleton;
	if (!get (mr.skeleton_get_handle(), &res_skeleton))
	{
		//lo skeleton deve essere "loaded"
		DBGBREAK;
		out_handle->setInvalid();
		return false;
	}
	skeleton::Reader sr(&res_skeleton->data.skeleton);



    engine::ResModel3dInst *res = handleList_model3dInst.reserveTS(out_handle);
    if (NULL == res)
    {
        logger::err ("Engine::modelinst_create() => can't create handle\n");
        return false;
    }
	res->data.minst.reset();
    res->brh.status = engine::eResStatus::ready;

	
	
	ModelInstance *mi = &res->data.minst;
	mi->allocator = this->allocator;

	mi->num_bones = sr.bone_get_num();
	mi->model_listof_bones = sr.bone_get_by_index(0);

	mi->num_meshes = mr.mesh_get_num();
	mi->listof_meshes = mr.mesh_get_by_index(0);
	
	mi->num_gpushapes = mr.gpushape_get_num();
	mi->listof_gpushapes = mr.gpushape_get_pt_to_list();

	mi->listof_bones = GOSALLOCT(Bone*, allocator, sizeof(Bone) * mi->num_bones);	
	memcpy (mi->listof_bones, mi->model_listof_bones, sizeof(Bone) * mi->num_bones);

	return true;
}

void Engine::release (ENGModel3dInst &handle)
{
    engine::ResModel3dInst res;
    if (handleList_model3dInst.releaseTS (handle, &res))
	{
		res.data.minst.free();
	}
    handle.setInvalid();
}


void Engine::modelinst_applyTransform (ENGModel3dInst handle, const mat4x4f &matW)
{
	engine::ResModel3dInst *res;
	if (handleList_model3dInst.fromHandleToPointer (handle, &res))
		priv_modelinst_applyTransform_ric (res->data.minst.model_listof_bones, res->data.minst.listof_bones, 0, matW);

	//DBGBREAK;
}

//**********************************************************************
void Engine::priv_modelinst_applyTransform_ric (const gos::Bone *model_listof_bones, gos::Bone *listof_bones, u32 boneIndex, const mat4x4f &parent_matW) const
{
    Bone *bone = &listof_bones[boneIndex];
    bone->matrix = parent_matW * model_listof_bones[boneIndex].matrix;
    
    u32 childrenIndex = bone->firstChildIndex;
    while (0xFF != childrenIndex)
    {
        priv_modelinst_applyTransform_ric (model_listof_bones, listof_bones, childrenIndex, bone->matrix);
        childrenIndex = listof_bones[childrenIndex].sigblinIndex;
    }
}


/**************************************************************** 
 * TEXTURE 3D
 *****************************************************************/
bool Engine::texture2D_createFromAsset (const char *uid_runtimeName, ENGTexture *out_handle, engine::eLoadMode loadMode)
{
    assert (NULL != out_handle);
    
    asset2::UID uid;
    if (!asset2::asset_getBy_rtname (asset_ctx, uid_runtimeName, &uid))
    {
        logger::err ("Engine::texture_createFromAsset(%s) => invalid runtime name\n", uid_runtimeName);
        return false;
    }

    u32 handle_asU32;
    if (!resHandler_texture.handle_get_or_create_from_asset (this, uid, loadMode, &handle_asU32))
        return false;

    out_handle->setFromU32(handle_asU32);
    return true;
}

bool Engine::texture2D_create (u16 dimx, u16 dimy, u8 nMipMap, eImageFormat fmt, eMemAccessMode memAccessMode, const void *srcDATA, ENGTexture *out_handle, gpu::StageHelper &stageHelper)
{
    GPUTextureHandle gpuResourceHandle;
    if (!gpu->texture_create2D (dimx, dimy, nMipMap, fmt, memAccessMode, srcDATA, &gpuResourceHandle, stageHelper))
    {
        return false;
    }

    assert (NULL != out_handle);
    engine::ResTexture *res = resHandler_texture.reserveTS(out_handle);
    if (NULL == res)
    {
        logger::err ("Engine::texture_create2D() => can't create handle\n");
        return false;
    }

    res->brh.status = engine::eResStatus::ready;
    res->data.texHandle = gpuResourceHandle;
    return true;
}

bool Engine::texture2D_create (const gos::Image *im, u8 srcTextureNum, eMemAccessMode memAccessMode, ENGTexture *out_handle, gpu::StageHelper &stageHelper)
{
    GPUTextureHandle gpuResourceHandle;
    if (!gpu->texture_create2D (im, srcTextureNum, memAccessMode, &gpuResourceHandle, stageHelper))
    {
        return false;
    }

    assert (NULL != out_handle);
    engine::ResTexture *res = resHandler_texture.reserveTS(out_handle);
    if (NULL == res)
    {
        logger::err ("Engine::texture_create2D() => can't create handle\n");
        return false;
    }

    res->brh.status = engine::eResStatus::ready;
    res->data.texHandle = gpuResourceHandle;
    return true;
}



/**************************************************************** 
 * PIPELIEN
 *****************************************************************/
bool Engine::pipeline_createFromAsset (const char *uid_runtimeName, ENGPipeline *out_handle, engine::eLoadMode loadMode)
{
    assert (NULL != out_handle);
    
    asset2::UID uid;
    if (!asset2::asset_getBy_rtname (asset_ctx, uid_runtimeName, &uid))
    {
        logger::err ("Engine::pipeline_createFromAsset(%s) => invalid runtime name\n", uid_runtimeName);
        return false;
    }

    u32 handle_asU32;
    if (!resHandler_pipeline.handle_get_or_create_from_asset (this, uid, loadMode, &handle_asU32))
        return false;

    out_handle->setFromU32(handle_asU32);
    return true;

    return true;
}






/**********************************************************************************************
* 
* RES LIST
* 
**********************************************************************************************/
void Engine::priv_resList_setup()
{
}

void Engine::priv_resList_unsetup()
{
}

engine::ResourceList* Engine::priv_resList_new_node (engine::Resource *res)
{
    engine::ResourceList *ret = GOSALLOCT(engine::ResourceList*, allocator, sizeof(engine::ResourceList));
	ret->brh = res;
	ret->next = NULL;
	return ret;
}

void Engine::priv_resList_free_node(engine::ResourceList *p)
{
	GOSFREE(allocator, p);
}

void Engine::priv_resList_add_figlio (engine::Resource *padre, engine::Resource *figlio)
{
    //<figlio> e' una subresource di padre
    engine::ResourceList *p = priv_resList_new_node(figlio);
    p->next = padre->figli;
    padre->figli = p;
}

void Engine::priv_resList_add_owner (engine::Resource *me, engine::Resource *my_owner)
{
    //<me> e' una subresource di <my_owner> il quale vuole essere notificato quando <me> cambia di stato
    engine::ResourceList *p = priv_resList_new_node(my_owner);
    p->next = me->deplist;
    me->deplist = p;
}

