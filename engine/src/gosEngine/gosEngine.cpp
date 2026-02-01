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
    vtxBufferMan.unsetup();
    idxBufferMan.unsetup();
    listof_knownUID.unsetup();

    
    //handle lists
    handleList_vtxBuffer.unsetup();
    handleList_idxBuffer.unsetup();
    handleList_GPUShape.unsetup();
	map_of_shape_to_gpushape.unsetup();
    resHandler_texture.unsetup();
    resHandler_pipeline.unsetup();
    resHandler_vtxShader.unsetup();
    resHandler_pxlShader.unsetup();
    resHandler_shape.unsetup();
	resHandler_skeleton.unsetup();
	resHandler_model3d.unsetup();
	resHandler_model3dInst.unsetup();

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
    handleList_vtxBuffer.setup (allocator);
    handleList_idxBuffer.setup (allocator);
    handleList_GPUShape.setup (allocator);
	map_of_shape_to_gpushape.setup (allocator, 8192);
    priv_setup_resource_handler(eAssetType::tex2D,      &resHandler_texture);
    priv_setup_resource_handler(eAssetType::pipe,       &resHandler_pipeline);
    priv_setup_resource_handler(eAssetType::vtx_shader, &resHandler_vtxShader);
    priv_setup_resource_handler(eAssetType::pxl_shader, &resHandler_pxlShader);
    priv_setup_resource_handler(eAssetType::shape,      &resHandler_shape);
	priv_setup_resource_handler(eAssetType::skeleton,	&resHandler_skeleton);
	priv_setup_resource_handler(eAssetType::model3d,	&resHandler_model3d);
	priv_setup_resource_handler(eAssetType::model3dinst,&resHandler_model3dInst);
    
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


                engine::BaseResHandle *brh = static_cast<engine::BaseResHandle*>(res);
                if (bLoadOK)
                    brh->status = engine::eResStatus::ready;
                else
                    brh->status = engine::eResStatus::error;


				//informo tutti quelli che dipendono da questa risorsa del suo cambio di stato
				engine::ResHandleDepList *p = brh->deplist;
				while (p)
				{
					p->brh->callback_onSubresStateChanged (p->brh, brh);
					p = p->next;
				}
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
bool Engine::asset_bind (asset2::UID uid, u32 handle_asU32)
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
            u32 handleAsU32;
            if (!internal__from_asset_to_handle (uid_child, &handleAsU32))
            {
                //devo creare un handle appropriato per l'asset in questione
                BaseResourceHandler *base_resHandler = priv_get_baseResourceHandler(uid_child.getAssetType());

                u32 handle_asU32;
                if (!base_resHandler->handle_get_or_create_from_asset (this, uid_child, engine::eLoadMode::onDemand, &handle_asU32))
                    ret = false;
            }
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
 * VTX BUFFER
 *****************************************************************/
bool Engine::vtxBuffer_create (u32 sizeInByte, eMemAccessMode mode, ENGVtxBuffer *out_handle)
{
    GPUVtxBufferHandle gpuResourceHandle;
    if (!gpu->vertexBuffer_create (sizeInByte, mode, &gpuResourceHandle))
    {
        return false;
    }

    assert (NULL != out_handle);
    engine::ResVtxBuffer *s = handleList_vtxBuffer.reserveTS(out_handle);
    if (NULL == s)
    {
        logger::err ("Engine::vtxBuffer_create() => can't create handle\n");
        return false;
    }

    s->brh.status = engine::eResStatus::ready;
    s->vbHandle = gpuResourceHandle;
    return true;
}

void Engine::release (ENGVtxBuffer &handle)
{
    engine::ResVtxBuffer res;
    if (handleList_vtxBuffer.releaseTS (handle, &res))
        gpu->deleteResource (res.vbHandle);
    handle.setInvalid();
}


/**************************************************************** 
 * IDX BUFFER
 *****************************************************************/
bool Engine::idxBuffer_create (u32 sizeInByte, eMemAccessMode mode, ENGIdxBuffer *out_handle)
{
    GPUIdxBufferHandle gpuResourceHandle;
    if (!gpu->indexBuffer_create (sizeInByte, mode, &gpuResourceHandle))
    {
        return false;
    }

    assert (NULL != out_handle);
    engine::ResIdxBuffer *s = handleList_idxBuffer.reserveTS(out_handle);
    if (NULL == s)
    {
        logger::err ("Engine::idxBuffer_create() => can't create handle\n");
        return false;
    }

    s->brh.status = engine::eResStatus::ready;
    s->ibHandle = gpuResourceHandle;
    return true;
}

void Engine::release (ENGIdxBuffer &handle)
{
    engine::ResIdxBuffer res;
    if (handleList_idxBuffer.releaseTS (handle, &res))
        gpu->deleteResource (res.ibHandle);
    handle.setInvalid();
}



/**************************************************************** 
 * GPU SHAPE
 *****************************************************************/
bool Engine::GPUShape_create (ENGShape handle_shape, ENGGPUShape *out_handle)
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

	engine::ResGPUShape *res = priv_GPUShape_create(&res_shape->data.shape, out_handle);
	if (NULL == res)
		return false;
	res->handle_shape = handle_shape;

	//mappo la coppia <shape, gpu_shape>
	map_of_shape_to_gpushape.insertIfNotExists (handle_shape, *out_handle);
	return true;
}

engine::ResGPUShape* Engine::priv_GPUShape_create (const gos::Shape *shape, ENGGPUShape *out_handle)
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
    return res;
}

bool Engine::GPUShape_create (const gos::Shape *shape, ENGGPUShape *out_handle)
{
    engine::ResGPUShape *res = priv_GPUShape_create(shape, out_handle);
    if (NULL == res)
    {
        logger::err ("Engine::GPUShape_create() => can't create handle\n");
        return false;
    }
	return true;
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

bool Engine::skeleton_create (const u8 *buffer, u32 sizeof_buffer, ENGSkeleton *out_handle)
{
    assert (NULL != out_handle);
    engine::ResSkeleton *res = resHandler_skeleton.reserveTS(out_handle);
    if (NULL == res)
    {
        logger::err ("Engine::skeleton_create() => can't create handle\n");
        return false;
    }

	const u32 n = skeleton::deserialize (buffer, sizeof_buffer, allocator, &res->data.skeleton);
    if (0 == n)
    {
        logger::err ("Engine::skeleton_create() => error during shapeAlloc\n");
        resHandler_skeleton.releaseTS (*out_handle, res);
        return false;
    }
    
    res->brh.status = engine::eResStatus::ready;
    return true;
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

bool Engine::model_create (u16 num_shape, u16 num_material, u16 num_meshes, ENGModel3d *out_handle)
{
    assert (NULL != out_handle);
    engine::ResModel3d *res = resHandler_model3d.reserveTS(out_handle);
    if (NULL == res)
    {
        logger::err ("Engine::model_create() => can't create handle\n");
        return false;
    }

	res->data.model.reset();
	if (!model::alloc (allocator, num_shape, num_material, num_meshes, &res->data.model))
    {
        logger::err ("Engine::model_create() => can't allocate model\n");
		resHandler_model3d.releaseTS (*out_handle, res);
        return false;
    }
    
    res->brh.status = engine::eResStatus::ready;
    return true;
}


/**************************************************************** 
 * MODEL INSTANCE
 *****************************************************************/
bool Engine::modelinst_create (ENGModel3d handle_model, ENGModel3dInst *out_handle)
{
    assert (NULL != out_handle);
    engine::ResModel3dInst *res = resHandler_model3dInst.reserveTS(out_handle);
    if (NULL == res)
    {
        logger::err ("Engine::modelinst_create() => can't create handle\n");
        return false;
    }

	res->data.minst.reset();
	if (!model::alloc (allocator, num_shape, num_material, num_meshes, &res->data.model))
    {
        logger::err ("Engine::modelinst_create() => can't allocate model\n");
		resHandler_model3dInst.releaseTS (*out_handle, res);
        return false;
    }
    
    res->brh.status = engine::eResStatus::ready;
    return true;
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



/**************************************************************** 
 * VTX SHADER
 *****************************************************************/
bool Engine::vtxshader_createFromAsset (const char *uid_runtimeName, ENGVtxShader *out_handle, engine::eLoadMode loadMode)
{
    assert (NULL != out_handle);
    
    asset2::UID uid;
    if (!asset2::asset_getBy_rtname (asset_ctx, uid_runtimeName, &uid))
    {
        logger::err ("Engine::vtxshader_createFromAsset(%s) => invalid runtime name\n", uid_runtimeName);
        return false;
    }
    u32 handle_asU32;
    if (!resHandler_vtxShader.handle_get_or_create_from_asset (this, uid, loadMode, &handle_asU32))
        return false;

    out_handle->setFromU32(handle_asU32);
    return true;
}

bool Engine::vtxshader_createFromFile (const char *filename, const char *mainFnName, ENGVtxShader *out_handle)
{
    GPUShaderHandle gpuResourceHandle;
    if (!gpu->vtxshader_createFromFile (filename, mainFnName, &gpuResourceHandle))
    {
        return false;
    }

    assert (NULL != out_handle);
    engine::ResShader *res = resHandler_vtxShader.reserveTS(out_handle);
    if (NULL == res)
    {
        logger::err ("Engine::vtxshader_createFromFile() => can't create handle\n");
        return false;
    }

    res->brh.status = engine::eResStatus::ready;
    res->data.shaderHandle = gpuResourceHandle;
    return true;
}

bool Engine::vtxshader_createFromMemory (const void *bufferIN, u32 bufferSize, const char *mainFnName, ENGVtxShader *out_handle)
{
    GPUShaderHandle gpuResourceHandle;
    if (!gpu->vtxshader_createFromMemory (bufferIN, bufferSize, mainFnName, &gpuResourceHandle))
    {
        return false;
    }

    assert (NULL != out_handle);
    engine::ResShader *res = resHandler_vtxShader.reserveTS(out_handle);
    if (NULL == res)
    {
        logger::err ("Engine::vtxshader_createFromMemory() => can't create handle\n");
        return false;
    }

    res->brh.status = engine::eResStatus::ready;
    res->data.shaderHandle = gpuResourceHandle;
    return true;
}




/**************************************************************** 
 * PXL SHADER
 *****************************************************************/
bool Engine::pxlshader_createFromAsset (const char *uid_runtimeName, ENGPxlShader *out_handle, engine::eLoadMode loadMode)
{
    assert (NULL != out_handle);
    
    asset2::UID uid;
    if (!asset2::asset_getBy_rtname (asset_ctx, uid_runtimeName, &uid))
    {
        logger::err ("Engine::pxlshader_createFromAsset(%s) => invalid runtime name\n", uid_runtimeName);
        return false;
    }

    u32 handle_asU32;
    if (!resHandler_pxlShader.handle_get_or_create_from_asset (this, uid, loadMode, &handle_asU32))
        return false;

    out_handle->setFromU32(handle_asU32);
    return true;
}

bool Engine::pxlshader_createFromFile (const char *filename, const char *mainFnName, ENGPxlShader *out_handle)
{
    GPUShaderHandle gpuResourceHandle;
    if (!gpu->pxlshader_createFromFile (filename, mainFnName, &gpuResourceHandle))
    {
        return false;
    }

    assert (NULL != out_handle);
    engine::ResShader *res = resHandler_pxlShader.reserveTS(out_handle);
    if (NULL == res)
    {
        logger::err ("Engine::pxlshader_createFromFile() => can't create handle\n");
        return false;
    }

    res->brh.status = engine::eResStatus::ready;
    res->data.shaderHandle = gpuResourceHandle;
    return true;
}

bool Engine::pxlshader_createFromMemory (const void *bufferIN, u32 bufferSize, const char *mainFnName, ENGPxlShader *out_handle)
{
    GPUShaderHandle gpuResourceHandle;
    if (!gpu->pxlshader_createFromMemory (bufferIN, bufferSize, mainFnName, &gpuResourceHandle))
    {
        return false;
    }

    assert (NULL != out_handle);
    engine::ResShader *res = resHandler_pxlShader.reserveTS(out_handle);
    if (NULL == res)
    {
        logger::err ("Engine::pxlshader_createFromMemory() => can't create handle\n");
        return false;
    }

    res->brh.status = engine::eResStatus::ready;
    res->data.shaderHandle = gpuResourceHandle;
    return true;
}



//*******************************************
void Engine::utils__quick_and_dirty__create_GPUSHape_and_stageIt_to_VB_IB (const gos::Shape *shapeSRC, ENGGPUShape *out_handle)
{
    if (!GPUShape_create (shapeSRC, out_handle))
    {
        DBGBREAK;
        return;
    }

    const u32 SIZE_OF_IDX = shapeSRC->numIdx * sizeof(u16);
    const u32 SIZE_OF_VTX = shape::calcSizeOfAVertex(shapeSRC->vtxLayout) * shapeSRC->numVtx;
    const u32 SIZE_OF_STAGE_BUFFER = SIZE_OF_IDX + SIZE_OF_VTX;

    GPUStgBufferHandle stgBufferHandle;
    gpu->stagingBuffer_create (SIZE_OF_STAGE_BUFFER, &stgBufferHandle);
    gpu->stagingBuffer_memcpy (stgBufferHandle, 0, shapeSRC->idxBuffer, SIZE_OF_IDX);
    gpu->stagingBuffer_memcpy (stgBufferHandle, SIZE_OF_IDX, shapeSRC->vtxBuffer, SIZE_OF_VTX);

    const engine::ResGPUShape *shapeInfo;
    if (!get (*out_handle, &shapeInfo))
    {
        DBGBREAK;
        return;
    }

    //creo un job per pushare lo stage buffer in VB/IB
    GPUCmdBufferHandle cmdBufferHandle;
    gpu->cmdBuffer_create (eGPUQueueFamily::transfer, &cmdBufferHandle);

    gos::gpu::CmdBufferWriter2 cw;
    cw.begin (gpu, cmdBufferHandle)
        .copyBuffer (stgBufferHandle, shapeInfo->ibHandle, 0, shapeInfo->alloc_idxbuf_offset, SIZE_OF_IDX)
        .copyBuffer (stgBufferHandle, shapeInfo->vbHandle, SIZE_OF_IDX, shapeInfo->alloc_vtxbuf_offset, SIZE_OF_VTX)
        .end();

    gpu::TransferJob job;
    job.setup (gpu);
    job.submit(cmdBufferHandle);

    while (!job.hasFinished())
    {
    }

	gpu->deleteResource(cmdBufferHandle);
	gpu->deleteResource(stgBufferHandle);

}




